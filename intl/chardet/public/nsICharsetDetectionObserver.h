




































#ifndef nsICDETObserver_h__
#define nsICDETObserver_h__

#include "nsISupports.h"
#include "nsDetectionConfident.h"


#define NS_ICHARSETDETECTIONOBSERVER_IID \
{ 0x12bb8f12, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }




class nsICharsetDetectionObserver : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICHARSETDETECTIONOBSERVER_IID)
  NS_IMETHOD Notify(const char* aCharset, nsDetectionConfident aConf) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICharsetDetectionObserver,
                              NS_ICHARSETDETECTIONOBSERVER_IID)

#endif 
