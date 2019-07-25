








#include "SkDrawClip.h"
#include "SkAnimateMaker.h"
#include "SkCanvas.h"
#include "SkDrawRectangle.h"
#include "SkDrawPath.h"


#if SK_USE_CONDENSED_INFO == 0

const SkMemberInfo SkDrawClip::fInfo[] = {
    SK_MEMBER(path, Path),
    SK_MEMBER(rect, Rect)
};

#endif

DEFINE_GET_MEMBER(SkDrawClip);

SkDrawClip::SkDrawClip() : rect(NULL), path(NULL) {
}

bool SkDrawClip::draw(SkAnimateMaker& maker ) {
    if (rect != NULL)
        maker.fCanvas->clipRect(rect->fRect);
    else {
        SkASSERT(path != NULL);
        maker.fCanvas->clipPath(path->fPath);
    }
    return false;
}

