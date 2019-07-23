













































#ifndef nsscd_h___
#define nsscd_h___

#include "nsjvm.h"






class NPISymantecDebugManager : public nsISupports {
public:

    NS_IMETHOD_(PRBool)
    SetDebugAgentPassword(PRInt32 pwd) = 0;

};

#define NP_ISYMANTECDEBUGMANAGER_IID                 \
{ /* 131362e0-d985-11d1-8155-006008119d7a */         \
    0x131362e0,                                      \
    0xd985,                                          \
    0x11d1,                                          \
    {0x81, 0x55, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}






enum NPSymantecDebugPort {
    NPSymantecDebugPort_None           = 0,
    NPSymantecDebugPort_SharedMemory   = -1
    
};

class NPISymantecDebugger : public nsISupports {
public:

    NS_IMETHOD_(JVMError)
    StartDebugger(NPSymantecDebugPort port) = 0;

};

#define NP_ISYMANTECDEBUGGER_IID                     \
{ /* 954399f0-d980-11d1-8155-006008119d7a */         \
    0x954399f0,                                      \
    0xd980,                                          \
    0x11d1,                                          \
    {0x81, 0x55, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}


#endif 
