






#ifndef SkPaintFlagsDrawFilter_DEFINED
#define SkPaintFlagsDrawFilter_DEFINED

#include "SkDrawFilter.h"

class SK_API SkPaintFlagsDrawFilter : public SkDrawFilter {
public:
    SkPaintFlagsDrawFilter(uint32_t clearFlags, uint32_t setFlags);

    virtual bool filter(SkPaint*, Type) SK_OVERRIDE;

private:
    uint16_t    fClearFlags;    
    uint16_t    fSetFlags;      
};

#endif
