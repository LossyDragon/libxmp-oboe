#include <jni.h>
#include "Engine.h"

Engine engine;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_libxmpoboe_Xmp_initPlayer(JNIEnv *env, jobject thiz) {
    (void) env;
    (void) thiz;
    return engine.initPlayer();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_deInitPlayer(JNIEnv *env, jobject thiz) {
    (void) env;
    (void) thiz;
    engine.deInitPlayer();
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_libxmpoboe_Xmp_loadModule(JNIEnv *env, jobject thiz, jint fd) {
    (void) env;
    (void) thiz;
    return engine.loadModule(fd);
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_libxmpoboe_Xmp_startModule(JNIEnv *env, jobject thiz) {
    (void) env;
    (void) thiz;
    return engine.startModule(48000, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_stopModule(JNIEnv *env, jobject thiz) {
    (void) env;
    (void) thiz;
    engine.stopModule();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_libxmpoboe_Xmp_getModuleName(JNIEnv *env, jobject thiz) {
    (void) thiz;
    return env->NewStringUTF(engine.getModuleName());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_libxmpoboe_Xmp_getModuleType(JNIEnv *env, jobject thiz) {
    (void) thiz;
    return env->NewStringUTF(engine.getModuleType());
}

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_example_libxmpoboe_Xmp_getSupportedFormats(JNIEnv *env, jobject thiz) {
    (void) thiz;

    jstring s;
    jclass stringClass;
    jobjectArray stringArray;

    int i, num;
    const char *const *list = engine.getSupportedFormats();
    for (num = 0; list[num] != NULL; num++);

    stringClass = env->FindClass("java/lang/String");
    if (stringClass == NULL)
        return NULL;

    stringArray = env->NewObjectArray(num, stringClass, NULL);
    if (stringArray == NULL)
        return NULL;

    for (i = 0; i < num; i++) {
        s = env->NewStringUTF(list[i]);
        env->SetObjectArrayElement(stringArray, i, s);
        env->DeleteLocalRef(s);
    }

    return stringArray;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_libxmpoboe_Xmp_getVersion(JNIEnv *env, jobject thiz) {
    (void) thiz;
    return env->NewStringUTF(engine.getVersion());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_libxmpoboe_Xmp_getComment(JNIEnv *env, jobject thiz) {
    (void) thiz;
    return env->NewStringUTF(engine.getComment());
}

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_example_libxmpoboe_Xmp_getInstruments(JNIEnv *env, jobject thiz) {
    (void) thiz;

    int i;
    char buf[80];

    jstring s;
    jclass stringClass;
    jobjectArray stringArray;

    int numIns = engine.getNumberOfInstruments();
    xmp_instrument *ins = engine.getInstruments();

    if (!numIns || !ins) {
        return NULL;
    }

    stringClass = env->FindClass("java/lang/String");
    if (stringClass == NULL)
        return NULL;

    stringArray = env->NewObjectArray(numIns, stringClass, NULL);
    if (stringArray == NULL)
        return NULL;

    for (i = 0; i < numIns; i++) {
        snprintf(buf, 80, "%02X %s", i + 1, ins[i].name);
        s = env->NewStringUTF(buf);
        env->SetObjectArrayElement(stringArray, i, s);
        env->DeleteLocalRef(s);
    }

    return stringArray;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_libxmpoboe_Xmp_getTime(JNIEnv *env, jobject thiz) {
    (void) thiz;
    (void) env;
    return engine.getTime();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_getInfo(JNIEnv *env, jobject thiz, jintArray values) {
    (void) thiz;

    xmp_frame_info *frameInfo = engine.getFrameInfo();

    int v[7];

    v[0] = frameInfo->pos;
    v[1] = frameInfo->pattern;
    v[2] = frameInfo->row;
    v[3] = frameInfo->num_rows;
    v[4] = frameInfo->frame;
    v[5] = frameInfo->speed;
    v[6] = frameInfo->bpm;

    env->SetIntArrayRegion(values, 0, 7, v);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_restartModule(JNIEnv *env, jobject thiz) {
    (void) thiz;
    (void) env;
    engine.restartModule();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_libxmpoboe_Xmp_pause(JNIEnv *env, jobject thiz, jboolean is_paused) {
    (void) thiz;
    (void) env;
    return engine.pause(is_paused);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_getModVars(JNIEnv *env, jobject thiz, jintArray vars) {
    (void) thiz;

    int v[8];

    xmp_module_info mi = engine.getModuleInfo();
    int seq = engine.getSequence();

    v[0] = mi.seq_data->duration;
    v[1] = mi.mod->len;
    v[2] = mi.mod->pat;
    v[3] = mi.mod->chn;
    v[4] = mi.mod->ins;
    v[5] = mi.mod->smp;
    v[6] = mi.num_sequences;
    v[7] = seq;

    env->SetIntArrayRegion(vars, 0, 8, v);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_libxmpoboe_Xmp_setSequence(JNIEnv *env, jobject thiz, jint seq) {
    (void) thiz;
    (void) env;
    return engine.setSequence(seq);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_releaseModule(JNIEnv *env, jobject thiz) {
    (void) thiz;
    (void) env;
    engine.releaseModule();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_libxmpoboe_Xmp_endPlayer(JNIEnv *env, jobject thiz) {
    (void) thiz;
    (void) env;
    engine.endPlayer();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_libxmpoboe_Xmp_tick(JNIEnv *env, jobject thiz, jboolean should_loop) {
    (void) thiz;
    (void) env;
    return engine.tick(should_loop);
}