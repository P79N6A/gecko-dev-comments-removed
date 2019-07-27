







#include "gl/SkDebugGLContext.h"

const GrGLInterface* SkDebugGLContext::createGLContext(GrGLStandard forcedGpuAPI) {
    if (kGLES_GrGLStandard == forcedGpuAPI) {
        return NULL;
    }

    return GrGLCreateDebugInterface();
};
