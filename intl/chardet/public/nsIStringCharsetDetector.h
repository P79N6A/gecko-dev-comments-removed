



































#ifndef nsIStringCharsetDetector_h__
#define nsIStringCharsetDetector_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsDetectionConfident.h"


#define NS_ISTRINGCHARSETDETECTOR_IID \
{ 0x12bb8f15, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_STRCDETECTOR_CONTRACTID_BASE "@mozilla.org/intl/stringcharsetdetect;1?type="








class nsIStringCharsetDetector : public nsISupports {
public:  

   NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTRINGCHARSETDETECTOR_IID)
  







  NS_IMETHOD DoIt(const char* aBytesArray, PRUint32 aLen, 
                    const char** oCharset, nsDetectionConfident &oConfident) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStringCharsetDetector,
                              NS_ISTRINGCHARSETDETECTOR_IID)

#endif 
