#include <oboe/Oboe.h>
#include <xmp.h>
#include <android/log.h>

#ifndef OBOEDEMO_ENGINE_H
#define OBOEDEMO_ENGINE_H

#define LOG_TAG "Xmp Test"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

enum class TickResult : int32_t {
    Continue = 0,
    Fail = 1,
    End = 2,
};

using namespace oboe;

class Engine : public AudioStreamCallback {
public:

    Engine() : audioBuffer(nullptr) {};

    virtual ~Engine() = default;

    DataCallbackResult
    onAudioReady(AudioStream *audioStream, void *audioData, int32_t numFrames) override;

    bool setSequence(int seq);

    TickResult tick(bool shouldLoop);

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
    std::mutex mLock;
    std::shared_ptr<oboe::AudioStream> stream;
    std::unique_ptr<oboe::FifoBuffer> audioBuffer;

    bool isInit;    // Is Oboe and Xmp created?
    bool isLoaded;  // Is libxmp in a loaded state?
    bool isPaused;  // Is the app calling for pause?
    bool isPlaying; // Is libxmp ready to play?
    bool moduleEnded; // Has the module finished playing once?
    int sequence;   // Module sequence
    int loopCount; // Current loop count

    struct xmp_frame_info fi;
    struct xmp_module_info mi;
    xmp_context ctx;
};

#endif //OBOEDEMO_ENGINE_H
