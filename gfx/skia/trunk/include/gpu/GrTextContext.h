






#ifndef GrTextContext_DEFINED
#define GrTextContext_DEFINED

#include "GrContext.h"
#include "GrGlyph.h"
#include "GrPaint.h"

class GrContext;
class GrDrawTarget;
class GrFontScaler;




class GrTextContext {
public:
    virtual void drawPackedGlyph(GrGlyph::PackedID, GrFixed left, GrFixed top,
                                 GrFontScaler*) = 0;

protected:
    GrTextContext(GrContext*, const GrPaint&);
    virtual ~GrTextContext() {}

    GrPaint                fPaint;
    GrContext*             fContext;
    GrDrawTarget*          fDrawTarget;

    SkIRect                fClipRect;

private:
};

#endif
