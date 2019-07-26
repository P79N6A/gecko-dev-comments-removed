







#ifndef GrTest_DEFINED
#define GrTest_DEFINED

#include "GrContext.h"
#include "GrDrawTarget.h"




class GrTestTarget {
public:
    GrTestTarget() {};

    void init(GrContext*, GrDrawTarget*);

    GrDrawTarget* target() { return fDrawTarget.get(); }

private:
    SkTLazy<GrDrawTarget::AutoStateRestore> fASR;
    SkTLazy<GrDrawTarget::AutoClipRestore>  fACR;
    SkTLazy<GrDrawTarget::AutoGeometryPush> fAGP;

    SkAutoTUnref<GrDrawTarget>              fDrawTarget;
    SkAutoTUnref<GrContext>                 fContext;
};

#endif
