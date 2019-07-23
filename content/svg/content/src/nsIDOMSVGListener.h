




































#ifndef __NS_SVGEVENT_H__
#define __NS_SVGEVENT_H__

#include "nsIDOMEventListener.h"

class nsIDOMEvent;





#define NS_IDOMSVGLISTENER_IID \
{ 0xcb6f30f1, 0x5754, 0x49a3, { 0xa6, 0x6f, 0x2d, 0x6b, 0xa1, 0xb6, 0x3a, 0x58 } }

class nsIDOMSVGListener : public nsIDOMEventListener {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMSVGLISTENER_IID)
  NS_IMETHOD Load   (nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Unload (nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Abort  (nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Error  (nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Resize (nsIDOMEvent* aEvent) = 0;
  NS_IMETHOD Scroll (nsIDOMEvent* aEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMSVGListener, NS_IDOMSVGLISTENER_IID)

#endif 
