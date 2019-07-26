






#ifndef SkEmbossMaskFilter_DEFINED
#define SkEmbossMaskFilter_DEFINED

#include "SkMaskFilter.h"





class SK_API SkEmbossMaskFilter : public SkMaskFilter {
public:
    struct Light {
        SkScalar    fDirection[3];  
        uint16_t    fPad;
        uint8_t     fAmbient;
        uint8_t     fSpecular;      
    };

    SkEmbossMaskFilter(const Light& light, SkScalar blurRadius);

    
    
    virtual SkMask::Format getFormat() const SK_OVERRIDE;
    
    virtual bool filterMask(SkMask* dst, const SkMask& src, const SkMatrix&,
                            SkIPoint* margin) const SK_OVERRIDE;

    SkDEVCODE(virtual void toString(SkString* str) const SK_OVERRIDE;)
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkEmbossMaskFilter)

protected:
    SkEmbossMaskFilter(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    Light       fLight;
    SkScalar    fBlurRadius;

    typedef SkMaskFilter INHERITED;
};

#endif
