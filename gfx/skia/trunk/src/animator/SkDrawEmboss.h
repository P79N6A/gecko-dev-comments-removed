








#ifndef SkDrawEmboss_DEFINED
#define SkDrawEmboss_DEFINED

#include "SkDrawBlur.h"

class SkDrawEmboss : public SkDrawMaskFilter {
    DECLARE_DRAW_MEMBER_INFO(Emboss);
    SkDrawEmboss();
    virtual SkMaskFilter* getMaskFilter() SK_OVERRIDE;
protected:
    SkTDScalarArray fDirection;
    SkScalar        fSigma;
    SkScalar        fAmbient;
    SkScalar        fSpecular;

    typedef SkDrawMaskFilter INHERITED;
};

#endif 
