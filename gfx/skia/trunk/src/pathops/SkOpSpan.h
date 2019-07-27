





#ifndef SkOpSpan_DEFINED
#define SkOpSpan_DEFINED

#include "SkPoint.h"

class SkOpAngle;
class SkOpSegment;

struct SkOpSpan {
    SkPoint fPt;  
    double fT;
    double fOtherT;  
    SkOpSegment* fOther;
    SkOpAngle* fFromAngle;  
    SkOpAngle* fToAngle;  
    int fOtherIndex;  
    int fWindSum;  
    int fOppSum;  
    int fWindValue;  
    int fOppValue;  
    bool fChased;  
    bool fCoincident;  
    bool fDone;  
    bool fLoop;  
    bool fMultiple;  
    bool fNear;  
    bool fSmall;   
    bool fTiny;  

    
    const SkOpSegment* debugToSegment(ptrdiff_t* ) const;
    void dump() const;
    void dumpOne() const;
};

#endif
