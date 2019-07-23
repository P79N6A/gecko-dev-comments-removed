




































#ifndef nsIDOMKeyListener_h__
#define nsIDOMKeyListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"


#ifdef KeyPress
#undef KeyPress
#endif




#define NS_IDOMKEYLISTENER_IID \
{ /* 35f0d080-da38-11d1-bd85-00805f8ae3f4 */ \
0x35f0d080, 0xda38, 0x11d1, \
{0xbd, 0x85, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

class nsIDOMKeyListener : public nsIDOMEventListener {

public:

     NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMKEYLISTENER_IID)
    
    




    NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent) = 0;

    




    NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent) = 0;

    





    NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMKeyListener, NS_IDOMKEYLISTENER_IID)

#endif 
