






#ifndef SkBlurMaskFilter_DEFINED
#define SkBlurMaskFilter_DEFINED


#include "SkMaskFilter.h"
#include "SkScalar.h"
#include "SkBlurTypes.h"

class SK_API SkBlurMaskFilter {
public:
    



    static SkScalar ConvertRadiusToSigma(SkScalar radius);

    enum BlurFlags {
        kNone_BlurFlag = 0x00,
        
        kIgnoreTransform_BlurFlag   = 0x01,
        
        kHighQuality_BlurFlag       = 0x02,
        
        kAll_BlurFlag = 0x03
    };

    





    static SkMaskFilter* Create(SkBlurStyle style, SkScalar sigma, uint32_t flags = kNone_BlurFlag);

    







    static SkMaskFilter* CreateEmboss(SkScalar blurSigma, const SkScalar direction[3],
                                      SkScalar ambient, SkScalar specular);

    SK_ATTR_DEPRECATED("use sigma version")
    static SkMaskFilter* CreateEmboss(const SkScalar direction[3],
                                      SkScalar ambient, SkScalar specular,
                                      SkScalar blurRadius);

    SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()

private:
    SkBlurMaskFilter(); 
};

#endif
