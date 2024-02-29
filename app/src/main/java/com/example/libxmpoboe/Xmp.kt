package com.example.libxmpoboe

import android.net.Uri
import java.lang.RuntimeException

object Xmp {

    init {
        System.loadLibrary("xmp-jni")
    }

    external fun deInitPlayer()
    external fun endPlayer()
    external fun getComment(): String?
    external fun getInfo(values: IntArray?)
    external fun getInstruments(): Array<String>?
    external fun getModVars(vars: IntArray?)
    external fun getModuleName(): String?
    external fun getModuleType(): String?
    external fun getSupportedFormats(): Array<String>?
    external fun getTime(): Int
    external fun getVersion(): String
    external fun initPlayer(): Boolean
    external fun pause(isPaused: Boolean): Boolean
    external fun releaseModule()
    external fun restartModule()
    external fun setSequence(seq: Int): Boolean
    external fun startModule(): Boolean
    external fun stopModule()
    external fun tick(shouldLoop: Boolean): Int

    private external fun loadModule(fd: Int): Boolean

    fun loadFromFd(uri: Uri): Boolean {
        val context = App.instance!!.applicationContext
        val pfd = context.contentResolver.openFileDescriptor(uri, "r")
        if (pfd != null) {
            val fd = pfd.detachFd()
            pfd.close()

            return loadModule(fd)
        }

        throw RuntimeException("Failed to load from fd")
    }
}