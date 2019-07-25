








#ifndef SkClampRange_DEFINED
#define SkClampRange_DEFINED

#include "SkFixed.h"









struct SkClampRange {
    int fCount0;    
    int fCount1;    
    int fCount2;    
    SkFixed fFx1;   
                    
    int fV0, fV1;
    bool fOverflowed;   

    void init(SkFixed fx, SkFixed dx, int count, int v0, int v1);

private:
    void initFor1(SkFixed fx);
};

#endif

