



































#ifndef nsICDETAdaptor_h__
#define nsICDETAdaptor_h__
#include "nsISupports.h"

class nsICharsetDetector;
class nsIWebShellServices;
class nsIDocument;
class nsIParser;


#define NS_ICHARSETDETECTIONADAPTOR_IID \
{ 0x12bb8f13, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }









class nsICharsetDetectionAdaptor : public nsISupports {
public:  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHARSETDETECTIONADAPTOR_IID)
  



  NS_IMETHOD Init(nsIWebShellServices* aWebShell, nsICharsetDetector *aDetector, 
                  nsIDocument* aDocument, nsIParser* aParser, 
                  const char* aCharset, const char* aCommand=nsnull) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICharsetDetectionAdaptor,
                              NS_ICHARSETDETECTIONADAPTOR_IID)

#endif 
