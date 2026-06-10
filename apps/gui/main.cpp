// 图形界面入口：用 GLFW 开窗口、建 OpenGL 3.3 核心上下文，初始化 Dear ImGui，
// 然后进入渲染循环，把 3D 视口和控制面板画出来。
//
// 普通运行：  pcseg_gui [点云文件]
// 截图模式：  pcseg_gui --screenshot out.ppm   （载入演示场景，渲染并保存一帧后退出）

#include "GLHeaders.h"
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "Viewer.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>

// 窗口里保存的应用状态：指向 Viewer，以及累积的滚轮量
struct AppState {
    Viewer* viewer = nullptr;
    double scrollY = 0.0;
};

// 滚轮回调：累积滚动量，留到主循环里处理（这样能先判断鼠标是否在面板上）
static void scrollCallback(GLFWwindow* w, double /*xoff*/, double yoff) {
    auto* s = static_cast<AppState*>(glfwGetWindowUserPointer(w));
    if (s) s->scrollY += yoff;
}

static void errorCallback(int code, const char* desc) {
    std::fprintf(stderr, "GLFW 错误 %d: %s\n", code, desc);
}

// 把当前帧缓冲读出来存成 PPM 图片（用于报告插图/无人值守验证）
static bool saveScreenshotPPM(const std::string& path, int w, int h) {
    std::vector<unsigned char> pixels(w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << w << " " << h << "\n255\n";
    // OpenGL 的原点在左下，图片习惯左上，所以按行翻转
    for (int y = h - 1; y >= 0; --y)
        out.write(reinterpret_cast<char*>(&pixels[y * w * 3]), w * 3);
    return true;
}

// 尝试加载一个中文字体，让 ImGui 能显示中文（找不到就用默认字体）
static void setupFont() {
    ImGuiIO& io = ImGui::GetIO();
    const char* candidates[] = {
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
    };
    for (const char* f : candidates) {
        std::ifstream test(f);
        if (test.good()) {
            io.Fonts->AddFontFromFileTTF(
                f, 18.0f, nullptr,
                io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            return;
        }
    }
    io.Fonts->AddFontDefault();  // 没找到中文字体，退回默认（中文会显示为方块）
}

int main(int argc, char** argv) {
    std::string loadPath;
    std::string screenshotPath;
    int shotLevel = -1;
    std::string shotColor;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--screenshot" && i + 1 < argc) screenshotPath = argv[++i];
        else if (a == "--level" && i + 1 < argc) shotLevel = std::atoi(argv[++i]);
        else if (a == "--color" && i + 1 < argc) shotColor = argv[++i];
        else if (!a.empty() && a[0] != '-') loadPath = a;
    }

    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW 初始化失败（可能没有可用显示器）。\n");
        return 1;
    }

    // 请求 OpenGL 3.3 核心模式（macOS 需要 forward-compatible）
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    if (!screenshotPath.empty()) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(1280, 800, "3D 点云目标分割系统", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "创建窗口失败（无显示环境时会这样）。\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    AppState state;
    glfwSetWindowUserPointer(window, &state);
    // 先设好自己的滚轮回调，ImGui 初始化时会"链式"调用到它
    glfwSetScrollCallback(window, scrollCallback);

    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    setupFont();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    Viewer viewer;
    state.viewer = &viewer;
    if (!viewer.initGL()) {
        std::fprintf(stderr, "OpenGL 初始化失败。\n");
        return 1;
    }
    // 载入初始点云
    if (!loadPath.empty()) viewer.loadFromFile(loadPath);
    else viewer.loadDemoScene();

    // 截图模式下，按命令行指定的着色方式和层来设置画面
    if (!screenshotPath.empty()) {
        if (shotColor == "normal") viewer.setColorMode(Viewer::ColorMode::Normal);
        else if (shotColor == "height") viewer.setColorMode(Viewer::ColorMode::Height);
        else if (shotColor == "flat") viewer.setColorMode(Viewer::ColorMode::Flat);
        if (shotLevel >= 0) viewer.setLevel(shotLevel);
    }

    // 鼠标拖动状态
    double lastX = 0, lastY = 0;
    bool wasDragging = false;

    int frameCount = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGuiIO& io = ImGui::GetIO();

        // 处理滚轮缩放（鼠标不在面板上时才作用于相机）
        if (state.scrollY != 0.0) {
            if (!io.WantCaptureMouse) viewer.onScroll((float)state.scrollY);
            state.scrollY = 0.0;
        }

        // 处理鼠标拖动旋转/平移
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        bool left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        bool dragging = (left || right) && !io.WantCaptureMouse;
        if (dragging && wasDragging) {
            viewer.onMouseDrag(left ? 0 : 1, (float)(mx - lastX), (float)(my - lastY));
        }
        wasDragging = dragging;
        lastX = mx;
        lastY = my;

        // 开始一帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        viewer.drawUI();

        ImGui::Render();
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        viewer.render(fbW, fbH);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        // 截图模式：渲染几帧让画面稳定后存图并退出
        if (!screenshotPath.empty()) {
            if (++frameCount >= 3) {
                if (saveScreenshotPPM(screenshotPath, fbW, fbH))
                    std::printf("已保存截图: %s (%dx%d)\n", screenshotPath.c_str(), fbW, fbH);
                else
                    std::fprintf(stderr, "保存截图失败\n");
                break;
            }
        }
    }

    // 清理
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
