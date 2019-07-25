








#ifndef GrPathRendererChain_DEFINED
#define GrPathRendererChain_DEFINED

#include "GrDrawTarget.h"
#include "GrRefCnt.h"
#include "SkTArray.h"

class GrContext;

class SkPath;
class GrPathRenderer;







class GrPathRendererChain : public SkRefCnt {
public:

    enum UsageFlags {
        kNone_UsageFlag      = 0,
        kNonAAOnly_UsageFlag = 1,
    };

    GrPathRendererChain(GrContext* context, UsageFlags flags);

    ~GrPathRendererChain();

    
    GrPathRenderer* addPathRenderer(GrPathRenderer* pr);

    GrPathRenderer* getPathRenderer(const GrDrawTarget::Caps& targetCaps,
                                    const SkPath& path,
                                    GrPathFill fill,
                                    bool antiAlias);

private:

    GrPathRendererChain();

    void init();

    enum {
        kPreAllocCount = 8,
    };
    bool fInit;
    GrContext*                                          fOwner;
    UsageFlags                                          fFlags;
    SkSTArray<kPreAllocCount, GrPathRenderer*, true>    fChain;
};

GR_MAKE_BITFIELD_OPS(GrPathRendererChain::UsageFlags)

#endif
