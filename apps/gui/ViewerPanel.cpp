#include "Viewer.h"

#include "imgui.h"

#include "pcseg/Normals.h"
#include "pcseg/PointCloudIO.h"
#include "pcseg/Synthetic.h"

#include <filesystem>
#include <string>
#include <vector>

using namespace pcseg;

namespace {

std::vector<std::filesystem::path> candidateRoots() {
    std::vector<std::filesystem::path> roots;

    std::filesystem::path cwd = std::filesystem::current_path();
    roots.push_back(cwd);
    for (std::filesystem::path p = cwd; p.has_parent_path(); p = p.parent_path()) {
        if (std::filesystem::exists(p / "CMakeLists.txt") &&
            std::filesystem::exists(p / "data")) {
            roots.push_back(p);
            break;
        }
        if (p == p.parent_path()) break;
    }
    return roots;
}

std::string resolvePointCloudPath(const std::string& input) {
    std::filesystem::path p(input);
    if (std::filesystem::exists(p)) return p.string();
    if (p.is_absolute()) return input;

    for (const auto& root : candidateRoots()) {
        std::filesystem::path candidate = root / p;
        if (std::filesystem::exists(candidate)) return candidate.string();
    }
    return input;
}

} // namespace

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
    std::string resolvedPath = resolvePointCloudPath(path);
    if (!loadPointCloud(resolvedPath, tmp, err)) {
        status_ = "载入失败: " + err + "（当前目录: " +
                  std::filesystem::current_path().string() + "）";
        return false;
    }
    cloud_ = tmp;
    tree_.build(cloud_.points);
    estimateNormals(cloud_, tree_, params_.k);
    fitCameraToCloud();
    uploadPositions();
    runSegmentation();
    status_ = "已载入 " + resolvedPath + "，点数 " + std::to_string(cloud_.size());
    return true;
}

void Viewer::runSegmentation() {
    if (cloud_.empty()) return;
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

void Viewer::drawUI() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330, 720), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"控制面板 / Control Panel");

    ImGui::TextWrapped(u8"选题56：3D点云目标分割。读取点云→估计法向量→由粗到细区域生长分割。");
    ImGui::Separator();

    ImGui::TextUnformatted(u8"载入点云");
    ImGui::InputText("##path", pathBuf_, sizeof(pathBuf_));
    ImGui::SameLine();
    if (ImGui::Button(u8"载入文件")) loadFromFile(pathBuf_);
    if (ImGui::Button(u8"载入内置演示场景")) loadDemoScene();
    ImGui::Separator();

    ImGui::TextUnformatted(u8"分割参数");
    ImGui::SliderInt(u8"近邻数 k", &params_.k, 4, 40);
    ImGui::SliderInt(u8"层数 levels", &params_.levels, 1, 5);
    ImGui::SliderFloat(u8"最粗夹角(度)", &params_.coarseSmoothnessDeg, 5.0f, 60.0f, "%.1f");
    ImGui::SliderFloat(u8"最细夹角(度)", &params_.fineSmoothnessDeg, 2.0f, 40.0f, "%.1f");
    ImGui::SliderFloat(u8"种子曲率阈值", &params_.curvatureThreshold, 0.005f, 0.3f, "%.3f");
    ImGui::SliderInt(u8"最小段点数", &params_.minClusterSize, 1, 300);
    if (ImGui::Button(u8"运行分割")) runSegmentation();
    ImGui::Separator();

    ImGui::TextUnformatted(u8"显示");
    const char* modes[] = {u8"分割结果", u8"法向量", u8"高度", u8"纯色"};
    int mode = (int)colorMode_;
    if (ImGui::Combo(u8"着色方式", &mode, modes, 4)) {
        colorMode_ = (ColorMode)mode;
        updateColors();
    }

    if (hasSegmentation_ && seg_.levels() > 1) {
        int maxLevel = seg_.levels() - 1;
        if (ImGui::SliderInt(u8"迭代层(粗→细)", &currentLevel_, 0, maxLevel)) {
            if (colorMode_ == ColorMode::Segment) updateColors();
        }
        ImGui::Text(u8"第 %d 层，段数 = %d", currentLevel_, seg_.segmentCount[currentLevel_]);
    }

    if (ImGui::SliderFloat(u8"点大小", &pointSize_, 1.0f, 12.0f, "%.1f")) {}
    ImGui::SliderFloat(u8"背景灰度", &bgGray_, 0.0f, 1.0f, "%.2f");
    ImGui::InputText(u8"导出路径", exportPathBuf_, sizeof(exportPathBuf_));
    if (ImGui::Button(u8"导出当前分割")) exportCurrentSegmentation(exportPathBuf_);
    ImGui::Separator();

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
