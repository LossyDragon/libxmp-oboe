package com.example.libxmpoboe

import android.net.Uri
import java.lang.RuntimeException

object Xmp {

    init {
        System.loadLibrary("xmp-jni")
    }

    private external fun startXmp(fd: Int)

    fun loadFromFd(uri: Uri) {
        val context = App.instance!!.applicationContext
        val pfd = context.contentResolver.openFileDescriptor(uri, "r")
        if (pfd != null) {
            val fd = pfd.detachFd()
            pfd.close()

            startXmp(fd)
            return
        }

        throw RuntimeException("Failed to load from fd")
    }
}