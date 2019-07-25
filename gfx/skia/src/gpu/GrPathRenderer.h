








#ifndef GrPathRenderer_DEFINED
#define GrPathRenderer_DEFINED

#include "GrDrawTarget.h"
#include "GrPathRendererChain.h"

#include "SkTArray.h"

class SkPath;

struct GrPoint;












class GR_API GrPathRenderer : public GrRefCnt {
public:

    










    static void AddPathRenderers(GrContext* context,
                                 GrPathRendererChain::UsageFlags flags,
                                 GrPathRendererChain* prChain);


    GrPathRenderer(void);
    













    virtual bool canDrawPath(const GrDrawTarget::Caps& targetCaps,
                             const SkPath& path,
                             GrPathFill fill,
                             bool antiAlias) const = 0;

    




















    virtual bool requiresStencilPass(const GrDrawTarget* target,
                                     const SkPath& path,
                                     GrPathFill fill) const { return false; }

    


















    void setPath(GrDrawTarget* target,
                 const SkPath* path,
                 GrPathFill fill,
                 bool antiAlias,
                 const GrPoint* translate);

    


    void clearPath();

    












    virtual void drawPath(GrDrawTarget::StageBitfield stages) = 0;

    












    virtual void drawPathToStencil() {
        GrCrash("Unexpected call to drawPathToStencil.");
    }

    


    class AutoClearPath {
    public:
        AutoClearPath() {
            fPathRenderer = NULL;
        }
        AutoClearPath(GrPathRenderer* pr,
                      GrDrawTarget* target,
                      const SkPath* path,
                      GrPathFill fill,
                      bool antiAlias,
                      const GrPoint* translate) {
            GrAssert(NULL != pr);
            pr->setPath(target, path, fill, antiAlias, translate);
            fPathRenderer = pr;
        }
        void set(GrPathRenderer* pr,
                 GrDrawTarget* target,
                 const SkPath* path,
                 GrPathFill fill,
                 bool antiAlias,
                 const GrPoint* translate) {
            if (NULL != fPathRenderer) {
                fPathRenderer->clearPath();
            }
            GrAssert(NULL != pr);
            pr->setPath(target, path, fill, antiAlias, translate);
            fPathRenderer = pr;
        }
        ~AutoClearPath() {
            if (NULL != fPathRenderer) {
                fPathRenderer->clearPath();
            }
        }
    private:
        GrPathRenderer* fPathRenderer;
    };

protected:

    
    
    virtual void pathWasSet() {}
    virtual void pathWillClear() {}

    const SkPath*               fPath;
    GrDrawTarget*               fTarget;
    GrPathFill                  fFill;
    GrPoint                     fTranslate;
    bool                        fAntiAlias;

private:

    typedef GrRefCnt INHERITED;
};

#endif

