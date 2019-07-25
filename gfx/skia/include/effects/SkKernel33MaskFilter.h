








#ifndef SkKernel33MaskFilter_DEFINED
#define SkKernel33MaskFilter_DEFINED

#include "SkMaskFilter.h"

class SkKernel33ProcMaskFilter : public SkMaskFilter {
public:
    SkKernel33ProcMaskFilter(unsigned percent256 = 256)
        : fPercent256(percent256) {}

    virtual uint8_t computeValue(uint8_t* const* srcRows) = 0;
    
    
    virtual SkMask::Format getFormat();
    virtual bool filterMask(SkMask*, const SkMask&, const SkMatrix&, SkIPoint*);

    
    virtual void flatten(SkFlattenableWriteBuffer& wb);

protected:
    SkKernel33ProcMaskFilter(SkFlattenableReadBuffer& rb);

private:
    int fPercent256;
    
    typedef SkMaskFilter INHERITED;
};



class SkKernel33MaskFilter : public SkKernel33ProcMaskFilter {
public:
    SkKernel33MaskFilter(const int coeff[3][3], int shift, int percent256 = 256)
            : SkKernel33ProcMaskFilter(percent256) {
        memcpy(fKernel, coeff, 9 * sizeof(int));
        fShift = shift;
    }
    
    
    virtual uint8_t computeValue(uint8_t* const* srcRows);
    
    
    virtual void flatten(SkFlattenableWriteBuffer& wb);
    virtual Factory getFactory();
    
private:
    int fKernel[3][3];
    int fShift;

    SkKernel33MaskFilter(SkFlattenableReadBuffer& rb);
    static SkFlattenable* Create(SkFlattenableReadBuffer& rb);
    
    typedef SkKernel33ProcMaskFilter INHERITED;
};

#endif
