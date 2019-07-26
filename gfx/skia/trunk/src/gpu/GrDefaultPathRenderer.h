






#ifndef GrDefaultPathRenderer_DEFINED
#define GrDefaultPathRenderer_DEFINED

#include "GrPathRenderer.h"
#include "SkTemplates.h"





class SK_API GrDefaultPathRenderer : public GrPathRenderer {
public:
    GrDefaultPathRenderer(bool separateStencilSupport, bool stencilWrapOpsSupport);

    virtual bool canDrawPath(const SkPath&,
                             const SkStrokeRec&,
                             const GrDrawTarget*,
                             bool antiAlias) const SK_OVERRIDE;

private:

    virtual StencilSupport onGetStencilSupport(const SkPath&,
                                               const SkStrokeRec&,
                                               const GrDrawTarget*) const SK_OVERRIDE;

    virtual bool onDrawPath(const SkPath&,
                            const SkStrokeRec&,
                            GrDrawTarget*,
                            bool antiAlias) SK_OVERRIDE;

    virtual void onStencilPath(const SkPath&,
                               const SkStrokeRec&,
                               GrDrawTarget*) SK_OVERRIDE;

    bool internalDrawPath(const SkPath&,
                          const SkStrokeRec&,
                          GrDrawTarget*,
                          bool stencilOnly);

    bool createGeom(const SkPath&,
                    const SkStrokeRec&,
                    SkScalar srcSpaceTol,
                    GrDrawTarget*,
                    GrPrimitiveType*,
                    int* vertexCnt,
                    int* indexCnt,
                    GrDrawTarget::AutoReleaseGeometry*);

    bool    fSeparateStencil;
    bool    fStencilWrapOps;

    typedef GrPathRenderer INHERITED;
};

#endif
