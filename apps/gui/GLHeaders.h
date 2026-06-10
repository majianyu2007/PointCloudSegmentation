#ifndef PCSEG_GUI_GLHEADERS_H
#define PCSEG_GUI_GLHEADERS_H

// 按平台包含正确的 OpenGL 头文件。
// macOS 自带 OpenGL.framework，直接用 <OpenGL/gl3.h> 就能拿到 3.x 的全部函数，
// 不需要 GLAD/GLEW 这类加载器。其它平台留个占位（本课程在 macOS 上开发）。
#ifdef __APPLE__
    #ifndef GL_SILENCE_DEPRECATION
    #define GL_SILENCE_DEPRECATION
    #endif
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif

#endif // PCSEG_GUI_GLHEADERS_H
