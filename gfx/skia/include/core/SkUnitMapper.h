








#ifndef SkUnitMapper_DEFINED
#define SkUnitMapper_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"

#include "SkFlattenable.h"

class SkUnitMapper : public SkFlattenable {
public:
    SkUnitMapper() {}

    

    virtual uint16_t mapUnit16(uint16_t x) = 0;
    
protected:
    SkUnitMapper(SkFlattenableReadBuffer& rb) : SkFlattenable(rb) {}
};

#endif

