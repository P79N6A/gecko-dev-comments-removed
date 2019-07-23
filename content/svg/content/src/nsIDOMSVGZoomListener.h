




































#ifndef __NS_SVGZOOMEVENT_H__
#define __NS_SVGZOOMEVENT_H__

#include "nsIDOMEventListener.h"

class nsIDOMEvent;





#define NS_IDOMSVGZOOMLISTENER_IID \
{ 0xccbeadab, 0xb3fe, 0x42f7, { 0x90, 0xed, 0xd6, 0xe4, 0x0f, 0x71, 0x2c, 0x29 } }

class nsIDOMSVGZoomListener : public nsIDOMEventListener {
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMSVGZOOMLISTENER_IID)
  NS_IMETHOD Zoom (nsIDOMEvent* aEvent) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMSVGZoomListener,
                              NS_IDOMSVGZOOMLISTENER_IID)

#endif 
