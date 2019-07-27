







#include "gl/SkNullGLContext.h"

const GrGLInterface* SkNullGLContext::createGLContext(GrGLStandard forcedGpuAPI) {
    if (kGLES_GrGLStandard == forcedGpuAPI) {
        return NULL;
    }
    return GrGLCreateNullInterface();
};
