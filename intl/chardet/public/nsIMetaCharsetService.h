




































#ifndef nsIMetaCharsetService_h__
#define nsIMetaCharsetService_h__
#include "nsISupports.h"



#define NS_IMETA_CHARSET_SERVICE_IID \
{ 0x218f2ac1, 0xa48, 0x11d3, { 0xb3, 0xba, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

class nsIMetaCharsetService : public nsISupports {
public:
   NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMETA_CHARSET_SERVICE_IID)

   NS_IMETHOD Start() = 0;
   NS_IMETHOD End() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMetaCharsetService,
                              NS_IMETA_CHARSET_SERVICE_IID)

#endif 
