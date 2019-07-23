














































#ifndef nsIJVMWindow_h___
#define nsIJVMWindow_h___

#include "nsISupports.h"







class nsIJVMWindow : public nsISupports {
public:
    
    

    NS_IMETHOD
    Show(void) = 0;

    NS_IMETHOD
    Hide(void) = 0;

    NS_IMETHOD
    IsVisible(PRBool *result) = 0;
    
};

#define NS_IJVMWINDOW_IID                            \
{ /* d981b540-6220-11d2-8164-006008119d7a */         \
    0xd981b540,                                      \
    0x6220,                                          \
    0x11d2,                                          \
    {0x81, 0x64, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}



#endif 

