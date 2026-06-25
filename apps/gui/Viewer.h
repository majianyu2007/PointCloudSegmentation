#ifndef PCSEG_GUI_VIEWER_H
#define PCSEG_GUI_VIEWER_H

#include <string>
#include <vector>

#include "pcseg/PointCloud.h"
#include "pcseg/KdTree.h"
#include "pcseg/Segmentation.h"
#include "Camera.h"

// 点云查看器，封装数据、分割结果、相机和 OpenGL 绘制状态。
class Viewer {
public:
    enum class ColorMode { Segment, Normal, Height, Flat };

    Viewer() = default;
    ~Viewer();

    bool initGL();

    void loadDemoScene();
    bool loadFromFile(const std::string& path);

    void runSegmentation();
    bool exportCurrentSegmentation(const std::string& path);

    void render(int fbWidth, int fbHeight);
    void drawUI();

    void onMouseDrag(int button, float dx, float dy);
    void onScroll(float dy) { camera_.zoom(dy > 0 ? 0.9f : 1.1f); }

    void setColorMode(ColorMode m) { colorMode_ = m; updateColors(); }
    bool setLevel(int l) {
        if (hasSegmentation_ && l >= 0 && l < seg_.levels()) {
            currentLevel_ = l; updateColors(); return true;
        }
        return false;
    }
    int numLevels() const { return seg_.levels(); }

private:
    void uploadPositions();
    void updateColors();
    void fitCameraToCloud();

    pcseg::PointCloud cloud_;
    pcseg::KdTree tree_;
    pcseg::SegResult seg_;
    pcseg::SegParams params_;
    std::string status_ = "未载入点云";
    char pathBuf_[512] = "data/scene.ply";
    char exportPathBuf_[512] = "segmented.ply";

    ColorMode colorMode_ = ColorMode::Segment;
    int currentLevel_ = 0;
    float pointSize_ = 3.0f;
    float bgGray_ = 0.12f;
    bool hasSegmentation_ = false;

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
