




#include "mozilla/dom/SVGZoomEvent.h"
#include "DOMSVGPoint.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "mozilla/dom/Element.h"
#include "prtime.h"

namespace mozilla {
namespace dom {




SVGZoomEvent::SVGZoomEvent(EventTarget* aOwner,
                           nsPresContext* aPresContext,
                           WidgetGUIEvent* aEvent)
  : nsDOMUIEvent(aOwner, aPresContext,
                 aEvent ? aEvent : new WidgetGUIEvent(false, NS_SVG_ZOOM, 0))
  , mPreviousScale(0)
  , mNewScale(0)
{
  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->eventStructType = NS_SVGZOOM_EVENT;
    mEvent->time = PR_Now();
  }

  mEvent->mFlags.mCancelable = false;

  
  
  
  
  nsIPresShell *presShell;
  if (mPresContext && (presShell = mPresContext->GetPresShell())) {
    nsIDocument *doc = presShell->GetDocument();
    if (doc) {
      Element *rootElement = doc->GetRootElement();
      if (rootElement) {
        
        
        
        
        if (rootElement->IsSVG(nsGkAtoms::svg)) {
          SVGSVGElement *SVGSVGElem =
            static_cast<SVGSVGElement*>(rootElement);

          mNewScale = SVGSVGElem->GetCurrentScale();
          mPreviousScale = SVGSVGElem->GetPreviousScale();

          const SVGPoint& translate = SVGSVGElem->GetCurrentTranslate();
          mNewTranslate =
            new DOMSVGPoint(translate.GetX(), translate.GetY());
          mNewTranslate->SetReadonly(true);

          const SVGPoint& prevTranslate = SVGSVGElem->GetPreviousTranslate();
          mPreviousTranslate =
            new DOMSVGPoint(prevTranslate.GetX(), prevTranslate.GetY());
          mPreviousTranslate->SetReadonly(true);
        }
      }
    }
  }
}


} 
} 





nsresult
NS_NewDOMSVGZoomEvent(nsIDOMEvent** aInstancePtrResult,
                      mozilla::dom::EventTarget* aOwner,
                      nsPresContext* aPresContext,
                      mozilla::WidgetGUIEvent* aEvent)
{
  mozilla::dom::SVGZoomEvent* it =
    new mozilla::dom::SVGZoomEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
