package com.zombiegame;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.view.MotionEvent;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Color;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.os.Handler;
import android.os.Looper;

public class MainActivity extends Activity implements SurfaceHolder.Callback {

    private SurfaceView mSurfaceView;
    private OverlayView mOverlay;
    private Handler     mHandler;
    private Runnable    mGameLoop;
    private boolean     mRunning = false;

    // Джойстики
    private float mMoveX=0, mMoveZ=0;   // левый джойстик
    private float mLookStartX=0, mLookStartY=0; // правый свайп
    private int   mLookPointerId = -1;
    private boolean mShooting = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Полный экран
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        android.widget.FrameLayout frame = new android.widget.FrameLayout(this);
        mSurfaceView = new SurfaceView(this);
        mSurfaceView.getHolder().addCallback(this);
        frame.addView(mSurfaceView);

        mOverlay = new OverlayView(this);
        frame.addView(mOverlay);

        setContentView(frame);
        mHandler = new Handler(Looper.getMainLooper());
    }

    // ── Игровой цикл ────────────────────────────────────────────────────
    private void startLoop() {
        mRunning = true;
        mGameLoop = new Runnable() {
            @Override public void run() {
                if(!mRunning) return;
                GameLib.nativeStep();
                // Обновить оверлей
                mOverlay.setStats(
                    GameLib.nativeGetHP(),
                    GameLib.nativeGetWave(),
                    GameLib.nativeGetKills(),
                    GameLib.nativeIsGameOver()
                );
                mOverlay.postInvalidate();
                mHandler.postDelayed(this, 16); // ~60 fps
            }
        };
        mHandler.post(mGameLoop);
    }

    private void stopLoop() {
        mRunning = false;
        if(mGameLoop!=null) mHandler.removeCallbacks(mGameLoop);
    }

    // ── SurfaceHolder.Callback ──────────────────────────────────────────
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        GameLib.nativeInit(holder.getSurface());
        startLoop();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int fmt, int w, int h) {
        GameLib.nativeResize(w, h);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopLoop();
        GameLib.nativeDestroy();
    }

    // ── Touch ───────────────────────────────────────────────────────────
    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        int action = ev.getActionMasked();
        int idx    = ev.getActionIndex();
        int pid    = ev.getPointerId(idx);
        float x = ev.getX(idx), y = ev.getY(idx);
        float sw = mSurfaceView.getWidth(), sh = mSurfaceView.getHeight();

        // Кнопка стрельбы (правый нижний угол)
        boolean onFireBtn = (x > sw*0.75f && y > sh*0.7f);

        // Левая половина — движение
        boolean onLeft = (x < sw*0.45f);

        if(action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            if(onFireBtn) {
                mShooting = true;
                GameLib.nativeShoot(true);
            } else if(onLeft) {
                // Виртуальный джойстик
            } else {
                // Правая часть — взгляд
                mLookStartX = x; mLookStartY = y;
                mLookPointerId = pid;
            }
        }

        if(action == MotionEvent.ACTION_MOVE) {
            for(int i=0;i<ev.getPointerCount();i++) {
                float px = ev.getX(i), py = ev.getY(i);
                int ppid = ev.getPointerId(i);
                if(ppid == mLookPointerId) {
                    float dx = px - mLookStartX;
                    float dy = py - mLookStartY;
                    GameLib.nativeLook(dx, dy);
                    mLookStartX = px; mLookStartY = py;
                }
                // Движение (левая часть)
                if(px < sw*0.45f) {
                    // Центр джойстика фиксируется при первом касании
                    // Упрощённо: направление = смещение от центра левой части
                    float jcx = sw*0.15f, jcy = sh*0.75f;
                    float jdx = (px - jcx) / (sw*0.15f);
                    float jdy = (py - jcy) / (sh*0.15f);
                    if(jdx >  1) jdx= 1; if(jdx< -1) jdx=-1;
                    if(jdy >  1) jdy= 1; if(jdy< -1) jdy=-1;
                    mMoveX = jdx; mMoveZ = jdy;
                    GameLib.nativeMove(mMoveX, mMoveZ);
                }
            }
        }

        if(action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
            if(pid == mLookPointerId) mLookPointerId = -1;
            if(onFireBtn) { mShooting=false; GameLib.nativeShoot(false); }
            if(onLeft) {
                mMoveX=0; mMoveZ=0; GameLib.nativeMove(0,0);
            }
            // Game over — рестарт по тапу
            if(GameLib.nativeIsGameOver()) GameLib.nativeRestart();
        }
        return true;
    }

    @Override
    protected void onPause()  { super.onPause();  stopLoop(); }
    @Override
    protected void onResume() { super.onResume(); if(mSurfaceView.getHolder().getSurface().isValid()) startLoop(); }

    // ── Загрузка библиотеки ─────────────────────────────────────────────
    static { System.loadLibrary("zombiegame"); }
}
