#ifndef PCSEG_GUI_GLHEADERS_H
#define PCSEG_GUI_GLHEADERS_H

// 按平台包含正确的 OpenGL 头文件。
// macOS 自带 OpenGL.framework，直接用 <OpenGL/gl3.h> 就能拿到 3.x 的全部函数。
// Windows / Linux 的系统 OpenGL 头通常不声明现代 OpenGL 函数；这里复用 ImGui
// 后端自带的轻量 loader，并在 GLCompat.cpp 中补齐本项目额外用到的少量符号。
#ifdef __APPLE__
    #ifndef GL_SILENCE_DEPRECATION
    #define GL_SILENCE_DEPRECATION
    #endif
    #include <OpenGL/gl3.h>
#else
    #include "imgui_impl_opengl3_loader.h"

    #ifndef GL_DEPTH_BUFFER_BIT
    #define GL_DEPTH_BUFFER_BIT 0x00000100
    #endif
    #ifndef GL_POINTS
    #define GL_POINTS 0x0000
    #endif
    #ifndef GL_STATIC_DRAW
    #define GL_STATIC_DRAW 0x88E4
    #endif
    #ifndef GL_RGB
    #define GL_RGB 0x1907
    #endif
    #ifndef GL_PROGRAM_POINT_SIZE
    #define GL_PROGRAM_POINT_SIZE 0x8642
    #endif

    typedef void (APIENTRYP PCSEG_PFNGLDRAWARRAYSPROC)(GLenum mode, GLint first, GLsizei count);
    typedef void (APIENTRYP PCSEG_PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);

    extern PCSEG_PFNGLDRAWARRAYSPROC pcseg_glDrawArrays;
    extern PCSEG_PFNGLUNIFORM1FPROC pcseg_glUniform1f;

    #ifndef glDrawArrays
    #define glDrawArrays pcseg_glDrawArrays
    #endif
    #ifndef glUniform1f
    #define glUniform1f pcseg_glUniform1f
    #endif
#endif

bool loadPcsegGLFunctions();

#endif // PCSEG_GUI_GLHEADERS_H
