








#include "SkDrawDash.h"
#include "SkDashPathEffect.h"

#if SK_USE_CONDENSED_INFO == 0

const SkMemberInfo SkDash::fInfo[] = {
    SK_MEMBER_ARRAY(intervals, Float),
    SK_MEMBER(phase, Float)
};

#endif

DEFINE_GET_MEMBER(SkDash);

SkDash::SkDash() : phase(0) {
}

SkDash::~SkDash() {
}

SkPathEffect* SkDash::getPathEffect() {
    int count = intervals.count();
    if (count == 0)
        return NULL;
    return SkDashPathEffect::Create(intervals.begin(), count, phase);
}
