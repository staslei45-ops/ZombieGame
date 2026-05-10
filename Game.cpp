#include "Game.h"
#include <cstring>
#include <ctime>

// ─── Генерация карты ───────────────────────────────────────────────────────
static void generateMap(GameState& gs) {
    srand(42);
    // Базово — трава
    for(int z=0;z<MAP_SIZE;z++)
        for(int x=0;x<MAP_SIZE;x++)
            gs.map[x][z] = { TILE_GRASS, 0.0f };

    // Дороги (горизонтальные и вертикальные)
    for(int i=0;i<MAP_SIZE;i++) {
        gs.map[i][MAP_SIZE/3]   = { TILE_ROAD, 0 };
        gs.map[i][MAP_SIZE*2/3] = { TILE_ROAD, 0 };
        gs.map[MAP_SIZE/3][i]   = { TILE_ROAD, 0 };
        gs.map[MAP_SIZE*2/3][i] = { TILE_ROAD, 0 };
    }

    // Здания (блоки 3×3 .. 5×5) — стиль Brick Rigs
    struct Building { int x,z,w,h; float ht; };
    Building buildings[] = {
        {3,3,5,5,4},{12,3,4,4,3},{22,3,6,5,5},{32,3,4,6,6},{42,3,5,4,3},
        {3,12,4,5,5},{22,12,5,5,7},{42,12,4,4,4},
        {3,22,6,4,3},{14,22,5,5,5},{32,22,4,6,6},{44,22,5,5,4},
        {3,42,5,5,3},{22,42,6,4,5},{38,42,4,5,4},
        {8,48,5,4,6},{28,48,4,5,3},{44,48,5,5,5}
    };
    for(auto& b : buildings)
        for(int dz=0;dz<b.h;dz++)
            for(int dx=0;dx<b.w;dx++)
                if(b.x+dx<MAP_SIZE && b.z+dz<MAP_SIZE)
                    gs.map[b.x+dx][b.z+dz] = { TILE_BUILDING, b.ht };

    // Деревья — избегаем дороги и здания
    gs.trees.clear();
    for(int i=0;i<80;i++) {
        int tx = rand()%MAP_SIZE;
        int tz = rand()%MAP_SIZE;
        if(gs.map[tx][tz].type == TILE_GRASS) {
            Tree t;
            t.pos   = { tx*CELL_SIZE + CELL_SIZE/2, 0, tz*CELL_SIZE + CELL_SIZE/2 };
            t.scale = 0.8f + (rand()%5)*0.1f;
            gs.trees.push_back(t);
            gs.map[tx][tz] = { TILE_WALL, t.scale*3.5f }; // дерево блокирует
        }
    }
}

// ─── Спавн зомби ──────────────────────────────────────────────────────────
static void spawnZombie(GameState& gs) {
    // Спавн по периметру карты
    Zombie z;
    int side = rand()%4;
    float edge = (MAP_SIZE-1)*CELL_SIZE;
    switch(side) {
        case 0: z.pos = {(float)(rand()%(int)edge), 0, 0};     break;
        case 1: z.pos = {(float)(rand()%(int)edge), 0, edge};  break;
        case 2: z.pos = {0, 0, (float)(rand()%(int)edge)};     break;
        case 3: z.pos = {edge, 0, (float)(rand()%(int)edge)};  break;
    }
    z.hp = 100.0f + gs.wave*10;
    z.alive = true;
    z.animTime = (float)(rand()%100)/100.0f;
    z.attackCooldown = 0;
    gs.zombies.push_back(z);
}

// ─── Инициализация ────────────────────────────────────────────────────────
void gameInit(GameState& gs) {
    gs = GameState{};
    generateMap(gs);
    // Первая волна
    for(int i=0;i<5;i++) spawnZombie(gs);
}

void gameReset(GameState& gs) { gameInit(gs); }

// ─── Выстрел ──────────────────────────────────────────────────────────────
void gameShoot(GameState& gs) {
    if(gs.shootTimer > 0) return;
    gs.shootTimer = SHOOT_COOLDOWN;
    gs.camShake = 0.15f;

    float yaw = gs.playerYaw;
    float pitch = gs.playerPitch;
    Bullet b;
    b.pos = gs.playerPos + Vec3(0, 1.6f, 0);
    b.dir = Vec3(
        -sinf(yaw)*cosf(pitch),
         sinf(pitch),
        -cosf(yaw)*cosf(pitch)
    );
    b.alive = true;
    b.lifetime = 3.0f;
    gs.bullets.push_back(b);
}

// ─── Коллизия с картой ────────────────────────────────────────────────────
static bool isSolid(GameState& gs, float wx, float wz) {
    int tx = (int)(wx / CELL_SIZE);
    int tz = (int)(wz / CELL_SIZE);
    if(tx<0||tz<0||tx>=MAP_SIZE||tz>=MAP_SIZE) return true;
    TileType t = gs.map[tx][tz].type;
    return t==TILE_BUILDING || t==TILE_WALL;
}

// ─── Обновление ───────────────────────────────────────────────────────────
void gameUpdate(GameState& gs, float dt) {
    if(gs.gameOver || gs.paused) return;

    gs.deltaTime = dt;
    gs.shootTimer   -= dt;
    gs.camShake     -= dt; if(gs.camShake<0) gs.camShake=0;

    // ── Движение игрока ──
    float s = sinf(gs.playerYaw), c = cosf(gs.playerYaw);
    Vec3 fwd = {-s, 0, -c};
    Vec3 right = {c, 0, -s};
    Vec3 move = fwd*gs.moveZ + right*gs.moveX;
    float ml = move.length();
    if(ml > 0.01f) {
        move = move.normalized() * PLAYER_SPEED * dt;
        Vec3 np = gs.playerPos + move;
        if(!isSolid(gs, np.x, gs.playerPos.z)) gs.playerPos.x = np.x;
        if(!isSolid(gs, gs.playerPos.x, np.z)) gs.playerPos.z = np.z;
        // Границы карты
        float maxW = (MAP_SIZE-1)*CELL_SIZE;
        if(gs.playerPos.x<0.5f) gs.playerPos.x=0.5f;
        if(gs.playerPos.z<0.5f) gs.playerPos.z=0.5f;
        if(gs.playerPos.x>maxW-0.5f) gs.playerPos.x=maxW-0.5f;
        if(gs.playerPos.z>maxW-0.5f) gs.playerPos.z=maxW-0.5f;
    }

    // Автострельба
    if(gs.fireBtn) gameShoot(gs);

    // ── Пули ──
    for(auto& b : gs.bullets) {
        if(!b.alive) continue;
        b.lifetime -= dt;
        if(b.lifetime <= 0) { b.alive=false; continue; }
        b.pos = b.pos + b.dir * (BULLET_SPEED * dt);
        if(isSolid(gs, b.pos.x, b.pos.z)) { b.alive=false; continue; }
        // Попадание в зомби
        for(auto& z : gs.zombies) {
            if(!z.alive) continue;
            Vec3 d = b.pos - z.pos;
            d.y = 0;
            if(d.length() < 0.6f) {
                z.hp -= 34.0f;
                b.alive = false;
                if(z.hp <= 0) { z.alive=false; gs.kills++; }
                break;
            }
        }
    }

    // ── Зомби ──
    for(auto& z : gs.zombies) {
        if(!z.alive) continue;
        z.animTime += dt * 3.0f;
        z.attackCooldown -= dt;

        Vec3 toPlayer = gs.playerPos - z.pos;
        toPlayer.y = 0;
        float dist = toPlayer.length();

        if(dist > 0.1f) {
            Vec3 dir = toPlayer.normalized();
            Vec3 np  = z.pos + dir * (ZOMBIE_SPEED * dt);
            if(!isSolid(gs, np.x, z.pos.z)) z.pos.x = np.x;
            if(!isSolid(gs, z.pos.x, np.z)) z.pos.z = np.z;
        }

        // Атака
        if(dist < 1.2f && z.attackCooldown <= 0) {
            gs.playerHP -= 10;
            z.attackCooldown = 1.0f;
            gs.camShake = 0.3f;
            if(gs.playerHP <= 0) { gs.gameOver = true; return; }
        }
    }

    // ── Волны ──
    int alive = 0;
    for(auto& z : gs.zombies) if(z.alive) alive++;
    if(alive == 0) {
        gs.wave++;
        int toSpawn = 5 + gs.wave * WAVE_ADD;
        for(int i=0;i<toSpawn;i++) spawnZombie(gs);
    }
}
