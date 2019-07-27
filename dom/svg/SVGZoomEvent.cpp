




#include "DOMSVGPoint.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "mozilla/dom/SVGZoomEvent.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "prtime.h"

namespace mozilla {
namespace dom {




NS_IMPL_CYCLE_COLLECTION_INHERITED(SVGZoomEvent, UIEvent, mPreviousTranslate, mNewTranslate)

NS_IMPL_ADDREF_INHERITED(SVGZoomEvent, UIEvent)
NS_IMPL_RELEASE_INHERITED(SVGZoomEvent, UIEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGZoomEvent)
NS_INTERFACE_MAP_END_INHERITING(UIEvent)

SVGZoomEvent::SVGZoomEvent(EventTarget* aOwner,
                           nsPresContext* aPresContext,
                           InternalSVGZoomEvent* aEvent)
  : UIEvent(aOwner, aPresContext,
            aEvent ? aEvent : new InternalSVGZoomEvent(false, NS_SVG_ZOOM))
  , mPreviousScale(0)
  , mNewScale(0)
{
  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }

  
  
  
  
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

SVGZoomEvent::~SVGZoomEvent()
{
}

} 
} 





nsresult
NS_NewDOMSVGZoomEvent(nsIDOMEvent** aInstancePtrResult,
                      mozilla::dom::EventTarget* aOwner,
                      nsPresContext* aPresContext,
                      mozilla::InternalSVGZoomEvent* aEvent)
{
  mozilla::dom::SVGZoomEvent* it =
    new mozilla::dom::SVGZoomEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
