




































#ifndef __NS_SVGZOOMEVENT_H__
#define __NS_SVGZOOMEVENT_H__

#include "nsIDOMSVGZoomEvent.h"
#include "nsDOMUIEvent.h"
#include "nsIDOMSVGSVGElement.h"

class nsDOMSVGZoomEvent : public nsDOMUIEvent,
                          public nsIDOMSVGZoomEvent
{
public:
  nsDOMSVGZoomEvent(nsPresContext* aPresContext, nsGUIEvent* aEvent);
                     
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGZOOMEVENT

  
  NS_FORWARD_TO_NSDOMUIEVENT

private:
  float                    mPreviousScale;
  nsCOMPtr<nsIDOMSVGPoint> mPreviousTranslate;
  float                    mNewScale;
  nsCOMPtr<nsIDOMSVGPoint> mNewTranslate;
};

#endif 
