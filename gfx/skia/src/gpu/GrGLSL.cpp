






#include "GrGLSL.h"

GrGLSLGeneration GetGLSLGeneration(GrGLBinding binding,
                                   const GrGLInterface* gl) {
    GrGLSLVersion ver = GrGLGetGLSLVersion(gl);
    switch (binding) {
        case kDesktop_GrGLBinding:
            GrAssert(ver >= GR_GLSL_VER(1,10));
            if (ver >= GR_GLSL_VER(1,50)) {
                return k150_GLSLGeneration;
            } else if (ver >= GR_GLSL_VER(1,30)) {
                return k130_GLSLGeneration;
            } else {
                return k110_GLSLGeneration;
            }
        case kES2_GrGLBinding:
            
            GrAssert(ver >= GR_GL_VER(1,00));
            return k110_GLSLGeneration;
        default:
            GrCrash("Unknown GL Binding");
            return k110_GLSLGeneration; 
    }
}

