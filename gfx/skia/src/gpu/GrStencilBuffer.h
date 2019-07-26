








#ifndef GrStencilBuffer_DEFINED
#define GrStencilBuffer_DEFINED

#include "GrClipData.h"
#include "GrResource.h"
#include "GrCacheID.h"

class GrRenderTarget;
class GrResourceEntry;
class GrResourceKey;

class GrStencilBuffer : public GrResource {
public:
    SK_DECLARE_INST_COUNT(GrStencilBuffer);
    GR_DECLARE_RESOURCE_CACHE_TYPE()

    virtual ~GrStencilBuffer() {
        
    }

    int width() const { return fWidth; }
    int height() const { return fHeight; }
    int bits() const { return fBits; }
    int numSamples() const { return fSampleCnt; }

    
    void setLastClip(const GrClipData& clipData, int width, int height) {
        
        
        fLastClipStack = *clipData.fClipStack;
        fLastClipData.fClipStack = &fLastClipStack;
        fLastClipData.fOrigin = clipData.fOrigin;
        fLastClipWidth = width;
        fLastClipHeight = height;
        GrAssert(width <= fWidth);
        GrAssert(height <= fHeight);
    }

    
    bool mustRenderClip(const GrClipData& clipData, int width, int height) const {
        
        
        
        
        return width > fLastClipWidth ||
               height > fLastClipHeight ||
               clipData != fLastClipData;
    }

    const GrClipData& getLastClip() const {
        return fLastClipData;
    }

    
    void transferToCache();

    static GrResourceKey ComputeKey(int width, int height, int sampleCnt);

protected:
    GrStencilBuffer(GrGpu* gpu, int width, int height, int bits, int sampleCnt)
        : GrResource(gpu)
        , fWidth(width)
        , fHeight(height)
        , fBits(bits)
        , fSampleCnt(sampleCnt)
        , fLastClipStack()
        , fLastClipData()
        , fLastClipWidth(-1)
        , fLastClipHeight(-1) {
    }

private:

    int fWidth;
    int fHeight;
    int fBits;
    int fSampleCnt;

    SkClipStack fLastClipStack;
    GrClipData  fLastClipData;
    int         fLastClipWidth;
    int         fLastClipHeight;

    typedef GrResource INHERITED;
};

#endif
