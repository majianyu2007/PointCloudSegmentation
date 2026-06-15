#include "GLHeaders.h"

#ifndef __APPLE__
PCSEG_PFNGLDRAWARRAYSPROC pcseg_glDrawArrays = nullptr;
PCSEG_PFNGLUNIFORM1FPROC pcseg_glUniform1f = nullptr;
#endif

bool loadPcsegGLFunctions() {
#ifdef __APPLE__
    return true;
#else
    pcseg_glDrawArrays =
        reinterpret_cast<PCSEG_PFNGLDRAWARRAYSPROC>(imgl3wGetProcAddress("glDrawArrays"));
    pcseg_glUniform1f =
        reinterpret_cast<PCSEG_PFNGLUNIFORM1FPROC>(imgl3wGetProcAddress("glUniform1f"));
    return pcseg_glDrawArrays && pcseg_glUniform1f;
#endif
}
