








#ifndef SkUnitMapper_DEFINED
#define SkUnitMapper_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"

#include "SkFlattenable.h"

class SkUnitMapper : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkUnitMapper)

    SkUnitMapper() {}

    

    virtual uint16_t mapUnit16(uint16_t x) = 0;

protected:
    SkUnitMapper(SkFlattenableReadBuffer& rb) : SkFlattenable(rb) {}

private:
    typedef SkFlattenable INHERITED;
};

#endif

