






#ifndef SkBlurTypes_DEFINED
#define SkBlurTypes_DEFINED

#include "SkTypes.h"

enum SkBlurStyle {
    kNormal_SkBlurStyle,  
    kSolid_SkBlurStyle,   
    kOuter_SkBlurStyle,   
    kInner_SkBlurStyle,   

    kLastEnum_SkBlurStyle = kInner_SkBlurStyle
};

enum SkBlurQuality {
    kLow_SkBlurQuality,     
    kHigh_SkBlurQuality,    

    kLastEnum_SkBlurQuality
};

#endif
