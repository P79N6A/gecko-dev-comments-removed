






#ifndef SkStippleMaskFilter_DEFINED
#define SkStippleMaskFilter_DEFINED

#include "SkMaskFilter.h"




class SK_API SkStippleMaskFilter : public SkMaskFilter {
public:
    SkStippleMaskFilter() : INHERITED() {
    }

    virtual bool filterMask(SkMask* dst, const SkMask& src,
                            const SkMatrix& matrix,
                            SkIPoint* margin) const SK_OVERRIDE;

    
    virtual SkMask::Format getFormat() const SK_OVERRIDE {
        return SkMask::kA8_Format;
    }

    SkDEVCODE(virtual void toString(SkString* str) const SK_OVERRIDE;)
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkStippleMaskFilter);

protected:
    SkStippleMaskFilter(SkFlattenableReadBuffer& buffer)
    : SkMaskFilter(buffer) {
    }

private:
    typedef SkMaskFilter INHERITED;
};

#endif 
