





#ifndef SkOpSpan_DEFINED
#define SkOpSpan_DEFINED

#include "SkPoint.h"

class SkOpSegment;

struct SkOpSpan {
    SkOpSegment* fOther;
    SkPoint fPt;  
    double fT;
    double fOtherT;  
    int fOtherIndex;  
    int fWindSum;  
    int fOppSum;  
    int fWindValue;  
    int fOppValue;  
    bool fDone;  
    bool fUnsortableStart;  
    bool fUnsortableEnd;  
    bool fSmall;   
    bool fTiny;  
    bool fLoop;  

#ifdef SK_DEBUG
    void dump() const;
#endif
};

#endif
