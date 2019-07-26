







#ifndef GrClipMaskManager_DEFINED
#define GrClipMaskManager_DEFINED

#include "GrContext.h"
#include "GrNoncopyable.h"
#include "GrRect.h"
#include "GrStencil.h"
#include "GrTexture.h"

#include "SkClipStack.h"
#include "SkDeque.h"
#include "SkPath.h"
#include "SkRefCnt.h"

#include "GrClipMaskCache.h"

class GrGpu;
class GrPathRenderer;
class GrPathRendererChain;
class SkPath;
class GrTexture;
class GrDrawState;









class GrClipMaskManager : public GrNoncopyable {
public:
    GR_DECLARE_RESOURCE_CACHE_DOMAIN(GetAlphaMaskDomain)

    GrClipMaskManager()
        : fGpu(NULL)
        , fCurrClipMaskType(kNone_ClipMaskType) {
    }

    




    bool setupClipping(const GrClipData* clipDataIn);

    void releaseResources();

    bool isClipInStencil() const {
        return kStencil_ClipMaskType == fCurrClipMaskType;
    }
    bool isClipInAlpha() const {
        return kAlpha_ClipMaskType == fCurrClipMaskType;
    }

    void invalidateStencilMask() {
        if (kStencil_ClipMaskType == fCurrClipMaskType) {
            fCurrClipMaskType = kNone_ClipMaskType;
        }
    }

    void setContext(GrContext* context) {
        fAACache.setContext(context);
    }

    GrContext* getContext() {
        return fAACache.getContext();
    }

    void setGpu(GrGpu* gpu) {
        fGpu = gpu;
    }

private:
    



    enum StencilClipMode {
        
        kModifyClip_StencilClipMode,
        
        
        kRespectClip_StencilClipMode,
        
        kIgnoreClip_StencilClipMode,
    };

    GrGpu* fGpu;

    




    enum ClipMaskType {
        kNone_ClipMaskType,
        kStencil_ClipMaskType,
        kAlpha_ClipMaskType,
    } fCurrClipMaskType;

    GrClipMaskCache fAACache;       

    bool createStencilClipMask(const GrClipData& clipDataIn,
                               const GrIRect& devClipBounds);
    bool createAlphaClipMask(const GrClipData& clipDataIn,
                             GrTexture** result,
                             GrIRect *devResultBounds);
    bool createSoftwareClipMask(const GrClipData& clipDataIn,
                                GrTexture** result,
                                GrIRect *devResultBounds);
    bool clipMaskPreamble(const GrClipData& clipDataIn,
                          GrTexture** result,
                          GrIRect *devResultBounds);

    bool useSWOnlyPath(const SkClipStack& clipIn);

    bool drawClipShape(GrTexture* target,
                       const SkClipStack::Iter::Clip* clip,
                       const GrIRect& resultBounds);

    void drawTexture(GrTexture* target,
                     GrTexture* texture);

    void getTemp(const GrIRect& bounds, GrAutoScratchTexture* temp);

    void setupCache(const SkClipStack& clip,
                    const GrIRect& bounds);

    




    void setGpuStencil();

    



    void adjustStencilParams(GrStencilSettings* settings,
                             StencilClipMode mode,
                             int stencilBitCnt);

    typedef GrNoncopyable INHERITED;
};

#endif
