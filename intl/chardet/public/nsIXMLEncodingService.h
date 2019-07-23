




































#ifndef nsIXMLEncodingService_h__
#define nsIXMLEncodingService_h__
#include "nsISupports.h"



#define NS_IXML_ENCODING_SERVICE_IID \
{ 0x12bb8f11, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


class nsIXMLEncodingService : public nsISupports {
public:
   NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXML_ENCODING_SERVICE_IID)

   NS_IMETHOD Start() = 0;
   NS_IMETHOD End() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXMLEncodingService,
                              NS_IXML_ENCODING_SERVICE_IID)

#endif 
