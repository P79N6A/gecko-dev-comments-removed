













































#ifndef nsISymantecDebugManager_h___
#define nsISymantecDebugManager_h___

#include "nsISupports.h"






#define NS_ISYMANTECDEBUGMANAGER_IID                 \
{ /* 131362e0-d985-11d1-8155-006008119d7a */         \
    0x131362e0,                                      \
    0xd985,                                          \
    0x11d1,                                          \
    {0x81, 0x55, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

class nsISymantecDebugManager : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISYMANTECDEBUGMANAGER_IID)

    NS_IMETHOD
    SetDebugAgentPassword(PRInt32 pwd) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISymantecDebugManager,
                              NS_ISYMANTECDEBUGMANAGER_IID)



#endif 
