







#include "gl/SkDebugGLContext.h"

const GrGLInterface* SkDebugGLContext::createGLContext() {
    return GrGLCreateDebugInterface();
};
