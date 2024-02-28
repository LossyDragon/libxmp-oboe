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

void Engine::start() {
    AudioStreamBuilder b;
    b.setFormat(AudioFormat::I16);
    b.setSampleRate(48000);
    b.setChannelCount(ChannelCount::Stereo);
    b.setPerformanceMode(PerformanceMode::LowLatency);
    b.setSharingMode(SharingMode::Exclusive);

    b.setCallback(this);

    b.openStream(&stream);

    if (audioBuffer != nullptr) {
        delete audioBuffer;
    }
    audioBuffer = new CircularBuffer(48000);

    stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);
    stream->requestStart();
}

void Engine::loadAndPlay(int fd) {

    ctx = xmp_create_context();

    FILE *file = fdopen(fd, "r");
    if (file == NULL) {
        return;
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) != 0) {
        fclose(file);
        return;
    }
    off_t size = statbuf.st_size;

    xmp_load_module_from_file(ctx, file, size);
    fclose(file);

    if (xmp_start_player(ctx, 48000, 0) == 0) {
        xmp_get_module_info(ctx, &mi);
        LOGD("%s (%s)\n", mi.mod->name, mi.mod->type);

        isPlaying = xmp_get_player(ctx, XMP_STATE_PLAYING);

        moduleEnded = false;
        while (!moduleEnded || !audioBuffer->isEmpty()) {
            if (!moduleEnded && isPlaying) {
                if (xmp_play_frame(ctx) == 0) {
                    xmp_get_frame_info(ctx, &fi);

                    size_t bufferCapacity = audioBuffer->capacity; // Assume you have a way to get this
                    size_t availableSpace = bufferCapacity - audioBuffer->available();

                    while (availableSpace < fi.buffer_size / sizeof(float)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        availableSpace = bufferCapacity - audioBuffer->available();
                    }

                    audioBuffer->write(static_cast<float *>(fi.buffer),
                                       fi.buffer_size / sizeof(float));

                    if (fi.loop_count > 0) {
                        moduleEnded = true;
                    }

                    LOGD("%3d/%3d %3d/%3d\r", fi.pos, mi.mod->len, fi.row, fi.num_rows);
                } else {
                    LOGD("Uh ohhh!");
                    break;
                }
            } else {
                // Module has ended, just wait for the buffer to empty
                LOGD("Waiting...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        xmp_end_player(ctx);
    }

    xmp_free_context(ctx);
    Engine::stop();
}

void Engine::stopPlaying() {
    xmp_release_module(ctx);
    xmp_stop_module(ctx);
}

void Engine::playOrPause() {
    isPlaying = !isPlaying;
}

void Engine::stop() {
    stream->requestStop();
    stream->close();
}

DataCallbackResult
Engine::onAudioReady(AudioStream *audioStream, void *audioData, int32_t numFrames) {
    if (audioBuffer && isPlaying) {
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
