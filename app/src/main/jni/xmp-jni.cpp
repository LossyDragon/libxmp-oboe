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