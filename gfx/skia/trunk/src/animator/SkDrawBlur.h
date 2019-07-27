






#ifndef SkDrawBlur_DEFINED
#define SkDrawBlur_DEFINED

#include "SkPaintPart.h"
#include "SkBlurMaskFilter.h"

class SkDrawBlur : public SkDrawMaskFilter {
    DECLARE_DRAW_MEMBER_INFO(Blur);
    SkDrawBlur();
    virtual SkMaskFilter* getMaskFilter() SK_OVERRIDE;
protected:
    SkScalar fSigma;
    int  fBlurStyle;

    typedef SkDrawMaskFilter INHERITED;
};

#endif 
