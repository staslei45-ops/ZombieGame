#include <jni.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <chrono>
#include <cstring>

#include "game/Game.h"
#include "renderer/Renderer.h"

// ─── Глобальное состояние ──────────────────────────────────────────────────
static GameState    gGame;
static Renderer     gRenderer;
static bool         gInitialized = false;

static EGLDisplay   gDisplay = EGL_NO_DISPLAY;
static EGLSurface   gSurface = EGL_NO_SURFACE;
static EGLContext   gContext  = EGL_NO_CONTEXT;
static ANativeWindow* gWindow  = nullptr;

static auto gLastTime = std::chrono::steady_clock::now();

// ─── EGL инициализация ─────────────────────────────────────────────────────
static bool eglInit(ANativeWindow* window) {
    gDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(gDisplay, nullptr, nullptr);

    const EGLint cfg_attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,   8, EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8, EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    EGLConfig config; EGLint numCfg;
    eglChooseConfig(gDisplay, cfg_attrs, &config, 1, &numCfg);

    gSurface = eglCreateWindowSurface(gDisplay, config, window, nullptr);

    const EGLint ctx_attrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    gContext = eglCreateContext(gDisplay, config, EGL_NO_CONTEXT, ctx_attrs);

    if(eglMakeCurrent(gDisplay, gSurface, gSurface, gContext) == EGL_FALSE) {
        LOGE("eglMakeCurrent failed");
        return false;
    }
    return true;
}

static void eglDestroy() {
    if(gDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(gDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if(gContext != EGL_NO_CONTEXT) eglDestroyContext(gDisplay, gContext);
        if(gSurface != EGL_NO_SURFACE) eglDestroySurface(gDisplay, gSurface);
        eglTerminate(gDisplay);
    }
    gDisplay = EGL_NO_DISPLAY;
    gContext = EGL_NO_CONTEXT;
    gSurface = EGL_NO_SURFACE;
}

// ─── JNI функции ──────────────────────────────────────────────────────────
extern "C" {

JNIEXPORT void JNICALL
Java_com_zombiegame_GameLib_nativeInit(JNIEnv* env, jclass, jobject surface) {
    gWindow = ANativeWindow_fromSurface(env, surface);
    if(!eglInit(gWindow)) return;

    EGLint w, h;
    eglQuerySurface(gDisplay, gSurface, EGL_WIDTH, &w);
    eglQuerySurface(gDisplay, gSurface, EGL_HEIGHT, &h);

    gameInit(gGame);
    gRenderer.init(w, h);
    gInitialized = true;
    gLastTime = std::chrono::steady_clock::now();
    LOGI("Game initialized %dx%d", w, h);
}

JNIEXPORT void JNICALL
Java_com_zombiegame_GameLib_nativeDestroy(JNIEnv*, jclass) {
    gRenderer.destroy();
    eglDestroy();
    if(gWindow) { ANativeWindow_release(gWindow); gWindow=nullptr; }
    gInitialized = false;
}

JNIEXPORT void JNICALL
Java_com_zombiegame_GameLib_nativeResize(JNIEnv*, jclass, jint w, jint h) {
    if(gInitialized) gRenderer.resize(w, h);
}

JNIEXPORT void JNICALL
Java_com_zombiegame_GameLib_nativeStep(JNIEnv*, jclass) {
    if(!gInitialized) return;

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - gLastTime).count();
    gLastTime = now;
    if(dt > 0.05f) dt = 0.05f; // clamp

    gameUpdate(gGame, dt);
    gRenderer.render(gGame);
    eglSwapBuffers(gDisplay, gSurface);
}

// ─── Управление ────────────────────────────────────────────────────────────

// Джойстик движения (нормализованный, -1..1)
JNIEXPORT void JNICALL
Java_com_zombiegame_GameLib_nativeMove(JNIEnv*, jclass, jfloat x, jfloat z) {
    gGame.moveX = x;
    gGame.moveZ = z;
}

// Джойстик камеры / свайп
JNIEXPORT void JNICALL
Java_com_zombiegame_GameLib_nativeLook(JNIEnv*, jclass, jfloat dx, jfloat dy) {
    gGame.playerYaw   -= dx * 0.003f;
    gGame.playerPitch += dy * 0.003f;
    if(gGame.playerPitch >  1.2f) gGame.playerPitch =  1.2f;
    if(gGame.playerPitch < -1.2f) gGame.playerPitch = -1.2f;
}

// Кнопка стрельбы
JNIEXPORT void JNICALL
Java_com_zombiegame_GameLib_nativeShoot(JNIEnv*, jclass, jboolean pressed) {
    gGame.fireBtn = pressed;
}

// Рестарт
JNIEXPORT void JNICALL
Java_com_zombiegame_GameLib_nativeRestart(JNIEnv*, jclass) {
    gameReset(gGame);
}

// Получить счёт
JNIEXPORT jint JNICALL
Java_com_zombiegame_GameLib_nativeGetKills(JNIEnv*, jclass) {
    return gGame.kills;
}

JNIEXPORT jint JNICALL
Java_com_zombiegame_GameLib_nativeGetWave(JNIEnv*, jclass) {
    return gGame.wave;
}

JNIEXPORT jint JNICALL
Java_com_zombiegame_GameLib_nativeGetHP(JNIEnv*, jclass) {
    return gGame.playerHP;
}

JNIEXPORT jboolean JNICALL
Java_com_zombiegame_GameLib_nativeIsGameOver(JNIEnv*, jclass) {
    return gGame.gameOver ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"
