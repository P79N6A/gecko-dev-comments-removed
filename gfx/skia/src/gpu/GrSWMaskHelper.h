






#ifndef GrSWMaskHelper_DEFINED
#define GrSWMaskHelper_DEFINED

#include "GrColor.h"
#include "GrMatrix.h"
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
class GrDrawTarget;















class GrSWMaskHelper : public GrNoncopyable {
public:
    GrSWMaskHelper(GrContext* context)
    : fContext(context) {
    }

    
    
    
    
    bool init(const GrIRect& resultBounds, const GrMatrix* matrix);

    
    void draw(const GrRect& rect, SkRegion::Op op,
              bool antiAlias, uint8_t alpha);

    
    void draw(const SkPath& path, SkRegion::Op op,
              GrPathFill fill, bool antiAlias, uint8_t alpha);

    
    
    bool getTexture(GrAutoScratchTexture* texture);

    
    
    void toTexture(GrTexture* texture, uint8_t alpha);

    
    void clear(uint8_t alpha) {
        fBM.eraseColor(SkColorSetARGB(alpha, alpha, alpha, alpha));
    }

    
    
    static GrTexture* DrawPathMaskToTexture(GrContext* context,
                                            const SkPath& path,
                                            const GrIRect& resultBounds,
                                            GrPathFill fill,
                                            bool antiAlias,
                                            GrMatrix* matrix);

    
    
    
    
    
    
    
    
    
    
    static void DrawToTargetWithPathMask(GrTexture* texture,
                                         GrDrawTarget* target,
                                         const GrIRect& rect);

protected:
private:
    GrContext*      fContext;
    GrMatrix        fMatrix;
    SkBitmap        fBM;
    SkDraw          fDraw;
    SkRasterClip    fRasterClip;

    typedef GrNoncopyable INHERITED;
};

#endif 
