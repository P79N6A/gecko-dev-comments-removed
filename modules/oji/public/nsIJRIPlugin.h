














































#ifndef nsIJRIPlugin_h___
#define nsIJRIPlugin_h___

#include "nsISupports.h"
#include "jri.h"





class nsIJRIPlugin : public nsISupports {
public:

    

    
    
    NS_IMETHOD_(nsrefcnt)
    GetJRIEnv(JRIEnv* *result) = 0;

    
    
    NS_IMETHOD_(nsrefcnt)
    ReleaseJRIEnv(JRIEnv* env) = 0;

};

#define NS_IJRIPLUGIN_IID                            \
{ /* bfe2d7d0-0164-11d2-815b-006008119d7a */         \
    0xbfe2d7d0,                                      \
    0x0164,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}



#endif 
