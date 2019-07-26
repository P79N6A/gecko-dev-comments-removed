








#include "GrTypes.h"

#include "gl/GrGLConfig.h"

#include "GrGpu.h"
#include "gl/GrGpuGL.h"

GrGpu* GrGpu::Create(GrBackend backend, GrBackendContext backendContext, GrContext* context) {

    const GrGLInterface* glInterface = NULL;
    SkAutoTUnref<const GrGLInterface> glInterfaceUnref;

    if (kOpenGL_GrBackend == backend) {
        glInterface = reinterpret_cast<const GrGLInterface*>(backendContext);
        if (NULL == glInterface) {
            glInterface = GrGLDefaultInterface();
            
            
            
            glInterfaceUnref.reset(glInterface);
        }
        if (NULL == glInterface) {
#if GR_DEBUG
            GrPrintf("No GL interface provided!\n");
#endif
            return NULL;
        }
        GrGLContext ctx(glInterface);
        if (ctx.isInitialized()) {
            return SkNEW_ARGS(GrGpuGL, (ctx, context));
        }
    }
    return NULL;
}
