#pragma once
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include "../game/Game.h"

// ─── Матрица 4×4 ──────────────────────────────────────────────────────────
struct Mat4 {
    float m[16];
    static Mat4 identity();
    static Mat4 perspective(float fov, float aspect, float near, float far);
    static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up);
    static Mat4 translate(Vec3 t);
    static Mat4 scale(Vec3 s);
    static Mat4 rotateY(float a);
    static Mat4 rotateX(float a);
    Mat4 operator*(const Mat4& o) const;
};

// ─── Шейдер ───────────────────────────────────────────────────────────────
struct Shader {
    GLuint prog;
    bool compile(const char* vs, const char* fs);
    void use() const;
    void setMat4(const char* name, const Mat4& m) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;
    void setFloat(const char* name, float v) const;
    void setInt(const char* name, int v) const;
};

// ─── Меш ──────────────────────────────────────────────────────────────────
struct Mesh {
    GLuint vbo, ibo;
    int    indexCount;
    void create(const float* verts, int vsize, const unsigned short* idx, int isize);
    void draw() const;
    void destroy();
};

// ─── Рендерер ─────────────────────────────────────────────────────────────
class Renderer {
public:
    bool init(int screenW, int screenH);
    void render(const GameState& gs);
    void resize(int w, int h);
    void destroy();

private:
    int   mW, mH;

    Shader mGroundShader;
    Shader mBuildingShader;
    Shader mZombieShader;
    Shader mBulletShader;
    Shader mTreeShader;
    Shader mHudShader;

    Mesh   mQuadMesh;      // 1×1 плоскость
    Mesh   mCubeMesh;      // 1×1×1 куб
    Mesh   mCylinderMesh;  // ствол дерева
    Mesh   mConeMesh;      // крона дерева

    Mat4   mProj;

    void buildMeshes();
    void renderGround(const GameState& gs, const Mat4& vp);
    void renderBuildings(const GameState& gs, const Mat4& vp);
    void renderTrees(const GameState& gs, const Mat4& vp);
    void renderZombies(const GameState& gs, const Mat4& vp);
    void renderBullets(const GameState& gs, const Mat4& vp);
    void renderHUD(const GameState& gs);
    void renderCrosshair();
    void renderGameOver();
    Mat4  getView(const GameState& gs) const;
    void  drawMesh(const Mesh& mesh, const Shader& shader,
                   const Mat4& mvp, float r,float g,float b,float a=1.0f);
};
