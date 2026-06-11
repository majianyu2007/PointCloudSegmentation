#ifndef PCSEG_GUI_GLHEADERS_H
#define PCSEG_GUI_GLHEADERS_H

// 按平台包含正确的 OpenGL 头文件。
// macOS 自带 OpenGL.framework，直接用 <OpenGL/gl3.h> 就能拿到 3.x 的全部函数。
// Windows / Linux 的系统 OpenGL 头通常不声明现代 OpenGL 函数，所以复用 ImGui
// 后端自带的轻量 loader。loader 的实现由 imgui_impl_opengl3.cpp 提供。
#ifdef __APPLE__
    #ifndef GL_SILENCE_DEPRECATION
    #define GL_SILENCE_DEPRECATION
    #endif
    #include <OpenGL/gl3.h>
#else
    #include "imgui_impl_opengl3_loader.h"
#endif

#endif // PCSEG_GUI_GLHEADERS_H
