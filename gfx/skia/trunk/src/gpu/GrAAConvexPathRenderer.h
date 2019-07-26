







#include "GrPathRenderer.h"


class GrAAConvexPathRenderer : public GrPathRenderer {
public:
    GrAAConvexPathRenderer();

    virtual bool canDrawPath(const SkPath& path,
                             const SkStrokeRec& stroke,
                             const GrDrawTarget* target,
                             bool antiAlias) const SK_OVERRIDE;

protected:
    virtual bool onDrawPath(const SkPath& path,
                            const SkStrokeRec& stroke,
                            GrDrawTarget* target,
                            bool antiAlias) SK_OVERRIDE;
};
