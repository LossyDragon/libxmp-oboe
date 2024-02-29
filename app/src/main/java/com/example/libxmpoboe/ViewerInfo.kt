package com.example.libxmpoboe

class ViewerInfo(
    val finalVols: IntArray = IntArray(64),
    val instruments: IntArray = IntArray(64),
    val keys: IntArray = IntArray(64),
    val pans: IntArray = IntArray(64),
    val periods: IntArray = IntArray(64),
    var time: Int = 0,
    var type: String = "",
    val values: IntArray = IntArray(7), // order pattern row num_rows frame speed bpm
    val volumes: IntArray = IntArray(64)
) {
    override fun toString(): String {
        return "ViewerInfo(" +
            "finalVols=${finalVols.contentToString()}, " +
            "instruments=${instruments.contentToString()}, " +
            "keys=${keys.contentToString()}, " +
            "pans=${pans.contentToString()}, " +
            "periods=${periods.contentToString()}, " +
            "time=$time, " +
            "type='$type', " +
            "values=${values.contentToString()}, " +
            "volumes=${volumes.contentToString()}" +
            ")"
    }
}