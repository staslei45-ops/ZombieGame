# 🧟 Zombie Survival — Mi Pad 1 (2014, K1)
## Полное руководство по сборке

---

## Что за игра

3D шутер от первого лица:
- Бесконечные волны зомби (каждая волна +5 зомби)
- Карта 60×60 клеток: трава, дороги, здания в стиле Brick Rigs, деревья
- Виртуальный джойстик (движение) + свайп правой рукой (взгляд) + кнопка ОГОНЬ
- HP бар игрока, HP бары над зомби, миникарта
- Бесконечные патроны, система волн

---

## Требования

| Что | Версия |
|-----|--------|
| Android Studio | 4.2+ или Bumblebee+ |
| Android NDK | r21e (21.4.7075529) |
| CMake | 3.18.1 |
| JDK | 8 или 11 |
| Android SDK | API 19+ (KitKat) |

Mi Pad 1 2014 K1 — **NVIDIA Tegra K1**, ARMv7-A + NEON, Android 4.4–5.x
Проект собирается под `armeabi-v7a`.

---

## Шаг 1 — Установить Android Studio

1. Скачай Android Studio: https://developer.android.com/studio
2. При установке включи компоненты:
   - Android SDK
   - Android SDK Platform 28
   - NDK (Side by Side) → выбери версию **21.4.7075529**
   - CMake **3.18.1**

---

## Шаг 2 — Настроить local.properties

Открой файл `ZombieGame/local.properties` и укажи пути:

**Windows:**
```
sdk.dir=C\:\\Users\\ИМЯ\\AppData\\Local\\Android\\Sdk
ndk.dir=C\:\\Users\\ИМЯ\\AppData\\Local\\Android\\Sdk\\ndk\\21.4.7075529
```

**Linux/Mac:**
```
sdk.dir=/home/ИМЯ/Android/Sdk
ndk.dir=/home/ИМЯ/Android/Sdk/ndk/21.4.7075529
```

---

## Шаг 3 — Открыть проект в Android Studio

1. File → Open → выбрать папку `ZombieGame`
2. Подождать синхронизацию Gradle (первый раз долго — скачивает зависимости)
3. Если Android Studio предложит обновить Gradle — соглашайся

---

## Шаг 4 — Добавить иконку (обязательно, иначе ошибка сборки)

Создай папку `app/src/main/res/mipmap-mdpi/` и положи туда файл `ic_launcher.png` (48×48 px).
Либо: в Android Studio → правой кнопкой на `res` → New → Image Asset → Launcher Icons.

---

## Шаг 5 — Включить отладку по USB на Mi Pad 1

1. Настройки → О планшете → тапни **7 раз** на "Номер сборки"
2. Настройки → Для разработчиков → **Отладка USB** → Включить
3. Подключи Mi Pad 1 к компьютеру кабелем
4. На планшете появится запрос "Разрешить отладку?" → **Разрешить**

---

## Шаг 6 — Сборка и запуск

### Вариант A — Через Android Studio (проще)

1. В верхней панели выбери устройство: **Mi Pad (твой планшет)**
2. Нажми зелёную кнопку ▶ **Run**
3. Gradle автоматически:
   - Скомпилирует C++ через CMake + NDK
   - Соберёт APK
   - Установит на планшет
   - Запустит игру

### Вариант B — Командная строка

```bash
# Из папки ZombieGame/
./gradlew assembleRelease        # собрать Release APK

# Установить на подключённый Mi Pad:
adb install app/build/outputs/apk/release/app-release-unsigned.apk

# Запустить:
adb shell am start -n com.zombiegame/.MainActivity
```

### Вариант C — Только NDK (без Gradle, для рутованных устройств)

```bash
cd ZombieGame/app/src/main/jni

# Собрать .so библиотеку:
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk

# Результат: libs/armeabi-v7a/libzombiegame.so
```

---

## Если есть ROOT на Mi Pad 1

С рутом можно запустить игру напрямую через нативный Activity без подписи APK:

```bash
# Скопировать .so на устройство
adb push libs/armeabi-v7a/libzombiegame.so /data/local/tmp/

# Проверить что библиотека рабочая:
adb shell "/system/bin/linker /data/local/tmp/libzombiegame.so"
```

Для полноценного запуска всё равно нужен APK — root упрощает установку без Play Store:
```bash
# Установить без проверки подписи (нужен root):
adb install -r app-release-unsigned.apk
# или
adb shell pm install /sdcard/game.apk
```

---

## Структура проекта

```
ZombieGame/
├── CMakeLists.txt                      ← CMake конфиг (NDK сборка)
├── app/
│   ├── build.gradle                    ← Android конфиг
│   └── src/main/
│       ├── AndroidManifest.xml
│       ├── cpp/
│       │   ├── main.cpp               ← JNI точка входа
│       │   ├── game/
│       │   │   ├── Game.h             ← структуры данных, константы
│       │   │   └── Game.cpp           ← логика игры, волны, ИИ зомби
│       │   └── renderer/
│       │       ├── Renderer.h         ← заголовок рендерера
│       │       ├── Math.cpp           ← матрицы, шейдеры, меши
│       │       └── Renderer.cpp       ← весь OpenGL ES 2.0 рендеринг
│       ├── jni/
│       │   ├── Android.mk             ← альтернативный ndk-build конфиг
│       │   └── Application.mk         ← ABI и флаги компилятора
│       └── java/com/zombiegame/
│           ├── MainActivity.java      ← Activity, тач-управление
│           ├── OverlayView.java       ← HUD поверх OpenGL
│           └── GameLib.java           ← JNI декларации
```

---

## Управление в игре

| Зона экрана | Действие |
|-------------|----------|
| Левый нижний круг | Джойстик движения |
| Правая половина (свайп) | Поворот камеры |
| Кнопка ОГОНЬ (правый нижний) | Стрельба |
| Тап при GAME OVER | Рестарт |

---

## Производительность на Mi Pad 1

Tegra K1 тянет этот движок на 40–60 FPS при:
- 40 зомби
- 60×60 карта
- OpenGL ES 2.0

Если FPS низкий — уменьши `MAP_SIZE` в `Game.h` с 60 до 40.

---

## Возможные ошибки

**`NDK не найден`** → укажи правильный путь в `local.properties`

**`CMake 3.18.1 not found`** → SDK Manager → SDK Tools → CMake → 3.18.1

**`INSTALL_FAILED_CPU_ABI_INCOMPATIBLE`** → проверь что в `build.gradle` стоит `armeabi-v7a`

**`Surface destroyed`** → нормально при сворачивании приложения, логика в onPause/onResume

**Белый экран** → смотри `adb logcat | grep ZombieGame` — там будут ошибки OpenGL
