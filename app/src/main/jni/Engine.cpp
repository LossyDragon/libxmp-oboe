#include <sys/stat.h>
#include "Engine.h"
#include "oboe/Oboe.h"
#include <android/log.h>
#include <chrono>
#include <iostream>
#include <thread>

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

        while (xmp_play_frame(ctx) == 0) {
            xmp_get_frame_info(ctx, &fi);

            // Check buffer fullness; assume total capacity is known
            size_t bufferCapacity = audioBuffer->capacity; // Assume you have a way to get this
            size_t availableSpace = bufferCapacity - audioBuffer->available();

            while (availableSpace < fi.buffer_size / sizeof(float)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simple wait
                availableSpace = bufferCapacity - audioBuffer->available(); // Update available space
            }

            audioBuffer->write(static_cast<float*>(fi.buffer), fi.buffer_size / sizeof(float));

            if (fi.loop_count > 0) {
                while(!audioBuffer->isEmpty()) {
                    // Finish the buffer
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                break;
            }

            LOGD("%3d/%3d %3d/%3d\r", fi.pos, mi.mod->len, fi.row, fi.num_rows);
        }

        xmp_end_player(ctx);
    }

    xmp_free_context(ctx);
    Engine::stop();
}

void Engine::stop() {
    stream->requestStop();
    stream->close();
}

DataCallbackResult
Engine::onAudioReady(AudioStream *audioStream, void *audioData, int32_t numFrames) {
    if (audioBuffer) { // Check if audioBuffer has been initialized
        float* outputData = static_cast<float*>(audioData);
        size_t framesRead = audioBuffer->read(outputData, static_cast<size_t>(numFrames));

        if (framesRead < numFrames) {
            std::fill(outputData + framesRead, outputData + numFrames, 0.0f);
        }

        return DataCallbackResult::Continue;
    } else {
        // Handle the case where audioBuffer is not initialized, possibly by outputting silence
        memset(audioData, 0, sizeof(float) * numFrames);
        return DataCallbackResult::Continue;
    }
}
