






#ifndef GrContextFactory_DEFINED
#define GrContextFactory_DEFINED

#if SK_ANGLE
    #include "gl/SkANGLEGLContext.h"
#endif
#include "gl/SkDebugGLContext.h"
#if SK_MESA
    #include "gl/SkMesaGLContext.h"
#endif
#include "gl/SkNativeGLContext.h"
#include "gl/SkNullGLContext.h"

#include "GrContext.h"
#include "SkTArray.h"








class GrContextFactory : SkNoncopyable {
public:
    





    enum GLContextType {
      kNative_GLContextType,
#if SK_ANGLE
      kANGLE_GLContextType,
#endif
#if SK_MESA
      kMESA_GLContextType,
#endif
      

      kNVPR_GLContextType,
      kNull_GLContextType,
      kDebug_GLContextType,

      kLastGLContextType = kDebug_GLContextType
    };

    static const int kGLContextTypeCnt = kLastGLContextType + 1;

    static bool IsRenderingGLContext(GLContextType type) {
        switch (type) {
            case kNull_GLContextType:
            case kDebug_GLContextType:
                return false;
            default:
                return true;
        }
    }

    static const char* GLContextTypeName(GLContextType type) {
        switch (type) {
            case kNative_GLContextType:
                return "native";
            case kNull_GLContextType:
                return "null";
#if SK_ANGLE
            case kANGLE_GLContextType:
                return "angle";
#endif
#if SK_MESA
            case kMESA_GLContextType:
                return "mesa";
#endif
            case kNVPR_GLContextType:
                return "nvpr";
            case kDebug_GLContextType:
                return "debug";
            default:
                SkFAIL("Unknown GL Context type.");
        }
    }

    GrContextFactory() { }

    ~GrContextFactory() { this->destroyContexts(); }

    void destroyContexts() {
        for (int i = 0; i < fContexts.count(); ++i) {
            fContexts[i].fGLContext->makeCurrent();
            fContexts[i].fGrContext->unref();
            fContexts[i].fGLContext->unref();
        }
        fContexts.reset();
    }

    


    GrContext* get(GLContextType type, GrGLStandard forcedGpuAPI = kNone_GrGLStandard) {
        for (int i = 0; i < fContexts.count(); ++i) {
            if (forcedGpuAPI != kNone_GrGLStandard &&
                forcedGpuAPI != fContexts[i].fGLContext->gl()->fStandard)
                continue;

            if (fContexts[i].fType == type) {
                fContexts[i].fGLContext->makeCurrent();
                return fContexts[i].fGrContext;
            }
        }
        SkAutoTUnref<SkGLContextHelper> glCtx;
        SkAutoTUnref<GrContext> grCtx;
        switch (type) {
            case kNVPR_GLContextType: 
            case kNative_GLContextType:
                glCtx.reset(SkNEW(SkNativeGLContext));
                break;
#ifdef SK_ANGLE
            case kANGLE_GLContextType:
                glCtx.reset(SkNEW(SkANGLEGLContext));
                break;
#endif
#ifdef SK_MESA
            case kMESA_GLContextType:
                glCtx.reset(SkNEW(SkMesaGLContext));
                break;
#endif
            case kNull_GLContextType:
                glCtx.reset(SkNEW(SkNullGLContext));
                break;
            case kDebug_GLContextType:
                glCtx.reset(SkNEW(SkDebugGLContext));
                break;
        }
        static const int kBogusSize = 1;
        if (!glCtx.get()) {
            return NULL;
        }
        if (!glCtx.get()->init(forcedGpuAPI, kBogusSize, kBogusSize)) {
            return NULL;
        }

        
        SkAutoTUnref<const GrGLInterface> glInterface(SkRef(glCtx.get()->gl()));
        if (kNVPR_GLContextType == type) {
            if (!glInterface->hasExtension("GL_NV_path_rendering")) {
                return NULL;
            }
        } else {
            glInterface.reset(GrGLInterfaceRemoveNVPR(glInterface));
            if (!glInterface) {
                return NULL;
            }
        }

        glCtx->makeCurrent();
        GrBackendContext p3dctx = reinterpret_cast<GrBackendContext>(glInterface.get());
        grCtx.reset(GrContext::Create(kOpenGL_GrBackend, p3dctx));
        if (!grCtx.get()) {
            return NULL;
        }
        GPUContext& ctx = fContexts.push_back();
        ctx.fGLContext = glCtx.get();
        ctx.fGLContext->ref();
        ctx.fGrContext = grCtx.get();
        ctx.fGrContext->ref();
        ctx.fType = type;
        return ctx.fGrContext;
    }

    
    
    SkGLContextHelper* getGLContext(GLContextType type) {
        for (int i = 0; i < fContexts.count(); ++i) {
            if (fContexts[i].fType == type) {
                return fContexts[i].fGLContext;
            }
        }

        return NULL;
    }

private:
    struct GPUContext {
        GLContextType             fType;
        SkGLContextHelper*        fGLContext;
        GrContext*                fGrContext;
    };
    SkTArray<GPUContext, true> fContexts;
};

#endif
