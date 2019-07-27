






#ifndef SkStippleMaskFilter_DEFINED
#define SkStippleMaskFilter_DEFINED

#include "SkMaskFilter.h"




class SK_API SkStippleMaskFilter : public SkMaskFilter {
public:
    static SkStippleMaskFilter* Create() {
        return SkNEW(SkStippleMaskFilter);
    }

    virtual bool filterMask(SkMask* dst, const SkMask& src,
                            const SkMatrix& matrix,
                            SkIPoint* margin) const SK_OVERRIDE;

    
    virtual SkMask::Format getFormat() const SK_OVERRIDE {
        return SkMask::kA8_Format;
    }

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkStippleMaskFilter);

protected:
    SkStippleMaskFilter() : INHERITED() {
    }
    explicit SkStippleMaskFilter(SkReadBuffer& buffer)
        : SkMaskFilter(buffer) {
    }

private:
    typedef SkMaskFilter INHERITED;
};

#endif 
