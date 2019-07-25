




































#ifndef __NS_SVGZOOMEVENT_H__
#define __NS_SVGZOOMEVENT_H__

#include "nsIDOMSVGZoomEvent.h"
#include "nsDOMUIEvent.h"
#include "nsIDOMSVGSVGElement.h"

namespace mozilla {
class DOMSVGPoint;
}

class nsDOMSVGZoomEvent : public nsDOMUIEvent,
                          public nsIDOMSVGZoomEvent
{
public:
  typedef mozilla::DOMSVGPoint DOMSVGPoint;

  nsDOMSVGZoomEvent(nsPresContext* aPresContext, nsGUIEvent* aEvent);
                     
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGZOOMEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

private:
  float mPreviousScale;
  float mNewScale;
  nsRefPtr<DOMSVGPoint> mPreviousTranslate;
  nsRefPtr<DOMSVGPoint> mNewTranslate;
};

#endif 
