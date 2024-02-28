#include <jni.h>
#include <xmp.h>
#include "Engine.h"

Engine engine;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_startXmp(JNIEnv *env, jobject thiz, jint fd) {
    (void) env;
    (void) thiz;

    engine.start();
    engine.loadAndPlay(fd);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_stopPlaying(JNIEnv *env, jobject thiz) {
    (void) env;
    (void) thiz;

    engine.stopPlaying();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_playOrPause(JNIEnv *env, jobject thiz) {
    engine.playOrPause();
}