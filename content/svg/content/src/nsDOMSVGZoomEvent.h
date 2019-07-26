




#ifndef __NS_SVGZOOMEVENT_H__
#define __NS_SVGZOOMEVENT_H__

#include "nsAutoPtr.h"
#include "nsDOMUIEvent.h"
#include "nsIDOMSVGZoomEvent.h"

class nsGUIEvent;
class nsPresContext;

namespace mozilla {
class DOMSVGPoint;
}

class nsDOMSVGZoomEvent : public nsDOMUIEvent,
                          public nsIDOMSVGZoomEvent
{
public:
  typedef mozilla::DOMSVGPoint DOMSVGPoint;

  nsDOMSVGZoomEvent(mozilla::dom::EventTarget* aOwner,
                    nsPresContext* aPresContext, nsGUIEvent* aEvent);
                     
  
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
