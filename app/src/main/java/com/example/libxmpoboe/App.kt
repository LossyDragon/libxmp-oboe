package com.example.libxmpoboe

import android.app.Application
import android.util.Log
import android.widget.Toast

class App : Application() {
    override fun onCreate() {
        super.onCreate()

        instance = this

        val res = Xmp.initPlayer()
        Toast.makeText(this, "Init: $res", Toast.LENGTH_SHORT).show()
        Log.d(this::class.java.simpleName, "Init: $res")
    }

    companion object {
        @get:Synchronized
        var instance: App? = null
            private set
    }
}