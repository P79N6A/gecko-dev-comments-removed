




































#ifndef nsISplashScreen_h__
#define nsISplashScreen_h__

#include "nsISupports.h"


#define NS_ISPLASHSCREEN_IID \
 { 0xb5030250, 0xd530, 0x11d3, { 0x80, 0x70, 0x0, 0x60, 0x8, 0x11, 0xa9, 0xc3 } }



















class nsISplashScreen : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISPLASHSCREEN_IID)
    NS_IMETHOD Show() = 0;
    NS_IMETHOD Hide() = 0;
}; 

NS_DEFINE_STATIC_IID_ACCESSOR(nsISplashScreen, NS_ISPLASHSCREEN_IID)

#endif 
