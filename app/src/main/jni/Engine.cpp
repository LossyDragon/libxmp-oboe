#include <sys/stat.h>
#include "Engine.h"
#include "oboe/Oboe.h"
#include <android/log.h>
#include <chrono>
#include <iostream>
#include <thread>

// https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst
// https://github.com/google/oboe

using namespace oboe;

// audioBuffer->flush(); // TODO

// Initialize the oboe engine and create a context for libxmp
bool Engine::initPlayer() {
    AudioStreamBuilder builder;
    builder.setFormat(AudioFormat::I16)
            ->setSampleRate(48000)
            ->setChannelCount(ChannelCount::Stereo)
            ->setPerformanceMode(PerformanceMode::LowLatency)
            ->setSharingMode(SharingMode::Exclusive)
            ->setCallback(this);

    if (stream != nullptr) {
        delete stream;
    }

    Result resultStream = builder.openStream(&stream);

    if (Result::OK != resultStream) {
        LOGD("Unable to open stream");
        return false;
    }

    stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);
    Result resultStart = stream->requestStart();

    if (Result::OK != resultStart) {
        LOGD("Unable to request start ");
        return false;
    }

    if (audioBuffer != nullptr) {
        delete audioBuffer;
    }
    audioBuffer = new CircularBuffer(48000);

    ctx = xmp_create_context();

    if (NULL == ctx) {
        LOGD("Unable to create xmp context");
        return false;
    }

    isInit = stream && audioBuffer && ctx;

    LOGD("Is player initialized? %d", isInit);

    return isInit;
}

void Engine::deInitPlayer() {
    stream->requestStop();
    stream->close();
    xmp_free_context(ctx);
}

bool Engine::loadModule(int fd) {
    FILE *file = fdopen(fd, "r");
    if (file == NULL) {
        LOGD("Couldnt get file from fd");
        return false;
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) != 0) {
        fclose(file);
        return false;
    }
    off_t size = statbuf.st_size;


    int res = xmp_load_module_from_file(ctx, file, size);
    fclose(file);

    if (res != 0) {
        LOGD("Couldnt load module from file");
        return false;
    }

    audioBuffer->flush();
    moduleEnded = false;
    sequence = 0;
    xmp_get_module_info(ctx, &mi);
    LOGD("%s (%s)\n", mi.mod->name, mi.mod->type);

    isLoaded = xmp_get_player(ctx, XMP_STATE_LOADED);
    LOGD("Is player loaded? %d", isLoaded);

    return isLoaded;
}

bool Engine::startModule(int rate, int format) {
    int res = xmp_start_player(ctx, rate, format);
    if (res != 0) {
        LOGD("Unable to start player. Rate %d, Format: %d", rate, format);
        return false;
    }

    isPlaying = xmp_get_player(ctx, XMP_STATE_PLAYING);

    return isPlaying;
}

bool Engine::tick(bool shouldLoop) {

    if (!isLoaded) {
        LOGD("Not loaded in ::tick");
        return false;
    }

    if (!isPlaying) {
        LOGD("Not playing in ::tick");
        return false;
    }

    if (!isPaused && !moduleEnded) {
        int res = xmp_play_frame(ctx);
        if (res == 0) {
            xmp_get_frame_info(ctx, &fi);

            size_t bufferCapacity = audioBuffer->capacity;
            size_t availableSpace = bufferCapacity - audioBuffer->available();

            while (availableSpace < fi.buffer_size / sizeof(float)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                availableSpace = bufferCapacity - audioBuffer->available();
            }

            audioBuffer->write(static_cast<float *>(fi.buffer), fi.buffer_size / sizeof(float));

            LOGD("%3d/%3d %3d/%3d | Loop: %d | Should Loop: %d\r",
                 fi.pos, mi.mod->len, fi.row, fi.num_rows, fi.loop_count, shouldLoop);

            // TODO: If we 'shouldLoop' when we already looped more than once, honor it before finishing
            if (fi.loop_count > 0 && !shouldLoop) {
                moduleEnded = true;
            }
        } else {
            LOGD("Couldn't play frame in ::tick");
        }
    }

    if (moduleEnded && !audioBuffer->isEmpty()) {
        while (!audioBuffer->isEmpty()) {
            // Module has ended, just wait for the buffer to empty
            LOGD("Waiting...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // TODO: notify that we have finished playing, somehow.
    }

    return true;
}

void Engine::stopModule() {
    xmp_stop_module(ctx);
}

char *Engine::getModuleName() {
    return mi.mod->name;
}

char *Engine::getModuleType() {
    return mi.mod->type;
}

char *Engine::getComment() {
    if (mi.comment) {
        char *comment = strdup(mi.comment); // Free?
        if (!comment) {
            return NULL;
        }
        return comment;
    } else {
        return NULL;
    }
}

const char *const *Engine::getSupportedFormats() {
    return xmp_get_format_list();
}

const char *Engine::getVersion() {
    return xmp_version;
}

int Engine::getNumberOfInstruments() {
    return mi.mod->ins;
}

xmp_instrument *Engine::getInstruments() {
    return mi.mod->xxi;
}

int Engine::getTime() {
    return fi.time;
}

xmp_frame_info *Engine::getFrameInfo() {
    return &fi;
}

void Engine::restartModule() {
    xmp_restart_module(ctx);
}

bool Engine::pause(bool pause) {
    isPaused = pause;
    return isPaused;
}

xmp_module_info Engine::getModuleInfo() {
    return mi;
}

int Engine::getSequence() {
    return sequence;
}

void Engine::releaseModule() {
    xmp_release_module(ctx);
}

void Engine::endPlayer() {
    xmp_end_player(ctx);
}

bool Engine::setSequence(int seq) {
    if (seq >= mi.num_sequences)
        return false;

    if (mi.seq_data->duration <= 0)
        return false;

    if (seq == sequence)
        return false;

    sequence = seq;

    xmp_set_position(ctx, mi.seq_data[sequence].entry_point);
    xmp_play_buffer(ctx, NULL, 0, 0);

    return true;
}

DataCallbackResult
Engine::onAudioReady(AudioStream *audioStream, void *audioData, int32_t numFrames) {
    if (audioBuffer && !isPaused) {
        float *outputData = static_cast<float *>(audioData);
        size_t framesRead = audioBuffer->read(outputData, static_cast<size_t>(numFrames));

        if (framesRead < numFrames) {
            std::fill(outputData + framesRead, outputData + numFrames, 0.0f);
        }
    } else {
        // Handle the case where audioBuffer is not initialized or paused.
        memset(audioData, 0, sizeof(float) * numFrames);
    }

    return DataCallbackResult::Continue;
}
