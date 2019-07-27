






#ifndef GrBitmapTextContext_DEFINED
#define GrBitmapTextContext_DEFINED

#include "GrTextContext.h"

class GrTextStrike;




class GrBitmapTextContext : public GrTextContext {
public:
    GrBitmapTextContext(GrContext*, const SkDeviceProperties&);
    virtual ~GrBitmapTextContext();

    virtual void drawText(const GrPaint&, const SkPaint&, const char text[], size_t byteLength,
                          SkScalar x, SkScalar y) SK_OVERRIDE;
    virtual void drawPosText(const GrPaint&, const SkPaint&,
                             const char text[], size_t byteLength,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPosition) SK_OVERRIDE;

    virtual bool canDraw(const SkPaint& paint) SK_OVERRIDE;

private:
    GrTextStrike*          fStrike;

    void init(const GrPaint&, const SkPaint&);
    void drawPackedGlyph(GrGlyph::PackedID, SkFixed left, SkFixed top, GrFontScaler*);
    void flushGlyphs();                 
    void finish();

    enum {
        kMinRequestedGlyphs      = 1,
        kDefaultRequestedGlyphs  = 64,
        kMinRequestedVerts       = kMinRequestedGlyphs * 4,
        kDefaultRequestedVerts   = kDefaultRequestedGlyphs * 4,
    };

    void*                       fVertices;
    SkAutoTUnref<GrEffect>      fCachedEffect;
    
    uint32_t                    fEffectTextureUniqueID;
    int                         fCurrVertex;
    SkRect                      fVertexBounds;
};

#endif
