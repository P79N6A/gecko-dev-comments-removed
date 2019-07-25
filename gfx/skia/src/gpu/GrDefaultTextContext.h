









#ifndef GrDefaultTextContext_DEFINED
#define GrDefaultTextContext_DEFINED

#include "GrBatchedTextContext.h"

struct GrGpuTextVertex;
class GrTextStrike;

class GrDefaultTextContext: public GrBatchedTextContext {
public:
    GrDefaultTextContext();
    ~GrDefaultTextContext();

    virtual void drawPackedGlyph(GrGlyph::PackedID, GrFixed left, GrFixed top,
                                 GrFontScaler*) SK_OVERRIDE;

protected:
    virtual void flush() SK_OVERRIDE;
    virtual void init(GrContext* context, const GrPaint&,
                      const GrMatrix* extMatrix) SK_OVERRIDE;
    virtual void finish() SK_OVERRIDE;

private:
    GrVertexLayout   fVertexLayout;
    GrGpuTextVertex* fVertices;
    GrIRect          fClipRect;

    GrTextStrike*   fStrike;

    GrMatrix    fExtMatrix;
    GrMatrix    fOrigViewMatrix;    

    inline void flushGlyphs();

    typedef GrBatchedTextContext INHERITED;
};

#endif
