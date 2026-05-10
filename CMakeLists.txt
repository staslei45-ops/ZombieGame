cmake_minimum_required(VERSION 3.18)
project(ZombieGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Оптимизации для ARM (Mi Pad 1 — Tegra K1, armv7a)
add_compile_options(
    -O2
    -ffast-math
    -fno-exceptions
    -fno-rtti
)

# Список исходников
add_library(
    zombiegame
    SHARED

    app/src/main/cpp/main.cpp
    app/src/main/cpp/game/Game.cpp
    app/src/main/cpp/renderer/Math.cpp
    app/src/main/cpp/renderer/Renderer.cpp
)

# Заголовки
target_include_directories(zombiegame PRIVATE
    app/src/main/cpp
)

# Android библиотеки
find_library(log-lib    log)
find_library(android-lib android)
find_library(egl-lib    EGL)
find_library(gles2-lib  GLESv2)

target_link_libraries(
    zombiegame
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${gles2-lib}
)
