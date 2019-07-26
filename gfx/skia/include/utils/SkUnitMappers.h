








#ifndef SkUnitMappers_DEFINED
#define SkUnitMappers_DEFINED

#include "SkUnitMapper.h"



class SkDiscreteMapper : public SkUnitMapper {
public:
    SkDiscreteMapper(int segments);
    
    virtual uint16_t mapUnit16(uint16_t x);

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkDiscreteMapper)

protected:
    SkDiscreteMapper(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    int     fSegments;
    SkFract fScale;    

    typedef SkUnitMapper INHERITED;
};




class SkCosineMapper : public SkUnitMapper {
public:
    SkCosineMapper() {}
    
    virtual uint16_t mapUnit16(uint16_t x);

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkCosineMapper)

protected:
    SkCosineMapper(SkFlattenableReadBuffer&);

private:

    typedef SkUnitMapper INHERITED;
};

#endif
