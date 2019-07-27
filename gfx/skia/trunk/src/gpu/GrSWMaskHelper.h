






#ifndef GrSWMaskHelper_DEFINED
#define GrSWMaskHelper_DEFINED

#include "GrColor.h"
#include "GrDrawState.h"
#include "SkBitmap.h"
#include "SkDraw.h"
#include "SkMatrix.h"
#include "SkRasterClip.h"
#include "SkRegion.h"
#include "SkTypes.h"

class GrAutoScratchTexture;
class GrContext;
class GrTexture;
class SkPath;
class SkStrokeRec;
class GrDrawTarget;















class GrSWMaskHelper : SkNoncopyable {
public:
    GrSWMaskHelper(GrContext* context)
    : fContext(context) {
    }

    
    
    
    
    bool init(const SkIRect& resultBounds, const SkMatrix* matrix);

    
    void draw(const SkRect& rect, SkRegion::Op op,
              bool antiAlias, uint8_t alpha);

    
    void draw(const SkPath& path, const SkStrokeRec& stroke, SkRegion::Op op,
              bool antiAlias, uint8_t alpha);

    
    
    bool getTexture(GrAutoScratchTexture* texture);

    
    void toTexture(GrTexture* texture);

    
    void clear(uint8_t alpha) {
        fBM.eraseColor(SkColorSetARGB(alpha, alpha, alpha, alpha));
    }

    
    
    static GrTexture* DrawPathMaskToTexture(GrContext* context,
                                            const SkPath& path,
                                            const SkStrokeRec& stroke,
                                            const SkIRect& resultBounds,
                                            bool antiAlias,
                                            SkMatrix* matrix);

    
    
    
    
    
    
    
    
    
    
    static void DrawToTargetWithPathMask(GrTexture* texture,
                                         GrDrawTarget* target,
                                         const SkIRect& rect);

protected:
private:
    GrContext*      fContext;
    SkMatrix        fMatrix;
    SkBitmap        fBM;
    SkDraw          fDraw;
    SkRasterClip    fRasterClip;

    
    
    void sendTextureData(GrTexture *texture, const GrTextureDesc& desc,
                         const void *data, int rowbytes);

    
    
    void compressTextureData(GrTexture *texture, const GrTextureDesc& desc);

    typedef SkNoncopyable INHERITED;
};

#endif 
