package com.example.libxmpoboe

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.os.Bundle
import android.os.Handler
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.libxmpoboe.ui.theme.AppTheme
import kotlin.concurrent.thread

/**
 * Steps to play a module in for testing purposes
 *
 * 1. Init (Done automatically by [App]
 * 2. Load File
 * 3. Start Module (Gray buttons should be functional at this point)
 * 4. Play
 */

@Suppress("DEPRECATION")
class MainActivity : ComponentActivity() {
    private lateinit var sharedPref: SharedPreferences

    private fun toast(message: String?) {
        runOnUiThread {
            Toast.makeText(this, message.orEmpty(), Toast.LENGTH_SHORT).show()
            Log.d(this::class.java.simpleName, message.orEmpty())
        }
    }

    private var isLooping by mutableStateOf(false)

    private val handler: Handler = Handler()
    private val runnable = object : Runnable {
        // This should be done in a service.
        override fun run() {
            val res = Xmp.tick(isLooping)

            if (res == 2) {
                Xmp.endPlayer()
                Xmp.releaseModule()
                Xmp.deInitPlayer()
                return
            }

            handler.postDelayed(this, 10)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        sharedPref = getSharedPreferences("pref", Context.MODE_PRIVATE)
        // val uriString = sharedPref.getString("uri", null)
        // val savedUri: Uri? = uriString?.let { Uri.parse(it) }

        setContent {
            var isPaused by remember { mutableStateOf(false) }

            AppTheme {
                Surface {
                    LazyVerticalGrid(
                        modifier = Modifier.fillMaxSize(),
                        columns = GridCells.Fixed(2),
                    ) {
                        item {
                            Button(
                                onClick = {
                                    Xmp.initPlayer().also {
                                        toast("Init: $it")
                                    }
                                },
                                content = { Text(text = "Init Player") }
                            )
                        }
                        item {
                            Button(
                                onClick = {
                                    val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
                                        addCategory(Intent.CATEGORY_OPENABLE)
                                        type = "*/*"
                                    }
                                    startActivityForResult(intent, 669)
                                },
                                content = { Text(text = "Load file") }
                            )
                        }
                        item {
                            Button(
                                onClick = { Xmp.deInitPlayer() },
                                content = { Text(text = "De-Init Player") }
                            )
                        }
                        item {
                            Button(
                                onClick = {
                                    Xmp.startModule().also {
                                        toast("Start module: $it")
                                    }
                                },
                                content = { Text(text = "Start Module") }
                            )
                        }
                        item {
                            Button(
                                onClick = { handler.post(runnable) },
                                content = { Text(text = "Play") }
                            )
                        }
                        item {
                            Button(
                                onClick = {
                                    isPaused = Xmp.pause(!isPaused)
                                },
                                content = { Text(text = "Pause: $isPaused") }
                            )
                        }
                        item {
                            Button(
                                onClick = {
                                    isLooping = !isLooping
                                },
                                content = { Text(text = "Loop: $isLooping") }
                            )
                        }
                        item {
                            Button(
                                onClick = { Xmp.stopModule() },
                                content = { Text(text = "Stop Module") }
                            )
                        }
                        item {
                            Button(
                                onClick = { Xmp.restartModule() },
                                content = { Text(text = "Restart Module") }
                            )
                        }
                        item {
                            Button(
                                onClick = { Xmp.releaseModule() },
                                content = { Text(text = "Release Module") }
                            )
                        }
                        item {
                            Button(
                                onClick = { Xmp.endPlayer() },
                                content = { Text(text = "End Player") }
                            )
                        }
                        item {
                            Button(
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = Color.DarkGray,
                                    contentColor = Color.White
                                ),
                                onClick = { Xmp.getModuleName().also(::toast) },
                                content = { Text(text = "Get Module Name") }
                            )
                        }
                        item {
                            Button(
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = Color.DarkGray,
                                    contentColor = Color.White
                                ),
                                onClick = { Xmp.getModuleType().also(::toast) },
                                content = { Text(text = "Get Module Type") }
                            )
                        }
                        item {
                            Button(
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = Color.DarkGray,
                                    contentColor = Color.White
                                ),
                                onClick = {
                                    Xmp.getSupportedFormats()
                                        ?.also {
                                            toast(it.toList().toString())
                                        }
                                },
                                content = { Text(text = "Supported Formats") }
                            )
                        }
                        item {
                            Button(
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = Color.DarkGray,
                                    contentColor = Color.White
                                ),
                                onClick = { Xmp.getVersion().also(::toast) },
                                content = { Text(text = "Get Version") }
                            )
                        }
                        item {
                            Button(
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = Color.DarkGray,
                                    contentColor = Color.White
                                ),
                                onClick = { Xmp.getComment().also(::toast) },
                                content = { Text(text = "Get Comment") }
                            )
                        }
                        item {
                            Button(
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = Color.DarkGray,
                                    contentColor = Color.White
                                ),
                                onClick = {
                                    Xmp.getInstruments().also {
                                        toast(it.orEmpty().toList().toString())
                                    }
                                },
                                content = { Text(text = "Get Instruments") }
                            )
                        }
                        item {
                            Button(
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = Color.DarkGray,
                                    contentColor = Color.White
                                ),
                                onClick = { Xmp.getTime().also { toast(it.toString()) } },
                                content = { Text(text = "Get Time") }
                            )
                        }
                        item {
                            Button(
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = Color.DarkGray,
                                    contentColor = Color.White
                                ),
                                onClick = {
                                    val info = ViewerInfo()
                                    Xmp.getInfo(info.values)
                                    toast(info.toString())
                                },
                                content = { Text(text = "Get Info") }
                            )
                        }
                    }
                }
            }
        }
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == 669 && resultCode == Activity.RESULT_OK) {
            data?.data?.also { uri ->
                val takeFlags = Intent.FLAG_GRANT_READ_URI_PERMISSION or
                    Intent.FLAG_GRANT_WRITE_URI_PERMISSION

                contentResolver.takePersistableUriPermission(uri, takeFlags)

                Log.i("FilePicker", "Selected File URI: $uri")

                with(sharedPref.edit()) {
                    putString("uri", uri.toString())
                    apply()
                }

                thread {
                    val res = Xmp.loadFromFd(uri)
                    toast("Load from fd: $res")
                }
            }
        }
    }
}


@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {
    Text(
        text = "Hello $name!",
        modifier = modifier
    )
}

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    AppTheme {
        Greeting("Android")
    }
}