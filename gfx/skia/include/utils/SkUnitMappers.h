








#ifndef SkUnitMappers_DEFINED
#define SkUnitMappers_DEFINED

#include "SkUnitMapper.h"



class SkDiscreteMapper : public SkUnitMapper {
public:
    SkDiscreteMapper(int segments);
    
    virtual uint16_t mapUnit16(uint16_t x);

protected:
    SkDiscreteMapper(SkFlattenableReadBuffer& );
    
    virtual void flatten(SkFlattenableWriteBuffer& );
    virtual Factory getFactory();
private:
    int     fSegments;
    SkFract fScale;    

    static SkFlattenable* Create(SkFlattenableReadBuffer& buffer);
    
    typedef SkUnitMapper INHERITED;
};




class SkCosineMapper : public SkUnitMapper {
public:
    SkCosineMapper() {}
    
    virtual uint16_t mapUnit16(uint16_t x);

protected:
    SkCosineMapper(SkFlattenableReadBuffer&);
    
    virtual Factory getFactory();

private:
    static SkFlattenable* Create(SkFlattenableReadBuffer&);

    typedef SkUnitMapper INHERITED;
};

#endif

