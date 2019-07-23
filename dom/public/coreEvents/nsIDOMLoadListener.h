





































#ifndef nsIDOMLoadListener_h__
#define nsIDOMLoadListener_h__

#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"





#define NS_IDOMLOADLISTENER_IID \
{ /* d1810238-14f8-4cab-9b96-96bedb9de7be */ \
0xd1810238, 0x14f8, 0x4cab, \
{0x9b, 0x96, 0x96, 0xbe, 0xdb, 0x9d, 0xe7, 0xbe} }

class nsIDOMLoadListener : public nsIDOMEventListener {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMLOADLISTENER_IID)
  




  NS_IMETHOD Load(nsIDOMEvent* aEvent) = 0;

  




  NS_IMETHOD BeforeUnload(nsIDOMEvent* aEvent) = 0;

  




  NS_IMETHOD Unload(nsIDOMEvent* aEvent) = 0;

  





  NS_IMETHOD Abort(nsIDOMEvent* aEvent) = 0;

  




  NS_IMETHOD Error(nsIDOMEvent* aEvent) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMLoadListener, NS_IDOMLOADLISTENER_IID)

#endif 
