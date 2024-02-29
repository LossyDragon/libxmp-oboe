#include "Engine.h"
#include "oboe/Oboe.h"
#include <chrono>
#include <sys/stat.h>
#include <thread>

// https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst
// https://github.com/google/oboe

using namespace oboe;

/**
 * This function basically heartbeats libxmp to play one frame at a time (usually ~50fps *)
 * Each frame will also get the current "frame info" to fill into a [FifoBuffer] which [onAudioReady]
 * oboe is set for 'Low Latency' so when it is ready to fill its own buffer for audio.
 * The number of frames to play can vary widely, so if the audio buffer has less than the number
 * of frames, silence will be filled in which is unnoticeable.
 *
 * If we're [isPaused], [onAudioReady] will just fill the frames with silence.
 *
 * Once a module is finished, there is most likely still a buffer of audio to be rendered. Another
 * loop will be held until the [audioBuffer] is empty.
 *
 * @see * https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst#introduction
 * @param shouldLoop whether the module should loop, provided by the thread controlling the tick
 * @return a result via [TickResult]: Continue, End, or Fail.
 */
TickResult Engine::tick(bool shouldLoop) {
    std::lock_guard<std::mutex> lock(mLock);

    if (!isLoaded) {
        LOGE("Not loaded in ::tick");
        return TickResult::Fail;
    }

    if (!isPlaying) {
        LOGE("Not playing in ::tick");
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
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
            LOGE("Couldn't play frame in ::tick");
            return TickResult::Fail;
        }
    }

    if (moduleEnded) {
        // Module has ended, just wait for the buffer to empty
        while (audioBuffer->getFullFramesAvailable() > 0) {
            LOGD("Waiting...");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        return TickResult::End;
    }

    return TickResult::Continue;
}

/**
 * Initialize oboe, audio buffer, and libxmp.
 * @param sampleRate a valid sample rate (8000, 22050, 44100, 48000)
 * @return True if oboe, audio buffer, and xmp were initialized correctly, otherwise false.
 */
bool Engine::initPlayer(int sampleRate) {
    std::lock_guard<std::mutex> lock(mLock);

    if (sampleRate != 8000 && sampleRate != 22050 && sampleRate != 44100 && sampleRate != 48000) {
        LOGE("%d is not a valid sample rate", sampleRate);
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
        LOGE("Unable to open stream");
        return false;
    }

    stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);
    Result resultStart = stream->requestStart();

    if (Result::OK != resultStart) {
        LOGE("Unable to request start ");
        return false;
    }

    if (audioBuffer != nullptr) {
        audioBuffer.reset(nullptr);
    }
    // https://github.com/google/oboe/discussions/1258
    uint32_t capacityInFrames = sampleRate / 2;
    audioBuffer = std::make_unique<oboe::FifoBuffer>(stream->getBytesPerFrame(), capacityInFrames);

    if (NULL != ctx) {
        ctx = NULL;
    }
    ctx = xmp_create_context();

    isInit = stream && audioBuffer && ctx;

    LOGD("Is player initialized? %d", isInit);

    return isInit;
}

/**
 * De-Initialize the player, this free's up the stream and free's context from libxmp
 */
void Engine::deInitPlayer() {
    stream->requestStop();
    stream->close();
    stream.reset();
    xmp_free_context(ctx);
    ctx = NULL;
}

/**
 * Load a module to libxmp using a File-Descriptor
 * @param fd The file descriptor
 * @return true if [XMP_STATE_LOADED] is true, otherwise false.
 */
bool Engine::loadModule(int fd) {
    std::lock_guard<std::mutex> lock(mLock);

    FILE *file = fdopen(fd, "r");
    if (file == NULL) {
        LOGE("Couldn't get file from fd");
        return false;
    }

    struct stat statBuf;
    if (fstat(fd, &statBuf) != 0) {
        fclose(file);
        return false;
    }
    off_t size = statBuf.st_size;

    int res = xmp_load_module_from_file(ctx, file, size);
    fclose(file);

    if (res != 0) {
        LOGE("Couldn't load module from file");
        return false;
    }

    moduleEnded = false;
    sequence = 0;
    xmp_get_module_info(ctx, &mi);

    isLoaded = xmp_get_player(ctx, XMP_STATE_LOADED);
    LOGD("Loaded: $d -> %s (%s)\n", mi.mod->name, mi.mod->type);

    return isLoaded;
}

/**
 * (libxmp) Start playing the currently loaded module.
 *
 * Note: This does not fill any audio buffer, only to init the module/player to a default start state
 *
 * Check out [::tick] to loop through the frame and fill a buffer for [onAudioReady] to consume
 *
 * @see https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst#int-xmp_start_playerxmp_context-c-int-rate-int-format
 * @param rate the sample rate to use. This should be the same as [initPlayer]
 * @param format bitmapped configurable player flags, one or more of the following: [XMP_FORMAT_8BIT], [XMP_FORMAT_UNSIGNED], or [XMP_FORMAT_MONO]
 * @return true if libxmp state is [XMP_STATE_PLAYING], otherwise false.
 */
bool Engine::startModule(int rate, int format) {
    if (!ctx) {
        LOGE("Context is null in ::startModule");
        return false;
    }

    int res = xmp_start_player(ctx, rate, format);
    if (res != 0) {
        LOGE("Unable to start player. Rate %d, Format: %d", rate, format);
        return false;
    }

    isPlaying = xmp_get_player(ctx, XMP_STATE_PLAYING);

    return isPlaying;
}

/**
 * (libxmp) Stop the currently playing module.
 *
 * @see https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst#void-xmp_stop_modulexmp_context-c
 */
void Engine::stopModule() {
    xmp_stop_module(ctx);
}

/**
 * Get the module name
 * @return the module name, can also be null.
 */
char *Engine::getModuleName() {
    return mi.mod->name;
}

/**
 * Get the module format
 * @return the module format (ie: Composer 669)
 */
char *Engine::getModuleType() {
    if (mi.mod == nullptr) {
        return nullptr;
    }
    return mi.mod->type;
}

/**
 * Get a module's comment, if any
 * @return module comment, otherwise null
 */
char *Engine::getComment() {
    if (!mi.comment) {
        return NULL;
    }
    return mi.comment;
}

/**
 * Get a list of supported module formats from libxmp
 * @return a list of formats
 */
const char *const *Engine::getSupportedFormats() {
    return xmp_get_format_list();
}

/**
 * Get the libxmp version number
 * @return the version number
 */
const char *Engine::getVersion() {
    return xmp_version;
}

/**
 * Get the number of instruments from the module
 * @return the number of instruments
 */
int Engine::getNumberOfInstruments() {
    if (mi.mod == nullptr) {
        return 0;
    }
    return mi.mod->ins;
}

/**
 * Get a list of instruments from the module
 * @see [xmp_instrument]
 * @see [xmp_module]
 * @return
 */
xmp_instrument *Engine::getInstruments() {
    if (mi.mod == nullptr) {
        return nullptr;
    }
    return mi.mod->xxi;
}

/**
 * Get the current time in ms
 * @return the current time in ms
 */
int Engine::getTime() {
    std::lock_guard<std::mutex> lock(mLock);
    return fi.time;
}

/**
 * Get the frame info
 *
 * @see [Engine::fi]
 * @see [xmp_frame_info]
 * @return the frame info
 */
xmp_frame_info *Engine::getFrameInfo() {
    std::lock_guard<std::mutex> lock(mLock);
    return &fi;
}

/**
 * (libxmp) Restart the currently playing module.
 *
 * @see https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst#void-xmp_restart_modulexmp_context-c
 */
void Engine::restartModule() {
    xmp_restart_module(ctx);
}

/***
 * Pause playback
 * @param pause if we should pause playback
 * @return the current state of pause
 */
bool Engine::pause(bool pause) {
    std::lock_guard<std::mutex> lock(mLock);
    isPaused = pause;
    return isPaused;
}

/**
 * Get the module data
 *
 * @see [Engine::mi]
 * @see [xmp_module_info]
 * @return the module data
 */
xmp_module_info Engine::getModuleInfo() {
    return mi;
}

/**
 * Get the current sequence the module is playing at.
 * @return the sequence number
 *
 * @see [setSequence]
 */
int Engine::getSequence() {
    return sequence;
}

/**
 * (libxmp) Release memory allocated by a module from the specified player context.
 *
 * @see https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst#void-xmp_release_modulexmp_context-c
 */
void Engine::releaseModule() {
    xmp_release_module(ctx);
}

/**
 * (libxmp) End module replay and release player memory.
 *
 * @see https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst#void-xmp_end_playerxmp_context-c
 */
void Engine::endPlayer() {
    xmp_end_player(ctx);
}

/**
 * (libxmp) Skip replay to the start of the given position.
 *
 * @see https://github.com/libxmp/libxmp/blob/master/docs/libxmp.rst#int-xmp_set_positionxmp_context-c-int-pos
 * @param seq the position index to set.
 * @return true if the position was set successfully, otherwise false if invalid
 */
bool Engine::setSequence(int seq) {
    std::lock_guard<std::mutex> lock(mLock);
    if (seq >= mi.num_sequences)
        return false;

    if (mi.seq_data->duration <= 0)
        return false;

    if (seq == sequence)
        return false;

    sequence = seq;

    int res = xmp_set_position(ctx, mi.seq_data[sequence].entry_point);
    xmp_play_buffer(ctx, NULL, 0, 0);

    if (res > -1) return true;
    else return false;
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
