





































#ifndef MyService_h__
#define MyService_h__

#include "nsISupports.h"

#define NS_IMYSERVICE_IID                            \
{ /* fedc3380-3648-11d2-8163-006008119d7a */         \
    0xfedc3380,                                      \
    0x3648,                                          \
    0x11d2,                                          \
    {0x81, 0x63, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

class IMyService : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMYSERVICE_IID)
    
    NS_IMETHOD
    Doit(void) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(IMyService, NS_IMYSERVICE_IID)

#define NS_IMYSERVICE_CID                            \
{ /* 34876550-364b-11d2-8163-006008119d7a */         \
    0x34876550,                                      \
    0x364b,                                          \
    0x11d2,                                          \
    {0x81, 0x63, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

#endif 
