








#ifndef SkPaintFlagsDrawFilter_DEFINED
#define SkPaintFlagsDrawFilter_DEFINED

#include "SkDrawFilter.h"

class SkPaintFlagsDrawFilter : public SkDrawFilter {
public:
    SkPaintFlagsDrawFilter(uint32_t clearFlags, uint32_t setFlags);
    
    
    virtual void filter(SkPaint*, Type);
    
private:
    uint32_t    fPrevFlags;     
    uint16_t    fClearFlags;    
    uint16_t    fSetFlags;      
};

#endif

