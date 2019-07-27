






#include "SkForceLinking.h"
#include "SkImageDecoder.h"




int SkForceLinking(bool doNotPassTrue) {
    if (doNotPassTrue) {
        SkASSERT(false);
        CreateJPEGImageDecoder();
        CreateWEBPImageDecoder();
        CreateBMPImageDecoder();
        CreateICOImageDecoder();
        CreatePKMImageDecoder();
        CreateKTXImageDecoder();
        CreateWBMPImageDecoder();
        
#if !defined(SK_BUILD_FOR_MAC) && !defined(SK_BUILD_FOR_WIN) && !defined(SK_BUILD_FOR_NACL) \
        && !defined(SK_BUILD_FOR_IOS)
        CreateGIFImageDecoder();
#endif
#if !defined(SK_BUILD_FOR_MAC) && !defined(SK_BUILD_FOR_WIN) && !defined(SK_BUILD_FOR_IOS)
        CreatePNGImageDecoder();
#endif
#if defined(SK_BUILD_FOR_IOS)
        CreatePNGImageEncoder_IOS();
#endif
        return -1;
    }
    return 0;
}
