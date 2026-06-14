// Viewer 类的“OpenGL 渲染”部分（点云载入 + 控制面板见 ViewerPanel.cpp）。
#include "Viewer.h"
#include "GLHeaders.h"

#include "pcseg/Palette.h"
#include "pcseg/PointCloudIO.h"

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>

using namespace pcseg;

// ----------------------- 着色器 -----------------------
// 顶点着色器：把点变换到裁剪空间，设定点的屏幕大小，把颜色传给片段着色器
static const char* kVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uMVP;
uniform float uPointSize;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    gl_PointSize = uPointSize;
    vColor = aColor;
}
)";

// 片段着色器：把方点裁成圆点，输出颜色
static const char* kFragmentShader = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    vec2 c = gl_PointCoord * 2.0 - 1.0;   // [-1,1]
    if (dot(c, c) > 1.0) discard;          // 圆形之外丢弃，点看起来是圆的
    FragColor = vec4(vColor, 1.0);
}
)";

static unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::fprintf(stderr, "着色器编译失败: %s\n", log);
    }
    return s;
}

Viewer::~Viewer() {
    if (vboPos_) glDeleteBuffers(1, &vboPos_);
    if (vboCol_) glDeleteBuffers(1, &vboCol_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (program_) glDeleteProgram(program_);
}

bool Viewer::initGL() {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    int ok = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::fprintf(stderr, "着色器链接失败: %s\n", log);
        return false;
    }
    mvpLoc_ = glGetUniformLocation(program_, "uMVP");
    pointSizeLoc_ = glGetUniformLocation(program_, "uPointSize");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vboPos_);
    glGenBuffers(1, &vboCol_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vboPos_);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vboCol_);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);   // 允许在着色器里设置 gl_PointSize

    glReady_ = true;
    return true;
}

void Viewer::uploadPositions() {
    if (!glReady_) return;
    glBindBuffer(GL_ARRAY_BUFFER, vboPos_);
    glBufferData(GL_ARRAY_BUFFER, cloud_.size() * sizeof(Vec3),
                 cloud_.points.data(), GL_STATIC_DRAW);
}

void Viewer::updateColors() {
    if (!glReady_ || cloud_.empty()) return;
    std::vector<Vec3> colors(cloud_.size());

    const std::vector<int>* labels = nullptr;
    if (hasSegmentation_ && currentLevel_ >= 0 && currentLevel_ < seg_.levels())
        labels = &seg_.levelLabels[currentLevel_];

    // 高度着色用到 z 范围
    Vec3 mn, mx;
    cloud_.boundingBox(mn, mx);
    float zr = (mx.z - mn.z);
    if (zr < 1e-6f) zr = 1.0f;

    for (std::size_t i = 0; i < cloud_.size(); ++i) {
        switch (colorMode_) {
            case ColorMode::Segment:
                colors[i] = labels ? colorForLabel((*labels)[i]) : Vec3(0.8f, 0.8f, 0.8f);
                break;
            case ColorMode::Normal: {
                Vec3 n = cloud_.hasNormals() ? cloud_.normals[i] : Vec3(0, 0, 1);
                colors[i] = Vec3((n.x + 1) * 0.5f, (n.y + 1) * 0.5f, (n.z + 1) * 0.5f);
                break;
            }
            case ColorMode::Height: {
                float t = (cloud_.points[i].z - mn.z) / zr;   // 0~1
                colors[i] = Vec3(t, 0.4f, 1.0f - t);          // 蓝->红渐变
                break;
            }
            case ColorMode::Flat:
                colors[i] = Vec3(0.75f, 0.78f, 0.82f);
                break;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, vboCol_);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(Vec3),
                 colors.data(), GL_STATIC_DRAW);
}

bool Viewer::exportCurrentSegmentation(const std::string& path) {
    if (cloud_.empty()) {
        status_ = "导出失败: 未载入点云";
        return false;
    }
    if (!hasSegmentation_ || currentLevel_ < 0 || currentLevel_ >= seg_.levels()) {
        status_ = "导出失败: 没有可导出的分割结果";
        return false;
    }

    const std::vector<int>& labels = seg_.levelLabels[currentLevel_];
    std::vector<unsigned char> r(cloud_.size()), g(cloud_.size()), b(cloud_.size());
    for (std::size_t i = 0; i < cloud_.size(); ++i) {
        Vec3 c = colorForLabel(labels[i]);
        r[i] = static_cast<unsigned char>(c.x * 255.0f);
        g[i] = static_cast<unsigned char>(c.y * 255.0f);
        b[i] = static_cast<unsigned char>(c.z * 255.0f);
    }

    std::string err;
    if (!savePLYColored(path, cloud_, r, g, b, err)) {
        status_ = "导出失败: " + err;
        return false;
    }
    status_ = "已导出第 " + std::to_string(currentLevel_) + " 层分割结果到 " + path;
    return true;
}

void Viewer::render(int fbWidth, int fbHeight) {
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(bgGray_, bgGray_, bgGray_, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (cloud_.empty() || !glReady_) return;

    float aspect = (fbHeight > 0) ? (float)fbWidth / (float)fbHeight : 1.0f;
    // 近/远裁剪面根据场景大小自适应
    float zNear = std::max(0.001f, sceneRadius_ * 0.01f);
    float zFar = sceneRadius_ * 100.0f;
    Mat4 proj = Mat4::perspective(45.0f * 3.14159265f / 180.0f, aspect, zNear, zFar);
    Mat4 view = camera_.viewMatrix();
    Mat4 mvp = proj * view;

    glUseProgram(program_);
    glUniformMatrix4fv(mvpLoc_, 1, GL_FALSE, mvp.m);
    glUniform1f(pointSizeLoc_, pointSize_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, (int)cloud_.size());
    glBindVertexArray(0);
}
