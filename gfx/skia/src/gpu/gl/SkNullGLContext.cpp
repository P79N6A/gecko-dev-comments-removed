







#include "gl/SkNullGLContext.h"

const GrGLInterface* SkNullGLContext::createGLContext() {
    return GrGLCreateNullInterface();
};
