




#ifndef mozilla_dom_SVGZoomEvent_h
#define mozilla_dom_SVGZoomEvent_h

#include "nsAutoPtr.h"
#include "nsDOMUIEvent.h"
#include "nsIDOMSVGZoomEvent.h"

class nsGUIEvent;
class nsPresContext;

namespace mozilla {
class DOMSVGPoint;

namespace dom {

class SVGZoomEvent : public nsDOMUIEvent,
                     public nsIDOMSVGZoomEvent
{
public:
  SVGZoomEvent(EventTarget* aOwner, nsPresContext* aPresContext,
               nsGUIEvent* aEvent);
                     
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGZOOMEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

private:
  float mPreviousScale;
  float mNewScale;
  nsRefPtr<DOMSVGPoint> mPreviousTranslate;
  nsRefPtr<DOMSVGPoint> mNewTranslate;
};

} 
} 

#endif 
