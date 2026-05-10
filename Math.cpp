#include "Renderer.h"
#include <cstring>
#include <cmath>

// ─── Mat4 ─────────────────────────────────────────────────────────────────
Mat4 Mat4::identity() {
    Mat4 r; memset(r.m, 0, sizeof(r.m));
    r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.0f;
    return r;
}

Mat4 Mat4::perspective(float fov, float aspect, float near, float far) {
    Mat4 r; memset(r.m, 0, sizeof(r.m));
    float f = 1.0f / tanf(fov * 0.5f);
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
}

Mat4 Mat4::lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = (center - eye).normalized();
    Vec3 r2;
    // cross(f, up)
    r2.x = f.y*up.z - f.z*up.y;
    r2.y = f.z*up.x - f.x*up.z;
    r2.z = f.x*up.y - f.y*up.x;
    r2 = r2.normalized();
    Vec3 u2;
    u2.x = r2.y*f.z - r2.z*f.y;
    u2.y = r2.z*f.x - r2.x*f.z;
    u2.z = r2.x*f.y - r2.y*f.x;
    Mat4 m = identity();
    m.m[0]=r2.x; m.m[4]=r2.y; m.m[8] =r2.z;
    m.m[1]=u2.x; m.m[5]=u2.y; m.m[9] =u2.z;
    m.m[2]=-f.x; m.m[6]=-f.y; m.m[10]=-f.z;
    m.m[12]=-(r2.x*eye.x+r2.y*eye.y+r2.z*eye.z);
    m.m[13]=-(u2.x*eye.x+u2.y*eye.y+u2.z*eye.z);
    m.m[14]= (f.x*eye.x+ f.y*eye.y+ f.z*eye.z);
    return m;
}

Mat4 Mat4::translate(Vec3 t) {
    Mat4 r = identity();
    r.m[12]=t.x; r.m[13]=t.y; r.m[14]=t.z;
    return r;
}

Mat4 Mat4::scale(Vec3 s) {
    Mat4 r = identity();
    r.m[0]=s.x; r.m[5]=s.y; r.m[10]=s.z;
    return r;
}

Mat4 Mat4::rotateY(float a) {
    Mat4 r = identity();
    r.m[0]=cosf(a); r.m[8]=sinf(a);
    r.m[2]=-sinf(a); r.m[10]=cosf(a);
    return r;
}

Mat4 Mat4::rotateX(float a) {
    Mat4 r = identity();
    r.m[5]=cosf(a); r.m[9]=-sinf(a);
    r.m[6]=sinf(a); r.m[10]=cosf(a);
    return r;
}

Mat4 Mat4::operator*(const Mat4& b) const {
    Mat4 r; memset(r.m, 0, sizeof(r.m));
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            for(int k=0;k<4;k++)
                r.m[j*4+i] += m[k*4+i] * b.m[j*4+k];
    return r;
}

// ─── Shader ───────────────────────────────────────────────────────────────
static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
        LOGE("Shader error: %s", log);
    }
    return s;
}

bool Shader::compile(const char* vs, const char* fs) {
    GLuint v = compileShader(GL_VERTEX_SHADER, vs);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
    prog = glCreateProgram();
    glAttachShader(prog, v); glAttachShader(prog, f);
    glLinkProgram(prog);
    glDeleteShader(v); glDeleteShader(f);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    return ok;
}

void Shader::use() const { glUseProgram(prog); }

void Shader::setMat4(const char* n, const Mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(prog,n),1,GL_FALSE,m.m);
}
void Shader::setVec3(const char* n,float x,float y,float z) const {
    glUniform3f(glGetUniformLocation(prog,n),x,y,z);
}
void Shader::setVec4(const char* n,float x,float y,float z,float w) const {
    glUniform4f(glGetUniformLocation(prog,n),x,y,z,w);
}
void Shader::setFloat(const char* n,float v) const {
    glUniform1f(glGetUniformLocation(prog,n),v);
}
void Shader::setInt(const char* n,int v) const {
    glUniform1i(glGetUniformLocation(prog,n),v);
}

// ─── Mesh ─────────────────────────────────────────────────────────────────
void Mesh::create(const float* verts, int vsize,
                  const unsigned short* idx, int isize) {
    indexCount = isize;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vsize, verts, GL_STATIC_DRAW);
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, isize*sizeof(unsigned short), idx, GL_STATIC_DRAW);
}

void Mesh::draw() const {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    // pos(3) + normal(3) = stride 24
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
}

void Mesh::destroy() {
    glDeleteBuffers(1,&vbo);
    glDeleteBuffers(1,&ibo);
}
