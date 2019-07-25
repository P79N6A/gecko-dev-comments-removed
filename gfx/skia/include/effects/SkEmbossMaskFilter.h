








#ifndef SkEmbossMaskFilter_DEFINED
#define SkEmbossMaskFilter_DEFINED

#include "SkMaskFilter.h"





class SkEmbossMaskFilter : public SkMaskFilter {
public:
    struct Light {
        SkScalar    fDirection[3];  
        uint16_t    fPad;
        uint8_t     fAmbient;
        uint8_t     fSpecular;      
    };

    SkEmbossMaskFilter(const Light& light, SkScalar blurRadius);

    
    
    virtual SkMask::Format getFormat();
    
    virtual bool filterMask(SkMask* dst, const SkMask& src, const SkMatrix&,
                            SkIPoint* margin);

    

    
    virtual Factory getFactory();
    
    virtual void flatten(SkFlattenableWriteBuffer&);

protected:
    SkEmbossMaskFilter(SkFlattenableReadBuffer&);

private:
    Light       fLight;
    SkScalar    fBlurRadius;

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);
    
    typedef SkMaskFilter INHERITED;
};

#endif

