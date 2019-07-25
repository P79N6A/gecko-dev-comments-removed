









#ifndef GrBatchedTextContext_DEFINED
#define GrBatchedTextContext_DEFINED

#include "GrPaint.h"
#include "GrTextContext.h"

class GrDrawTarget;
class GrTexture;








class GrBatchedTextContext: public GrTextContext {
public:
    virtual ~GrBatchedTextContext();

protected:
    enum {
        kMinRequestedGlyphs      = 1,
        kDefaultRequestedGlyphs  = 64,
        kMinRequestedVerts       = kMinRequestedGlyphs * 4,
        kDefaultRequestedVerts   = kDefaultRequestedGlyphs * 4,
        kGlyphMaskStage          = GrPaint::kTotalStages,
    };

    GrPaint         fGrPaint;
    GrDrawTarget*   fDrawTarget;

    int32_t     fMaxVertices;
    GrTexture*  fCurrTexture;
    int         fCurrVertex;

    GrBatchedTextContext();
    virtual void init(GrContext* context, const GrPaint&,
                      const GrMatrix* extMatrix) SK_OVERRIDE;
    virtual void finish() SK_OVERRIDE;

    




    void prepareForGlyph(GrTexture*);

    



    virtual void flush() = 0;

    




    void setupVertexBuff(void** vertexBuff, GrVertexLayout vertexLayout);

    


    void reset();

private:

    typedef GrTextContext INHERITED;
};

#endif
