




































#ifndef nsICharsetDetector_h__
#define nsICharsetDetector_h__

#include "nsISupports.h"

class nsICharsetDetectionObserver;


#define NS_ICHARSETDETECTOR_IID \
{ 0x12bb8f14, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

#define NS_CHARSET_DETECTOR_CONTRACTID_BASE "@mozilla.org/intl/charsetdetect;1?type="
#define NS_CHARSET_DETECTOR_CATEGORY "charset-detectors"
 
class nsICharsetDetector : public nsISupports {
public:  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHARSETDETECTOR_IID)

  


  NS_IMETHOD Init(nsICharsetDetectionObserver* observer) = 0;

  









  NS_IMETHOD DoIt(const char* aBytesArray, PRUint32 aLen, PRBool* oDontFeedMe) = 0;

  


  NS_IMETHOD Done() = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICharsetDetector,
                              NS_ICHARSETDETECTOR_IID)

#endif 
