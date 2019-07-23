




































#ifndef nsIDOMUIListener_h__
#define nsIDOMUIListener_h__

#include "nsIDOMEventListener.h"

class nsIDOMEvent;





#define NS_IDOMUILISTENER_IID \
{ 0x5cb5527a, 0x512f, 0x4163, { 0x93, 0x93, 0xca, 0x95, 0xce, 0xdd, 0xbc, 0x13 } }

class nsIDOMUIListener : public nsIDOMEventListener {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMUILISTENER_IID)

  NS_IMETHOD Activate(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD FocusIn(nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD FocusOut(nsIDOMEvent* aEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMUIListener, NS_IDOMUILISTENER_IID)

#endif 
