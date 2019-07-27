







#include "gl/GrGLInterface.h"

typedef void(*GrGLFuncPtr)();
typedef GrGLFuncPtr (*GrGLGetProc)(void* ctx, const char name[]);





const GrGLInterface* GrGLAssembleGLInterface(void* ctx, GrGLGetProc get);
