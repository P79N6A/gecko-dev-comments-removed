







#include "GrClipMaskCache.h"

GrClipMaskCache::GrClipMaskCache()
    : fContext(NULL)
    , fStack(sizeof(GrClipStackFrame)) {
    
    
    SkNEW_PLACEMENT(fStack.push_back(), GrClipStackFrame);
}

void GrClipMaskCache::push() {
    SkNEW_PLACEMENT(fStack.push_back(), GrClipStackFrame);
}

