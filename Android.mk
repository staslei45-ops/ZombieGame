LOCAL_PATH := $(call my-dir)/app/src/main/cpp

include $(CLEAR_VARS)

LOCAL_MODULE    := zombiegame
LOCAL_CPP_EXTENSION := .cpp
LOCAL_SRC_FILES := \
    main.cpp \
    game/Game.cpp \
    renderer/Math.cpp \
    renderer/Renderer.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS  := -std=c++17 -O2 -ffast-math -fno-exceptions -fno-rtti

LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2

include $(BUILD_SHARED_LIBRARY)
