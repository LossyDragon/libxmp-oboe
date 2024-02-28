//
// Created by IT on 2/27/2024.
//

#include <oboe/Oboe.h>
#include <xmp.h>
#include "CircularBuffer.h"

#ifndef OBOEDEMO_ENGINE_H
#define OBOEDEMO_ENGINE_H

using namespace oboe;

class Engine : public AudioStreamCallback {
public:

    Engine() : audioBuffer(nullptr) {};

    ~Engine() { delete audioBuffer; };

    DataCallbackResult
    onAudioReady(AudioStream *audioStream, void *audioData, int32_t numFrames) override;

    void start();

    void stop();

    void loadAndPlay(int fd);

    AudioStream *stream;
    CircularBuffer* audioBuffer;

    struct xmp_frame_info fi;
    struct xmp_module_info mi;
    xmp_context ctx;
};

#endif //OBOEDEMO_ENGINE_H
