




































#ifndef nsIDOMTextListener_h__
#define nsIDOMTextListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMTEXTLISTENER_IID \
{ 0xc6296e81, 0xd823, 0x11d2, { 0x9e, 0x7f, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b } }


class nsIDOMTextListener : public nsIDOMEventListener
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMTEXTLISTENER_IID)

  NS_IMETHOD HandleText(nsIDOMEvent* aTextEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMTextListener, NS_IDOMTEXTLISTENER_IID)

#endif 
