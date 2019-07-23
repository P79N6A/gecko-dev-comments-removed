





































#ifndef nsIDOMFormListener_h__
#define nsIDOMFormListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMFORMLISTENER_IID \
{ /* 566c3f80-28ab-11d2-bd89-00805f8ae3f4 */ \
0x566c3f80, 0x28ab, 0x11d2, \
{0xbd, 0x89, 0x00, 0x80, 0x5f, 0x8a, 0xe3, 0xf4} }

class nsIDOMFormListener : public nsIDOMEventListener {

public:
   NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMFORMLISTENER_IID)
  




  NS_IMETHOD Submit(nsIDOMEvent* aEvent) = 0;

  




  NS_IMETHOD Reset(nsIDOMEvent* aEvent) = 0;

  




  NS_IMETHOD Change(nsIDOMEvent* aEvent) = 0;

  




  NS_IMETHOD Select(nsIDOMEvent* aEvent) = 0;
  
  




  NS_IMETHOD Input(nsIDOMEvent* aEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMFormListener, NS_IDOMFORMLISTENER_IID)

#endif 
