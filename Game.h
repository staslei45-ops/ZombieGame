#pragma once
#include <vector>
#include <memory>
#include <string>
#include <cmath>
#include <cstdlib>
#include <android/log.h>

#define LOG_TAG "ZombieGame"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ─── Векторы ───────────────────────────────────────────────────────────────
struct Vec2 { float x, y; };
struct Vec3 {
    float x, y, z;
    Vec3(float x=0,float y=0,float z=0): x(x),y(y),z(z){}
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s)        const { return {x*s, y*s, z*s}; }
    float length() const { return sqrtf(x*x+y*y+z*z); }
    Vec3 normalized() const {
        float l = length();
        return l>0 ? Vec3(x/l,y/l,z/l) : Vec3(0,0,0);
    }
};

// ─── Константы ─────────────────────────────────────────────────────────────
static const int   MAP_SIZE        = 60;      // клетки карты
static const float CELL_SIZE       = 2.0f;
static const float PLAYER_SPEED    = 5.0f;
static const float ZOMBIE_SPEED    = 1.5f;
static const float BULLET_SPEED    = 20.0f;
static const float SHOOT_COOLDOWN  = 0.25f;   // сек
static const int   MAX_ZOMBIES     = 40;
static const int   WAVE_ADD        = 5;       // зомби на волну

// ─── Типы тайлов ───────────────────────────────────────────────────────────
enum TileType { TILE_GRASS=0, TILE_ROAD, TILE_BUILDING, TILE_WALL };

struct Tile {
    TileType type;
    float    height;  // высота стены/здания
};

// ─── Сущности ──────────────────────────────────────────────────────────────
struct Bullet {
    Vec3  pos, dir;
    bool  alive;
    float lifetime;
};

struct Zombie {
    Vec3  pos;
    float hp;
    bool  alive;
    float animTime;
    float attackCooldown;
};

struct Tree {
    Vec3  pos;
    float scale;
};

// ─── Состояние игры ────────────────────────────────────────────────────────
struct GameState {
    // Игрок
    Vec3  playerPos     = {MAP_SIZE*CELL_SIZE/2, 0, MAP_SIZE*CELL_SIZE/2};
    float playerYaw     = 0.0f;   // поворот по Y
    float playerPitch   = 0.0f;
    int   playerHP      = 100;
    float shootTimer    = 0.0f;
    int   ammo          = 9999;   // бесконечные патроны (счётчик анимации)
    int   kills         = 0;
    int   wave          = 1;
    float waveTimer     = 0.0f;

    // Сущности
    std::vector<Zombie> zombies;
    std::vector<Bullet> bullets;
    std::vector<Tree>   trees;

    // Карта
    Tile map[MAP_SIZE][MAP_SIZE];

    bool  gameOver  = false;
    bool  paused    = false;
    float deltaTime = 0.016f;

    // Ввод
    float moveX = 0, moveZ = 0;   // джойстик движения
    bool  fireBtn = false;

    // Тряска камеры
    float camShake = 0.0f;
};

// ─── Функции логики ────────────────────────────────────────────────────────
void gameInit(GameState& gs);
void gameUpdate(GameState& gs, float dt);
void gameShoot(GameState& gs);
void gameReset(GameState& gs);
