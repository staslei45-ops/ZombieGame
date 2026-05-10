#include "Renderer.h"
#include <cstring>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════════════
// GLSL шейдеры (встроенные строки)
// ═══════════════════════════════════════════════════════════════════════════

// ── Универсальный шейдер освещения ─────────────────────────────────────
static const char* VS_LIT = R"(
attribute vec3 aPos;
attribute vec3 aNorm;
uniform mat4 uMVP;
uniform mat4 uModel;
varying vec3 vNorm;
varying vec3 vWorldPos;
void main(){
    gl_Position = uMVP * vec4(aPos,1.0);
    vNorm = mat3(uModel) * aNorm;
    vWorldPos = (uModel * vec4(aPos,1.0)).xyz;
}
)";

static const char* FS_LIT = R"(
precision mediump float;
varying vec3 vNorm;
varying vec3 vWorldPos;
uniform vec4 uColor;
uniform vec3 uLightDir;
uniform float uAmbient;
void main(){
    float d = max(dot(normalize(vNorm), normalize(uLightDir)), 0.0);
    float light = uAmbient + (1.0-uAmbient)*d;
    gl_FragColor = vec4(uColor.rgb * light, uColor.a);
}
)";

// ── Плоский (HUD, пули) ─────────────────────────────────────────────────
static const char* VS_FLAT = R"(
attribute vec3 aPos;
attribute vec3 aNorm;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos,1.0); }
)";

static const char* FS_FLAT = R"(
precision mediump float;
uniform vec4 uColor;
void main(){ gl_FragColor = uColor; }
)";

// ── HUD 2D шейдер ───────────────────────────────────────────────────────
static const char* VS_HUD = R"(
attribute vec2 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos, 0.0, 1.0); }
)";

static const char* FS_HUD = R"(
precision mediump float;
uniform vec4 uColor;
void main(){ gl_FragColor = uColor; }
)";

// ═══════════════════════════════════════════════════════════════════════════
// Геометрия
// ═══════════════════════════════════════════════════════════════════════════

// Куб: pos(3)+norm(3) × 24 вершины, 36 индексов
static const float CUBE_VERTS[] = {
    // -Z
    -0.5f,-0.5f,-0.5f, 0,0,-1,  0.5f,-0.5f,-0.5f,0,0,-1,
     0.5f, 0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f,0,0,-1,
    // +Z
    -0.5f,-0.5f, 0.5f, 0,0,1,   0.5f,-0.5f, 0.5f,0,0,1,
     0.5f, 0.5f, 0.5f, 0,0,1,  -0.5f, 0.5f, 0.5f,0,0,1,
    // -X
    -0.5f,-0.5f,-0.5f,-1,0,0,  -0.5f,-0.5f, 0.5f,-1,0,0,
    -0.5f, 0.5f, 0.5f,-1,0,0,  -0.5f, 0.5f,-0.5f,-1,0,0,
    // +X
     0.5f,-0.5f,-0.5f, 1,0,0,   0.5f,-0.5f, 0.5f,1,0,0,
     0.5f, 0.5f, 0.5f, 1,0,0,   0.5f, 0.5f,-0.5f,1,0,0,
    // -Y
    -0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f,-0.5f,0,-1,0,
     0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f, 0.5f,0,-1,0,
    // +Y
    -0.5f, 0.5f,-0.5f, 0,1,0,   0.5f, 0.5f,-0.5f,0,1,0,
     0.5f, 0.5f, 0.5f, 0,1,0,  -0.5f, 0.5f, 0.5f,0,1,0
};
static const unsigned short CUBE_IDX[] = {
    0,1,2,0,2,3,  4,5,6,4,6,7,  8,9,10,8,10,11,
    12,13,14,12,14,15, 16,17,18,16,18,19, 20,21,22,20,22,23
};

// Квадрат XZ плоскость
static const float QUAD_VERTS[] = {
    -0.5f,0,-0.5f, 0,1,0,
     0.5f,0,-0.5f, 0,1,0,
     0.5f,0, 0.5f, 0,1,0,
    -0.5f,0, 0.5f, 0,1,0
};
static const unsigned short QUAD_IDX[] = { 0,1,2, 0,2,3 };

// Простой цилиндр (8 секций)
static void buildCylinder(std::vector<float>& verts, std::vector<unsigned short>& idx,
                           float radius, float height, int segs) {
    // Боковые грани
    for(int i=0;i<segs;i++){
        float a0 = (float)i/segs*6.2832f;
        float a1 = (float)(i+1)/segs*6.2832f;
        float x0=cosf(a0)*radius, z0=sinf(a0)*radius;
        float x1=cosf(a1)*radius, z1=sinf(a1)*radius;
        float nx=(x0+x1)*0.5f, nz=(z0+z1)*0.5f;
        int base = (int)verts.size()/6;
        // 4 вершины
        auto push=[&](float x,float y,float z,float nxx,float nzz){
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
            verts.push_back(nxx); verts.push_back(0); verts.push_back(nzz);
        };
        push(x0,0,z0,nx,nz); push(x1,0,z1,nx,nz);
        push(x1,height,z1,nx,nz); push(x0,height,z0,nx,nz);
        idx.push_back(base); idx.push_back(base+1); idx.push_back(base+2);
        idx.push_back(base); idx.push_back(base+2); idx.push_back(base+3);
    }
}

static void buildCone(std::vector<float>& verts, std::vector<unsigned short>& idx,
                      float radius, float height, int segs) {
    int apex = (int)verts.size()/6;
    verts.push_back(0); verts.push_back(height); verts.push_back(0);
    verts.push_back(0); verts.push_back(1); verts.push_back(0);
    for(int i=0;i<segs;i++){
        float a0=(float)i/segs*6.2832f, a1=(float)(i+1)/segs*6.2832f;
        float x0=cosf(a0)*radius, z0=sinf(a0)*radius;
        float x1=cosf(a1)*radius, z1=sinf(a1)*radius;
        int b = (int)verts.size()/6;
        float nx=(x0+x1)*0.5f,nz=(z0+z1)*0.5f;
        verts.push_back(x0);verts.push_back(0);verts.push_back(z0);
        verts.push_back(nx);verts.push_back(0.5f);verts.push_back(nz);
        verts.push_back(x1);verts.push_back(0);verts.push_back(z1);
        verts.push_back(nx);verts.push_back(0.5f);verts.push_back(nz);
        idx.push_back(apex); idx.push_back(b); idx.push_back(b+1);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Renderer
// ═══════════════════════════════════════════════════════════════════════════

bool Renderer::init(int w, int h) {
    mW = w; mH = h;
    glViewport(0, 0, w, h);
    glClearColor(0.4f, 0.6f, 0.9f, 1.0f); // голубое небо
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Шейдеры
    mGroundShader.compile(VS_LIT, FS_LIT);
    mBuildingShader.compile(VS_LIT, FS_LIT);
    mZombieShader.compile(VS_LIT, FS_LIT);
    mBulletShader.compile(VS_FLAT, FS_FLAT);
    mTreeShader.compile(VS_LIT, FS_LIT);
    mHudShader.compile(VS_HUD, FS_HUD);

    buildMeshes();
    mProj = Mat4::perspective(1.0472f, (float)w/h, 0.1f, 200.0f);
    LOGI("Renderer init %dx%d", w, h);
    return true;
}

void Renderer::resize(int w, int h) {
    mW=w; mH=h;
    glViewport(0,0,w,h);
    mProj = Mat4::perspective(1.0472f, (float)w/h, 0.1f, 200.0f);
}

void Renderer::buildMeshes() {
    mQuadMesh.create(QUAD_VERTS, sizeof(QUAD_VERTS), QUAD_IDX, 6);
    mCubeMesh.create(CUBE_VERTS, sizeof(CUBE_VERTS), CUBE_IDX, 36);

    std::vector<float> cv; std::vector<unsigned short> ci;
    buildCylinder(cv, ci, 0.15f, 1.0f, 8);
    mCylinderMesh.create(cv.data(), cv.size()*4, ci.data(), ci.size());

    std::vector<float> conev; std::vector<unsigned short> conei;
    buildCone(conev, conei, 0.6f, 1.2f, 8);
    mConeMesh.create(conev.data(), conev.size()*4, conei.data(), conei.size());
}

Mat4 Renderer::getView(const GameState& gs) const {
    float shake = 0;
    if(gs.camShake > 0) {
        shake = sinf(gs.camShake * 40.0f) * gs.camShake * 0.08f;
    }
    Vec3 eye = gs.playerPos + Vec3(0, 1.65f, 0);
    float yaw   = gs.playerYaw;
    float pitch = gs.playerPitch + shake;
    Vec3 target = eye + Vec3(-sinf(yaw)*cosf(pitch), sinf(pitch), -cosf(yaw)*cosf(pitch));
    return Mat4::lookAt(eye, target, {0,1,0});
}

void Renderer::drawMesh(const Mesh& mesh, const Shader& shader,
                         const Mat4& mvp, float r, float g, float b, float a) {
    shader.use();
    shader.setMat4("uMVP", mvp);
    shader.setVec4("uColor", r, g, b, a);
    mesh.draw();
}

// ─── Земля ─────────────────────────────────────────────────────────────────
void Renderer::renderGround(const GameState& gs, const Mat4& vp) {
    mGroundShader.use();
    mGroundShader.setVec3("uLightDir", 0.5f, 1.0f, 0.3f);
    mGroundShader.setFloat("uAmbient", 0.4f);

    float cs = CELL_SIZE;
    for(int z=0;z<MAP_SIZE;z++) {
        for(int x=0;x<MAP_SIZE;x++) {
            float wx = x*cs + cs/2;
            float wz = z*cs + cs/2;
            TileType t = gs.map[x][z].type;

            float r=0.3f,g2=0.6f,b=0.2f; // трава
            if(t==TILE_ROAD)     { r=0.45f;g2=0.45f;b=0.45f; }
            if(t==TILE_BUILDING) continue; // рисуем зданиями

            Mat4 model = Mat4::translate({wx,0,wz}) * Mat4::scale({cs,1,cs});
            Mat4 mvp = mProj * vp * model;
            mGroundShader.setMat4("uMVP", mvp);
            mGroundShader.setMat4("uModel", model);
            mGroundShader.setVec4("uColor", r, g2, b, 1.0f);
            mQuadMesh.draw();
        }
    }
}

// ─── Здания (стиль Brick Rigs — разноцветные кубы) ─────────────────────
void Renderer::renderBuildings(const GameState& gs, const Mat4& vp) {
    mBuildingShader.use();
    mBuildingShader.setVec3("uLightDir", 0.5f, 1.0f, 0.3f);
    mBuildingShader.setFloat("uAmbient", 0.35f);

    // Палитра кирпичных цветов
    static const float colors[][3] = {
        {0.72f,0.36f,0.24f},{0.8f,0.5f,0.3f},{0.6f,0.3f,0.2f},
        {0.9f,0.8f,0.7f},{0.65f,0.4f,0.3f},{0.5f,0.5f,0.55f}
    };
    float cs = CELL_SIZE;
    for(int z=0;z<MAP_SIZE;z++) {
        for(int x=0;x<MAP_SIZE;x++) {
            Tile& t = gs.map[x][z];
            if(t.type != TILE_BUILDING) continue;
            float wx = x*cs + cs/2;
            float wz = z*cs + cs/2;
            float ht = t.height;

            // Основной блок здания
            int ci = ((x*3+z*7)%6);
            Mat4 model = Mat4::translate({wx, ht/2, wz}) * Mat4::scale({cs, ht, cs});
            Mat4 mvp = mProj * vp * model;
            mBuildingShader.setMat4("uMVP", mvp);
            mBuildingShader.setMat4("uModel", model);
            mBuildingShader.setVec4("uColor", colors[ci][0], colors[ci][1], colors[ci][2], 1.0f);
            mCubeMesh.draw();

            // Крыша — немного светлее
            Mat4 roof = Mat4::translate({wx, ht+0.1f, wz}) * Mat4::scale({cs+0.05f, 0.2f, cs+0.05f});
            Mat4 roofMVP = mProj * vp * roof;
            mBuildingShader.setMat4("uMVP", roofMVP);
            mBuildingShader.setMat4("uModel", roof);
            mBuildingShader.setVec4("uColor", 0.4f, 0.4f, 0.45f, 1.0f);
            mCubeMesh.draw();

            // Окна (маленькие тёмные прямоугольники на фасадах)
            for(int fl=0;fl<(int)(ht/1.5f);fl++) {
                float wy = fl*1.5f + 0.7f;
                // Фронт и зад
                for(int side=0;side<2;side++) {
                    float sz = side==0 ? wz-cs/2-0.01f : wz+cs/2+0.01f;
                    Mat4 win = Mat4::translate({wx, wy, sz}) * Mat4::scale({0.4f, 0.5f, 0.05f});
                    Mat4 winMVP = mProj * vp * win;
                    mBuildingShader.setMat4("uMVP", winMVP);
                    mBuildingShader.setMat4("uModel", win);
                    // ночное свечение окон
                    mBuildingShader.setVec4("uColor", 0.9f, 0.85f, 0.3f, 1.0f);
                    mCubeMesh.draw();
                }
            }
        }
    }
}

// ─── Деревья ───────────────────────────────────────────────────────────────
void Renderer::renderTrees(const GameState& gs, const Mat4& vp) {
    mTreeShader.use();
    mTreeShader.setVec3("uLightDir", 0.5f, 1.0f, 0.3f);
    mTreeShader.setFloat("uAmbient", 0.4f);

    for(const auto& tr : gs.trees) {
        float s = tr.scale;
        // Ствол
        Mat4 trunk = Mat4::translate(tr.pos) * Mat4::scale({s,s*2.5f,s});
        Mat4 mvp = mProj * vp * trunk;
        mTreeShader.setMat4("uMVP", mvp);
        mTreeShader.setMat4("uModel", trunk);
        mTreeShader.setVec4("uColor", 0.45f, 0.3f, 0.15f, 1.0f);
        mCylinderMesh.draw();

        // Крона
        Mat4 crown = Mat4::translate(tr.pos + Vec3(0, s*2.5f, 0)) * Mat4::scale({s*2.5f, s*2.5f, s*2.5f});
        Mat4 cmvp = mProj * vp * crown;
        mTreeShader.setMat4("uMVP", cmvp);
        mTreeShader.setMat4("uModel", crown);
        mTreeShader.setVec4("uColor", 0.15f, 0.55f, 0.15f, 1.0f);
        mConeMesh.draw();
    }
}

// ─── Зомби (составные кубы) ────────────────────────────────────────────────
void Renderer::renderZombies(const GameState& gs, const Mat4& vp) {
    mZombieShader.use();
    mZombieShader.setVec3("uLightDir", 0.5f, 1.0f, 0.3f);
    mZombieShader.setFloat("uAmbient", 0.5f);

    for(const auto& z : gs.zombies) {
        if(!z.alive) continue;

        Vec3 p = z.pos;
        float bob = sinf(z.animTime) * 0.05f;
        float legSwing = sinf(z.animTime) * 0.3f;

        // Направление к игроку
        Vec3 toP = gs.playerPos - z.pos; toP.y=0;
        float ang = atan2f(toP.x, toP.z);
        Mat4 rot = Mat4::rotateY(ang);

        auto drawPart = [&](Vec3 offset, Vec3 sz, float r, float g2, float b) {
            Mat4 model = Mat4::translate(p + Vec3(0, bob, 0)) * rot *
                         Mat4::translate(offset) * Mat4::scale(sz);
            Mat4 mvp = mProj * vp * model;
            mZombieShader.setMat4("uMVP", mvp);
            mZombieShader.setMat4("uModel", model);
            mZombieShader.setVec4("uColor", r, g2, b, 1.0f);
            mCubeMesh.draw();
        };

        // Тело
        drawPart({0,0.8f,0},    {0.5f,0.6f,0.3f}, 0.3f,0.5f,0.25f);
        // Голова
        drawPart({0,1.4f,0},    {0.38f,0.38f,0.38f}, 0.5f,0.7f,0.4f);
        // Левая нога
        drawPart({-0.15f,0.25f+legSwing*0.1f,legSwing*0.15f}, {0.2f,0.5f,0.2f}, 0.25f,0.4f,0.2f);
        // Правая нога
        drawPart({0.15f,0.25f-legSwing*0.1f,-legSwing*0.15f},{0.2f,0.5f,0.2f}, 0.25f,0.4f,0.2f);
        // Левая рука (вытянута вперёд — классический зомби)
        drawPart({-0.35f,0.9f,0.2f},{0.18f,0.15f,0.45f}, 0.35f,0.55f,0.3f);
        // Правая рука
        drawPart({0.35f,0.85f,0.15f},{0.18f,0.15f,0.4f}, 0.35f,0.55f,0.3f);

        // HP бар над головой (2 кубика)
        float hpRatio = z.hp / (100.0f + gs.wave * 10.0f);
        Vec3 barPos = p + Vec3(0, 2.0f, 0);
        // Фон
        Mat4 bg = Mat4::translate(barPos) * Mat4::scale({0.6f,0.06f,0.06f});
        Mat4 bgMVP = mProj * vp * bg;
        mZombieShader.setMat4("uMVP", bgMVP); mZombieShader.setMat4("uModel", bg);
        mZombieShader.setVec4("uColor", 0.6f,0.1f,0.1f,1.0f);
        mCubeMesh.draw();
        // Полоска HP
        Mat4 hp = Mat4::translate(barPos + Vec3((hpRatio-1)*0.3f, 0, 0)) *
                  Mat4::scale({0.6f*hpRatio, 0.07f, 0.07f});
        Mat4 hpMVP = mProj * vp * hp;
        mZombieShader.setMat4("uMVP", hpMVP); mZombieShader.setMat4("uModel", hp);
        mZombieShader.setVec4("uColor", 0.1f,0.9f,0.1f,1.0f);
        mCubeMesh.draw();
    }
}

// ─── Пули ──────────────────────────────────────────────────────────────────
void Renderer::renderBullets(const GameState& gs, const Mat4& vp) {
    mBulletShader.use();
    for(const auto& b : gs.bullets) {
        if(!b.alive) continue;
        Mat4 model = Mat4::translate(b.pos) * Mat4::scale({0.08f,0.08f,0.08f});
        Mat4 mvp = mProj * vp * model;
        mBulletShader.setMat4("uMVP", mvp);
        mBulletShader.setVec4("uColor", 1.0f, 0.9f, 0.1f, 1.0f);
        mCubeMesh.draw();
    }
}

// ─── HUD (простые прямоугольники) ──────────────────────────────────────────
static void drawRect2D(const Shader& sh, const Mesh& quad,
                       float x, float y, float w, float h,
                       float r, float g, float b, float a,
                       const Mat4& ortho) {
    Mat4 model = Mat4::translate({x + w/2, y + h/2, 0}) * Mat4::scale({w, h, 1});
    Mat4 mvp = ortho * model;
    sh.use();
    sh.setMat4("uMVP", mvp);
    sh.setVec4("uColor", r, g, b, a);
    // Draw quad with 2D attrib
    static const float v[] = {-0.5f,-0.5f, 0.5f,-0.5f, 0.5f,0.5f, -0.5f,0.5f};
    static const unsigned short ii[] = {0,1,2,0,2,3};
    GLuint vbo,ibo;
    glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_DYNAMIC_DRAW);
    glGenBuffers(1,&ibo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(ii),ii,GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,8,(void*)0);
    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,0);
    glDeleteBuffers(1,&vbo); glDeleteBuffers(1,&ibo);
}

void Renderer::renderHUD(const GameState& gs) {
    glDisable(GL_DEPTH_TEST);
    // Ортогональная проекция
    Mat4 ortho; memset(ortho.m,0,sizeof(ortho.m));
    float w=mW,h=mH;
    ortho.m[0]=2.0f/w; ortho.m[5]=2.0f/h;
    ortho.m[10]=-1; ortho.m[12]=-1; ortho.m[13]=-1; ortho.m[15]=1;

    // HP бар
    float hpW = 200.0f * ((float)gs.playerHP/100.0f);
    drawRect2D(mHudShader, mQuadMesh, 20, 20, 202, 24, 0.2f,0.2f,0.2f,0.8f, ortho);
    drawRect2D(mHudShader, mQuadMesh, 21, 21, hpW,22,
               0.1f+0.9f*(1-gs.playerHP/100.0f), 0.7f*(gs.playerHP/100.0f), 0.1f, 0.9f, ortho);

    // Волна — верх по центру
    float boxW=160, boxH=36;
    drawRect2D(mHudShader, mQuadMesh, w/2-boxW/2, h-boxH-10, boxW,boxH, 0,0,0,0.6f, ortho);

    // Прицел
    renderCrosshair();

    // Миникарта (правый нижний угол)
    float mmS=120, mmX=w-mmS-10, mmY=10;
    drawRect2D(mHudShader, mQuadMesh, mmX, mmY, mmS, mmS, 0.1f,0.1f,0.1f,0.7f, ortho);
    // Зомби на миникарте
    float scale = mmS/(MAP_SIZE*CELL_SIZE);
    for(const auto& z : gs.zombies) {
        if(!z.alive) continue;
        float zx = mmX + z.pos.x*scale, zy = mmY + z.pos.z*scale;
        drawRect2D(mHudShader, mQuadMesh, zx-2,zy-2,4,4,0.9f,0.1f,0.1f,1.0f, ortho);
    }
    // Игрок на миникарте
    float px=mmX+gs.playerPos.x*scale, py=mmY+gs.playerPos.z*scale;
    drawRect2D(mHudShader, mQuadMesh, px-3,py-3,6,6,0.2f,0.8f,0.9f,1.0f, ortho);

    if(gs.gameOver) renderGameOver();

    glEnable(GL_DEPTH_TEST);
}

void Renderer::renderCrosshair() {
    Mat4 ortho; memset(ortho.m,0,sizeof(ortho.m));
    float w=mW,h=mH;
    ortho.m[0]=2.0f/w; ortho.m[5]=2.0f/h;
    ortho.m[10]=-1; ortho.m[12]=-1; ortho.m[13]=-1; ortho.m[15]=1;
    float cx=w/2,cy=h/2;
    // Горизонтальная линия
    drawRect2D(mHudShader,mQuadMesh, cx-15,cy-1,30,2, 1,1,1,0.9f, ortho);
    // Вертикальная линия
    drawRect2D(mHudShader,mQuadMesh, cx-1,cy-15,2,30, 1,1,1,0.9f, ortho);
    // Точка
    drawRect2D(mHudShader,mQuadMesh, cx-3,cy-3,6,6, 1,0.2f,0.2f,0.9f, ortho);
}

void Renderer::renderGameOver() {
    Mat4 ortho; memset(ortho.m,0,sizeof(ortho.m));
    float w=mW,h=mH;
    ortho.m[0]=2.0f/w; ortho.m[5]=2.0f/h;
    ortho.m[10]=-1; ortho.m[12]=-1; ortho.m[13]=-1; ortho.m[15]=1;
    drawRect2D(mHudShader,mQuadMesh, 0,0,w,h, 0,0,0,0.6f, ortho);
    drawRect2D(mHudShader,mQuadMesh, w/2-150,h/2-40,300,80, 0.7f,0.05f,0.05f,0.9f, ortho);
}

// ─── Главный рендер ────────────────────────────────────────────────────────
void Renderer::render(const GameState& gs) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Mat4 view = getView(gs);
    Mat4 vp   = mProj * view;

    renderGround(gs, vp);
    renderBuildings(gs, vp);
    renderTrees(gs, vp);
    renderZombies(gs, vp);
    renderBullets(gs, vp);
    renderHUD(gs);
}

void Renderer::destroy() {
    mQuadMesh.destroy();
    mCubeMesh.destroy();
    mCylinderMesh.destroy();
    mConeMesh.destroy();
}
