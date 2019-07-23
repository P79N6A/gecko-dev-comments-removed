














































#ifndef nsIJVMConsole_h___
#define nsIJVMConsole_h___

#include "nsIJVMWindow.h"

#define NS_IJVMCONSOLE_IID                           \
{ /* fefaf860-6220-11d2-8164-006008119d7a */         \
    0xfefaf860,                                      \
    0x6220,                                          \
    0x11d2,                                          \
    {0x81, 0x64, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}






class nsIJVMConsole : public nsIJVMWindow {
public:
    NS_IMETHOD
    Show(void) = 0;

    NS_IMETHOD
    Hide(void) = 0;

    NS_IMETHOD
    IsVisible(PRBool *result) = 0;
    
    
    
    
    NS_IMETHOD
    Print(const char* msg, const char* encodingName = NULL) = 0;
    
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJVMCONSOLE_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJVMConsole, NS_IJVMCONSOLE_IID)



#endif
