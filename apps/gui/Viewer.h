#ifndef PCSEG_GUI_VIEWER_H
#define PCSEG_GUI_VIEWER_H

#include <string>
#include <vector>

#include "pcseg/PointCloud.h"
#include "pcseg/KdTree.h"
#include "pcseg/Segmentation.h"
#include "Camera.h"

// 点云查看器：管理点云数据、分割结果、OpenGL 缓冲与着色器、相机，
// 并负责每帧渲染和绘制 ImGui 控制面板。main.cpp 只管开窗口和转发输入。
class Viewer {
public:
    // 着色方式
    enum class ColorMode { Segment, Normal, Height, Flat };

    Viewer() = default;
    ~Viewer();

    // 创建着色器和顶点缓冲（需在 OpenGL 上下文就绪后调用）
    bool initGL();

    // 载入点云：内置演示场景 / 文件
    void loadDemoScene();
    bool loadFromFile(const std::string& path);

    // 重新跑分割（用当前参数）
    void runSegmentation();

    // 每帧渲染点云
    void render(int fbWidth, int fbHeight);
    // 绘制控制面板（ImGui）
    void drawUI();

    // 输入转发
    void onMouseDrag(int button, float dx, float dy);
    void onScroll(float dy) { camera_.zoom(dy > 0 ? 0.9f : 1.1f); }

    // 截图模式用的设置接口
    void setColorMode(ColorMode m) { colorMode_ = m; updateColors(); }
    bool setLevel(int l) {
        if (hasSegmentation_ && l >= 0 && l < seg_.levels()) {
            currentLevel_ = l; updateColors(); return true;
        }
        return false;
    }
    int numLevels() const { return seg_.levels(); }

private:
    void uploadPositions();   // 上传点坐标到 GPU（载入时一次）
    void updateColors();      // 根据着色方式/层重算颜色并上传
    void fitCameraToCloud();  // 根据包围盒把相机放到合适位置

    // --- 数据 ---
    pcseg::PointCloud cloud_;
    pcseg::KdTree tree_;
    pcseg::SegResult seg_;
    pcseg::SegParams params_;
    std::string status_ = "未载入点云";
    char pathBuf_[512] = "data/scene.ply";

    // --- 显示状态 ---
    ColorMode colorMode_ = ColorMode::Segment;
    int currentLevel_ = 0;     // 当前显示第几层分割
    float pointSize_ = 3.0f;
    float bgGray_ = 0.12f;
    bool hasSegmentation_ = false;

    // --- OpenGL 对象 ---
    unsigned int program_ = 0;
    unsigned int vao_ = 0;
    unsigned int vboPos_ = 0;
    unsigned int vboCol_ = 0;
    int mvpLoc_ = -1;
    int pointSizeLoc_ = -1;
    bool glReady_ = false;

    Camera camera_;
    float sceneRadius_ = 1.0f;
};

#endif // PCSEG_GUI_VIEWER_H
