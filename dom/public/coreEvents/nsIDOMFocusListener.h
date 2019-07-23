





































#ifndef nsIDOMFocusListener_h__
#define nsIDOMFocusListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMFOCUSLISTENER_IID \
{ /* 80974670-ded6-11d1-bd85-00805f8ae3f4 */ \
0x80974670, 0xded6, 0x11d1, \
{0xbd, 0x85, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

class nsIDOMFocusListener : public nsIDOMEventListener
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMFOCUSLISTENER_IID)

  




  NS_IMETHOD Focus(nsIDOMEvent* aEvent) = 0;

  




  NS_IMETHOD Blur(nsIDOMEvent* aEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMFocusListener, NS_IDOMFOCUSLISTENER_IID)

#endif 
