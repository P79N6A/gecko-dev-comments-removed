






#include "gl/GrGLInterface.h"

const GrGLInterface* GrGLDefaultInterface() {
    return GrGLCreateNativeInterface();
}
