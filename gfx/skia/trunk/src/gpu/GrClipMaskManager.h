






#ifndef GrClipMaskManager_DEFINED
#define GrClipMaskManager_DEFINED

#include "GrClipMaskCache.h"
#include "GrContext.h"
#include "GrDrawState.h"
#include "GrReducedClip.h"
#include "GrStencil.h"
#include "GrTexture.h"

#include "SkClipStack.h"
#include "SkDeque.h"
#include "SkPath.h"
#include "SkRefCnt.h"
#include "SkTLList.h"
#include "SkTypes.h"

class GrGpu;
class GrPathRenderer;
class GrPathRendererChain;
class GrTexture;
class SkPath;









class GrClipMaskManager : public SkNoncopyable {
public:
    GrClipMaskManager()
        : fGpu(NULL)
        , fCurrClipMaskType(kNone_ClipMaskType) {
    }

    






    bool setupClipping(const GrClipData* clipDataIn, GrDrawState::AutoRestoreEffects*,
                       const SkRect* devBounds);

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

    GrContext* getContext() {
        return fAACache.getContext();
    }

    void setGpu(GrGpu* gpu);

    void adjustPathStencilParams(GrStencilSettings* settings);
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

    
    
    bool installClipEffects(const GrReducedClip::ElementList&,
                            GrDrawState::AutoRestoreEffects*,
                            const SkVector& clipOffset,
                            const SkRect* devBounds);

    
    bool createStencilClipMask(int32_t elementsGenID,
                               GrReducedClip::InitialState initialState,
                               const GrReducedClip::ElementList& elements,
                               const SkIRect& clipSpaceIBounds,
                               const SkIPoint& clipSpaceToStencilOffset);
    
    
    GrTexture* createAlphaClipMask(int32_t elementsGenID,
                                   GrReducedClip::InitialState initialState,
                                   const GrReducedClip::ElementList& elements,
                                   const SkIRect& clipSpaceIBounds);
    
    GrTexture* createSoftwareClipMask(int32_t elementsGenID,
                                      GrReducedClip::InitialState initialState,
                                      const GrReducedClip::ElementList& elements,
                                      const SkIRect& clipSpaceIBounds);

    
    
    
    bool getMaskTexture(int32_t elementsGenID,
                        const SkIRect& clipSpaceIBounds,
                        GrTexture** result,
                        bool willUpload);

    bool useSWOnlyPath(const GrReducedClip::ElementList& elements);

    
    
    
    bool drawElement(GrTexture* target, const SkClipStack::Element*, GrPathRenderer* = NULL);

    
    
    
    bool canStencilAndDrawElement(GrTexture* target, const SkClipStack::Element*, GrPathRenderer**);

    void mergeMask(GrTexture* dstMask,
                   GrTexture* srcMask,
                   SkRegion::Op op,
                   const SkIRect& dstBound,
                   const SkIRect& srcBound);

    void getTemp(int width, int height, GrAutoScratchTexture* temp);

    void setupCache(const SkClipStack& clip,
                    const SkIRect& bounds);

    




    void setGpuStencil();

    



    void adjustStencilParams(GrStencilSettings* settings,
                             StencilClipMode mode,
                             int stencilBitCnt);

    typedef SkNoncopyable INHERITED;
};

#endif
