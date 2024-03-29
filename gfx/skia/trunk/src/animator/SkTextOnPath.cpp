








#include "SkTextOnPath.h"
#include "SkAnimateMaker.h"
#include "SkCanvas.h"
#include "SkDrawPath.h"
#include "SkDrawText.h"
#include "SkPaint.h"

#if SK_USE_CONDENSED_INFO == 0

const SkMemberInfo SkTextOnPath::fInfo[] = {
    SK_MEMBER(offset, Float),
    SK_MEMBER(path, Path),
    SK_MEMBER(text, Text)
};

#endif

DEFINE_GET_MEMBER(SkTextOnPath);

SkTextOnPath::SkTextOnPath() : offset(0), path(NULL), text(NULL) {
}

bool SkTextOnPath::draw(SkAnimateMaker& maker) {
    SkASSERT(text);
    SkASSERT(path);
    SkBoundableAuto boundable(this, maker);
    maker.fCanvas->drawTextOnPathHV(text->getText(), text->getSize(),
                                    path->getPath(), offset, 0, *maker.fPaint);
    return false;
}
