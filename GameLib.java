package com.zombiegame;

import android.view.Surface;

/**
 * Мост между Java и нативным C++ движком.
 */
public class GameLib {
    public static native void    nativeInit(Surface surface);
    public static native void    nativeDestroy();
    public static native void    nativeResize(int w, int h);
    public static native void    nativeStep();

    public static native void    nativeMove(float x, float z);
    public static native void    nativeLook(float dx, float dy);
    public static native void    nativeShoot(boolean pressed);
    public static native void    nativeRestart();

    public static native int     nativeGetKills();
    public static native int     nativeGetWave();
    public static native int     nativeGetHP();
    public static native boolean nativeIsGameOver();
}
