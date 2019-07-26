






#ifndef GrBitmapTextContext_DEFINED
#define GrBitmapTextContext_DEFINED

#include "GrTextContext.h"

class GrTextStrike;




class GrBitmapTextContext : public GrTextContext {
public:
    GrBitmapTextContext(GrContext*, const GrPaint&, SkColor);
    virtual ~GrBitmapTextContext();

    virtual void drawPackedGlyph(GrGlyph::PackedID, GrFixed left, GrFixed top,
                         GrFontScaler*) SK_OVERRIDE;

private:
    GrContext::AutoMatrix  fAutoMatrix;
    GrTextStrike*          fStrike;

    void flushGlyphs();                 

    enum {
        kMinRequestedGlyphs      = 1,
        kDefaultRequestedGlyphs  = 64,
        kMinRequestedVerts       = kMinRequestedGlyphs * 4,
        kDefaultRequestedVerts   = kDefaultRequestedGlyphs * 4,
    };

    SkColor                 fSkPaintColor;
    SkPoint*                fVertices;
    int32_t                 fMaxVertices;
    GrTexture*              fCurrTexture;
    int                     fCurrVertex;
};

#endif
