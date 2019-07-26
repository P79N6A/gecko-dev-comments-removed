






#ifndef SkStippleMaskFilter_DEFINED
#define SkStippleMaskFilter_DEFINED

#include "SkMaskFilter.h"




class SkStippleMaskFilter : public SkMaskFilter {
public:
    SkStippleMaskFilter() : INHERITED() {
    }

    virtual bool filterMask(SkMask* dst, const SkMask& src,
                            const SkMatrix& matrix,
                            SkIPoint* margin) SK_OVERRIDE;

    
    virtual SkMask::Format getFormat() SK_OVERRIDE {
        return SkMask::kA8_Format;
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkStippleMaskFilter);

protected:
    SkStippleMaskFilter(SkFlattenableReadBuffer& buffer)
    : SkMaskFilter(buffer) {
    }

private:
    typedef SkMaskFilter INHERITED;
};

#endif 
