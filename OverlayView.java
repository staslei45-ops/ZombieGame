package com.zombiegame;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.view.View;

/**
 * Прозрачный View поверх SurfaceView.
 * Рисует:
 *  - Волна и убийства (текст сверху по центру)
 *  - Джойстик-круг (левый нижний угол)
 *  - Кнопка FIRE (правый нижний угол)
 *  - GAME OVER экран
 */
public class OverlayView extends View {

    private final Paint mTxtPaint  = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint mBgPaint   = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint mFirePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint mJoyPaint  = new Paint(Paint.ANTI_ALIAS_FLAG);

    private int     mHP=100, mWave=1, mKills=0;
    private boolean mGameOver=false;

    public OverlayView(Context ctx) {
        super(ctx);
        setBackgroundColor(Color.TRANSPARENT);

        mTxtPaint.setColor(Color.WHITE);
        mTxtPaint.setTextSize(36);
        mTxtPaint.setTypeface(Typeface.DEFAULT_BOLD);
        mTxtPaint.setShadowLayer(4, 2, 2, Color.BLACK);

        mBgPaint.setColor(Color.argb(120, 0, 0, 0));
        mBgPaint.setStyle(Paint.Style.FILL);

        mFirePaint.setColor(Color.argb(180, 220, 50, 50));
        mFirePaint.setStyle(Paint.Style.FILL);

        mJoyPaint.setColor(Color.argb(80, 255, 255, 255));
        mJoyPaint.setStyle(Paint.Style.STROKE);
        mJoyPaint.setStrokeWidth(3);
    }

    public void setStats(int hp, int wave, int kills, boolean gameOver) {
        mHP=hp; mWave=wave; mKills=kills; mGameOver=gameOver;
    }

    @Override
    protected void onDraw(Canvas c) {
        float w=getWidth(), h=getHeight();

        // Волна и убийства — сверху по центру
        String info = "Волна: "+mWave+"   Убийства: "+mKills;
        float tw = mTxtPaint.measureText(info);
        RectF bg = new RectF(w/2-tw/2-12, 14, w/2+tw/2+12, 60);
        c.drawRoundRect(bg, 10, 10, mBgPaint);
        c.drawText(info, w/2 - tw/2, 50, mTxtPaint);

        // HP — левый верх
        mTxtPaint.setColor(mHP > 40 ? Color.WHITE : Color.RED);
        c.drawText("HP: "+mHP, 20, 90, mTxtPaint);
        mTxtPaint.setColor(Color.WHITE);

        // Кнопка FIRE
        float fireCX = w*0.88f, fireCY = h*0.80f;
        c.drawCircle(fireCX, fireCY, 70, mFirePaint);
        mTxtPaint.setTextSize(28);
        float fireW = mTxtPaint.measureText("ОГОНЬ");
        c.drawText("ОГОНЬ", fireCX - fireW/2, fireCY+10, mTxtPaint);
        mTxtPaint.setTextSize(36);

        // Джойстик
        float jcx = w*0.14f, jcy = h*0.80f;
        c.drawCircle(jcx, jcy, 80, mJoyPaint);
        c.drawCircle(jcx, jcy, 30, mJoyPaint);

        // Game Over
        if(mGameOver) {
            mBgPaint.setColor(Color.argb(160, 0, 0, 0));
            c.drawRect(0, 0, w, h, mBgPaint);
            mBgPaint.setColor(Color.argb(120, 0, 0, 0));

            mTxtPaint.setTextSize(60);
            mTxtPaint.setColor(Color.RED);
            String go = "GAME OVER";
            float gw = mTxtPaint.measureText(go);
            c.drawText(go, w/2-gw/2, h/2-20, mTxtPaint);

            mTxtPaint.setTextSize(32);
            mTxtPaint.setColor(Color.WHITE);
            String sub = "Убийств: "+mKills+"  Волна: "+mWave+"  Тапни для рестарта";
            float sw = mTxtPaint.measureText(sub);
            c.drawText(sub, w/2-sw/2, h/2+40, mTxtPaint);
            mTxtPaint.setTextSize(36);
        }
    }
}
