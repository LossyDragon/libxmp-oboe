#include <oboe/Oboe.h>
#include <xmp.h>
#include <android/log.h>

#ifndef OBOEDEMO_ENGINE_H
#define OBOEDEMO_ENGINE_H

#define LOG_TAG "Xmp Test"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using namespace oboe;

class Engine : public AudioStreamCallback {
public:

    Engine() : audioBuffer(nullptr) {};

    ~Engine() {
        audioBuffer.release();
        delete stream;
    };

    DataCallbackResult
    onAudioReady(AudioStream *audioStream, void *audioData, int32_t numFrames) override;

    bool setSequence(int seq);

    bool tick(bool shouldLoop);

    char *getComment();

    char *getModuleName();

    char *getModuleType();

    const char *const *getSupportedFormats();

    const char *getVersion();

    int getNumberOfInstruments();

    int getSequence();

    int getTime();

    void deInitPlayer();

    void endPlayer();

    bool initPlayer(int sampleRate);

    bool loadModule(int fd);

    bool pause(bool pause);

    void releaseModule();

    void restartModule();

    bool startModule(int rate, int format);

    void stopModule();

    xmp_frame_info *getFrameInfo();

    xmp_instrument *getInstruments();

    xmp_module_info getModuleInfo();

private:
    AudioStream *stream;
    std::unique_ptr<oboe::FifoBuffer> audioBuffer;

    bool isInit;    // Is Oboe and Xmp created?
    bool isLoaded;  // Is libxmp in a loaded state?
    bool isPlaying; // Is libxmp ready to play?
    bool isPaused;  // Is the app calling for pause?
    int sequence;   // Module sequence
    bool moduleEnded; // Has the module finished playing once?

    struct xmp_frame_info fi;
    struct xmp_module_info mi;
    xmp_context ctx;
};

#endif //OBOEDEMO_ENGINE_H
