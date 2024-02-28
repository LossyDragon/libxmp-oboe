#include <oboe/Oboe.h>
#include <xmp.h>
#include "CircularBuffer.h"

#ifndef OBOEDEMO_ENGINE_H
#define OBOEDEMO_ENGINE_H


#define LOG_TAG "Xmp Test"

#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

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

    void stopPlaying();

    void playOrPause();

    AudioStream *stream;
    CircularBuffer *audioBuffer;

    bool isPlaying;
    bool moduleEnded;

    struct xmp_frame_info fi;
    struct xmp_module_info mi;
    xmp_context ctx;
};

#endif //OBOEDEMO_ENGINE_H
