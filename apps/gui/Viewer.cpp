#include "Viewer.h"
#include "GLHeaders.h"

#include "imgui.h"

#include "pcseg/Normals.h"
#include "pcseg/PointCloudIO.h"
#include "pcseg/Palette.h"
#include "pcseg/Synthetic.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

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

void Viewer::loadDemoScene() {
    LabeledCloud lc = makeScene();
    cloud_ = lc.cloud;
    tree_.build(cloud_.points);
    estimateNormals(cloud_, tree_, params_.k);
    fitCameraToCloud();
    uploadPositions();
    runSegmentation();
    status_ = "已载入演示场景，点数 " + std::to_string(cloud_.size());
}

bool Viewer::loadFromFile(const std::string& path) {
    PointCloud tmp;
    std::string err;
    if (!loadPointCloud(path, tmp, err)) {
        status_ = "载入失败: " + err;
        return false;
    }
    cloud_ = tmp;
    tree_.build(cloud_.points);
    estimateNormals(cloud_, tree_, params_.k);
    fitCameraToCloud();
    uploadPositions();
    runSegmentation();
    status_ = "已载入 " + path + "，点数 " + std::to_string(cloud_.size());
    return true;
}

void Viewer::runSegmentation() {
    if (cloud_.empty()) return;
    // 参数里改了 k 的话，法向量也要重算
    estimateNormals(cloud_, tree_, params_.k);
    seg_ = segment(cloud_, tree_, params_);
    hasSegmentation_ = seg_.levels() > 0;
    currentLevel_ = hasSegmentation_ ? seg_.levels() - 1 : 0;
    updateColors();
    if (hasSegmentation_) {
        status_ = "分割完成，各层段数:";
        for (int L = 0; L < seg_.levels(); ++L)
            status_ += " L" + std::to_string(L) + "=" + std::to_string(seg_.segmentCount[L]);
    }
}

void Viewer::fitCameraToCloud() {
    Vec3 mn, mx;
    cloud_.boundingBox(mn, mx);
    Vec3 center = (mn + mx) * 0.5f;
    float radius = (mx - mn).length() * 0.5f;
    if (radius < 1e-4f) radius = 1.0f;
    sceneRadius_ = radius;
    camera_.setTarget(center);
    camera_.setDistance(radius * 3.0f);
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

void Viewer::drawUI() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330, 720), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"控制面板 / Control Panel");

    ImGui::TextWrapped(u8"选题56：3D点云目标分割。读取点云→估计法向量→由粗到细区域生长分割。");
    ImGui::Separator();

    // --- 载入 ---
    ImGui::TextUnformatted(u8"载入点云");
    ImGui::InputText("##path", pathBuf_, sizeof(pathBuf_));
    ImGui::SameLine();
    if (ImGui::Button(u8"载入文件")) loadFromFile(pathBuf_);
    if (ImGui::Button(u8"载入内置演示场景")) loadDemoScene();
    ImGui::Separator();

    // --- 分割参数 ---
    ImGui::TextUnformatted(u8"分割参数");
    ImGui::SliderInt(u8"近邻数 k", &params_.k, 4, 40);
    ImGui::SliderInt(u8"层数 levels", &params_.levels, 1, 5);
    ImGui::SliderFloat(u8"最粗夹角(度)", &params_.coarseSmoothnessDeg, 5.0f, 60.0f, "%.1f");
    ImGui::SliderFloat(u8"最细夹角(度)", &params_.fineSmoothnessDeg, 2.0f, 40.0f, "%.1f");
    ImGui::SliderFloat(u8"种子曲率阈值", &params_.curvatureThreshold, 0.005f, 0.3f, "%.3f");
    ImGui::SliderInt(u8"最小段点数", &params_.minClusterSize, 1, 300);
    if (ImGui::Button(u8"运行分割")) runSegmentation();
    ImGui::Separator();

    // --- 显示 ---
    ImGui::TextUnformatted(u8"显示");
    const char* modes[] = {u8"分割结果", u8"法向量", u8"高度", u8"纯色"};
    int mode = (int)colorMode_;
    if (ImGui::Combo(u8"着色方式", &mode, modes, 4)) {
        colorMode_ = (ColorMode)mode;
        updateColors();
    }

    // 迭代回放：在各层之间切换，看由粗到细的过程
    if (hasSegmentation_ && seg_.levels() > 1) {
        int maxLevel = seg_.levels() - 1;
        if (ImGui::SliderInt(u8"迭代层(粗→细)", &currentLevel_, 0, maxLevel)) {
            if (colorMode_ == ColorMode::Segment) updateColors();
        }
        ImGui::Text(u8"第 %d 层，段数 = %d", currentLevel_, seg_.segmentCount[currentLevel_]);
    }

    if (ImGui::SliderFloat(u8"点大小", &pointSize_, 1.0f, 12.0f, "%.1f")) {}
    ImGui::SliderFloat(u8"背景灰度", &bgGray_, 0.0f, 1.0f, "%.2f");
    ImGui::Separator();

    // --- 视角 ---
    ImGui::TextUnformatted(u8"视角");
    if (ImGui::Button(u8"正视")) camera_.viewFront();
    ImGui::SameLine();
    if (ImGui::Button(u8"侧视")) camera_.viewSide();
    ImGui::SameLine();
    if (ImGui::Button(u8"俯视")) camera_.viewTop();
    ImGui::TextWrapped(u8"左键拖动旋转，右键拖动平移，滚轮缩放。");
    ImGui::Separator();

    ImGui::TextWrapped(u8"状态: %s", status_.c_str());
    ImGui::Text(u8"帧率: %.1f FPS", ImGui::GetIO().Framerate);
    ImGui::End();
}

void Viewer::onMouseDrag(int button, float dx, float dy) {
    if (button == 0) {        // 左键：旋转
        camera_.orbit(-dx * 0.3f, -dy * 0.3f);
    } else {                  // 右键/中键：平移
        camera_.pan(dx, dy);
    }
}
