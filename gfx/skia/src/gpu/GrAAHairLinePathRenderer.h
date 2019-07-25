







#ifndef GrAAHairLinePathRenderer_DEFINED
#define GrAAHairLinePathRenderer_DEFINED

#include "GrPathRenderer.h"

class GrAAHairLinePathRenderer : public GrPathRenderer {
public:
    virtual ~GrAAHairLinePathRenderer();

    static GrPathRenderer* Create(GrContext* context);
    
    virtual bool canDrawPath(const GrDrawTarget::Caps& targetCaps,
                             const SkPath& path,
                             GrPathFill fill,
                             bool antiAlias) const  SK_OVERRIDE;
    virtual void drawPath(GrDrawState::StageMask stages) SK_OVERRIDE;

protected:

    
    virtual void pathWillClear()  SK_OVERRIDE;

private:
    void resetGeom();

    GrAAHairLinePathRenderer(const GrContext* context,
                             const GrIndexBuffer* fLinesIndexBuffer,
                             const GrIndexBuffer* fQuadsIndexBuffer);

    bool createGeom(GrDrawState::StageMask stages);

    const GrIndexBuffer*        fLinesIndexBuffer;
    const GrIndexBuffer*        fQuadsIndexBuffer;

    
    GrDrawState::StageMask      fPreviousStages;
    int                         fPreviousRTHeight;
    SkVector                    fPreviousTranslate;
    GrIRect                     fClipRect;

    
    GrMatrix                    fPreviousViewMatrix;
    int                         fLineSegmentCnt;
    int                         fQuadCnt;

    typedef GrPathRenderer INHERITED;
};


#endif

