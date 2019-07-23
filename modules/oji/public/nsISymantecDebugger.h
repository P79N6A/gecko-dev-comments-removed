













































#ifndef nsISymantecDebugger_h___
#define nsISymantecDebugger_h___

#include "nsISupports.h"






enum nsSymantecDebugPort {
    nsSymantecDebugPort_None           = 0,
    nsSymantecDebugPort_SharedMemory   = -1
    
};

class nsISymantecDebugger : public nsISupports {
public:

    NS_IMETHOD
    StartDebugger(nsSymantecDebugPort port) = 0;

};

#define NS_ISYMANTECDEBUGGER_IID                     \
{ /* 954399f0-d980-11d1-8155-006008119d7a */         \
    0x954399f0,                                      \
    0xd980,                                          \
    0x11d1,                                          \
    {0x81, 0x55, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}



#endif 
