







#include "GrTest.h"

#include "GrGpu.h"
#include "GrResourceCache.h"

void GrTestTarget::init(GrContext* ctx, GrDrawTarget* target) {
    SkASSERT(!fContext);

    fContext.reset(SkRef(ctx));
    fDrawTarget.reset(SkRef(target));

    SkNEW_IN_TLAZY(&fASR, GrDrawTarget::AutoStateRestore, (target, GrDrawTarget::kReset_ASRInit));
    SkNEW_IN_TLAZY(&fACR, GrDrawTarget::AutoClipRestore, (target));
    SkNEW_IN_TLAZY(&fAGP, GrDrawTarget::AutoGeometryPush, (target));
}

void GrContext::getTestTarget(GrTestTarget* tar) {
    this->flush();
    
    
    
    
    tar->init(this, fGpu);
}



void GrContext::setMaxTextureSizeOverride(int maxTextureSizeOverride) {
    fMaxTextureSizeOverride = maxTextureSizeOverride;
}

void GrContext::purgeAllUnlockedResources() {
    fResourceCache->purgeAllUnlocked();
}
