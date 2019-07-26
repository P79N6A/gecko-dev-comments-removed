






#ifndef GrSWMaskHelper_DEFINED
#define GrSWMaskHelper_DEFINED

#include "GrColor.h"
#include "SkMatrix.h"
#include "GrNoncopyable.h"
#include "SkBitmap.h"
#include "SkDraw.h"
#include "SkRasterClip.h"
#include "SkRegion.h"
#include "GrDrawState.h"

class GrAutoScratchTexture;
class GrContext;
class GrTexture;
class SkPath;
class SkStrokeRec;
class GrDrawTarget;















class GrSWMaskHelper : public GrNoncopyable {
public:
    GrSWMaskHelper(GrContext* context)
    : fContext(context) {
    }

    
    
    
    
    bool init(const GrIRect& resultBounds, const SkMatrix* matrix);

    
    void draw(const GrRect& rect, SkRegion::Op op,
              bool antiAlias, uint8_t alpha);

    
    void draw(const SkPath& path, const SkStrokeRec& stroke, SkRegion::Op op,
              bool antiAlias, uint8_t alpha);

    
    
    bool getTexture(GrAutoScratchTexture* texture);

    
    
    void toTexture(GrTexture* texture, uint8_t alpha);

    
    void clear(uint8_t alpha) {
        fBM.eraseColor(SkColorSetARGB(alpha, alpha, alpha, alpha));
    }

    
    
    static GrTexture* DrawPathMaskToTexture(GrContext* context,
                                            const SkPath& path,
                                            const SkStrokeRec& stroke,
                                            const GrIRect& resultBounds,
                                            bool antiAlias,
                                            SkMatrix* matrix);

    
    
    
    
    
    
    
    
    
    
    static void DrawToTargetWithPathMask(GrTexture* texture,
                                         GrDrawTarget* target,
                                         const GrIRect& rect);

protected:
private:
    GrContext*      fContext;
    SkMatrix        fMatrix;
    SkBitmap        fBM;
    SkDraw          fDraw;
    SkRasterClip    fRasterClip;

    typedef GrNoncopyable INHERITED;
};

#endif 
