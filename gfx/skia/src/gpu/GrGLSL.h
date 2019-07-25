






#ifndef GrGLSL_DEFINED
#define GrGLSL_DEFINED

#include "GrGLInterface.h"



enum GrGLSLGeneration {
    


    k110_GLSLGeneration,
    


    k130_GLSLGeneration,
    


    k150_GLSLGeneration,
};

GrGLSLGeneration GetGLSLGeneration(GrGLBinding binding,
                                   const GrGLInterface* gl);

#endif

