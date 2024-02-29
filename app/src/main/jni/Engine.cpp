#include "Engine.h"
#include "oboe/Oboe.h"
#include <chrono>
#include <sys/stat.h>
#include <thread>

// https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst
// https://github.com/google/oboe

using namespace oboe;

// Initialize the oboe engine and create a context for libxmp
bool Engine::initPlayer(int sampleRate) {
    std::lock_guard<std::mutex> lock(mLock);

    if (sampleRate != 8000 && sampleRate != 22050 && sampleRate != 44100 && sampleRate != 48000) {
        LOGD("%d is not a valid sample rate", sampleRate);
        return false;
    }

    AudioStreamBuilder builder;
    builder.setFormat(AudioFormat::I16)
            ->setSampleRate(sampleRate)
            ->setChannelCount(ChannelCount::Stereo)
            ->setPerformanceMode(PerformanceMode::LowLatency)
            ->setSharingMode(SharingMode::Exclusive)
            ->setCallback(this);

    Result resultStream = builder.openStream(stream);

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
        audioBuffer.reset(nullptr);
    }
    // https://github.com/google/oboe/discussions/1258
    uint32_t capacityInFrames = sampleRate / 2;
    audioBuffer = std::make_unique<oboe::FifoBuffer>(stream->getBytesPerFrame(), capacityInFrames);

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
    stream.reset();
    xmp_free_context(ctx);
}

bool Engine::loadModule(int fd) {
    std::lock_guard<std::mutex> lock(mLock);

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

TickResult Engine::tick(bool shouldLoop) {
    std::lock_guard<std::mutex> lock(mLock);

    if (!isLoaded) {
        LOGD("Not loaded in ::tick");
        return TickResult::Fail;
    }

    if (!isPlaying) {
        LOGD("Not playing in ::tick");
        return TickResult::Fail;
    }

    if (!isPaused && !moduleEnded) {
        int res = xmp_play_frame(ctx);
        if (res == 0) {
            xmp_get_frame_info(ctx, &fi);

            size_t bufferCapacity = audioBuffer->getBufferCapacityInFrames();
            size_t availableSpace = bufferCapacity - audioBuffer->getFullFramesAvailable();

            while (availableSpace < fi.buffer_size / sizeof(float)) {
                LOGD("Waiting for buffer space...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                availableSpace = bufferCapacity - audioBuffer->getFullFramesAvailable();
            }

            audioBuffer->write(static_cast<float *>(fi.buffer), fi.buffer_size / sizeof(float));

            //LOGD("%3d/%3d %3d/%3d | Loop: %d | Should Loop: %d\r",
            //     fi.pos, mi.mod->len, fi.row, fi.num_rows, fi.loop_count, shouldLoop);

            // TODO: If we 'shouldLoop' when we already looped more than once, honor it before finishing
            if (fi.loop_count > 0 && !shouldLoop) {
                moduleEnded = true;
            }
        } else {
            LOGD("Couldn't play frame in ::tick");
        }
    }

    if (moduleEnded) {
        // Module has ended, just wait for the buffer to empty
        while (audioBuffer->getFullFramesAvailable() > 0) {
            LOGD("Waiting...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        return TickResult::End;
    }

    return TickResult::Continue;
}

void Engine::stopModule() {
    xmp_stop_module(ctx);
}

char *Engine::getModuleName() {
    return mi.mod->name;
}

char *Engine::getModuleType() {
    if (mi.mod == nullptr) {
        return nullptr;
    }
    return mi.mod->type;
}

char *Engine::getComment() {
    if (mi.comment) {
        return mi.comment;
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
    if (mi.mod == nullptr) {
        return 0;
    }
    return mi.mod->ins;
}

xmp_instrument *Engine::getInstruments() {
    if (mi.mod == nullptr) {
        return nullptr;
    }
    return mi.mod->xxi;
}

int Engine::getTime() {
    std::lock_guard<std::mutex> lock(mLock);
    return fi.time;
}

xmp_frame_info *Engine::getFrameInfo() {
    std::lock_guard<std::mutex> lock(mLock);
    return &fi;
}

void Engine::restartModule() {
    xmp_restart_module(ctx);
}

bool Engine::pause(bool pause) {
    std::lock_guard<std::mutex> lock(mLock);
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
    std::lock_guard<std::mutex> lock(mLock);
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
        int16_t *outputData = static_cast<int16_t *>(audioData); // 16-bit PCM
        size_t framesRead = audioBuffer->read(outputData, static_cast<size_t>(numFrames));

        if (framesRead < numFrames) {
            // Fill the remaining frames with silence
            std::fill(outputData + framesRead, outputData + numFrames, 0.0f);
        }
    } else {
        // Silence - Handle if were paused or audioBuffer is null
        memset(audioData, 0, sizeof(float) * numFrames);
    }

    return DataCallbackResult::Continue;
}
