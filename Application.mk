# Mi Pad 1 — NVIDIA Tegra K1 (ARMv7-A + NEON)
APP_ABI      := armeabi-v7a
APP_PLATFORM := android-19     # Android 4.4 KitKat minimum
APP_STL      := c++_static
APP_OPTIM    := release
APP_CPPFLAGS := -std=c++17 -mfpu=neon -mfloat-abi=softfp
