














































#ifndef nsIJVMPrefsWindow_h___
#define nsIJVMPrefsWindow_h___

#include "nsIJVMWindow.h"







class nsIJVMPrefsWindow : public nsIJVMWindow {
public:
    
    NS_IMETHOD
    Show(void) = 0;

    NS_IMETHOD
    Hide(void) = 0;

    NS_IMETHOD
    IsVisible(PRBool *result) = 0;

};

#define NS_IJVMPREFSWINDOW_IID                       \
{ /* 20330d70-4ec9-11d2-8164-006008119d7a */         \
    0x20330d70,                                      \
    0x4ec9,                                          \
    0x11d2,                                          \
    {0x81, 0x64, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

#define NS_JVMPREFSWINDOW_CID                        \
{ /* e9c1ef10-6304-11d2-8164-006008119d7a */         \
    0xe9c1ef10,                                      \
    0x6304,                                          \
    0x11d2,                                          \
    {0x81, 0x64, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}



#endif 
