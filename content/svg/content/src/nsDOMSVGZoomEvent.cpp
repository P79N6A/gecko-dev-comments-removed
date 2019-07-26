




#include "nsDOMSVGZoomEvent.h"
#include "nsSVGRect.h"
#include "DOMSVGPoint.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::dom;




nsDOMSVGZoomEvent::nsDOMSVGZoomEvent(nsPresContext* aPresContext,
                                     nsGUIEvent* aEvent)
  : nsDOMUIEvent(aPresContext,
                 aEvent ? aEvent : new nsGUIEvent(false, NS_SVG_ZOOM, 0))
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

          const nsSVGTranslatePoint& translate =
            SVGSVGElem->GetCurrentTranslate();
          mNewTranslate =
            new DOMSVGPoint(translate.GetX(), translate.GetY());
          mNewTranslate->SetReadonly(true);

          const nsSVGTranslatePoint& prevTranslate =
            SVGSVGElem->GetPreviousTranslate();
          mPreviousTranslate =
            new DOMSVGPoint(prevTranslate.GetX(), prevTranslate.GetY());
          mPreviousTranslate->SetReadonly(true);
        }
      }
    }
  }
}





NS_IMPL_ADDREF_INHERITED(nsDOMSVGZoomEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMSVGZoomEvent, nsDOMUIEvent)

DOMCI_DATA(SVGZoomEvent, nsDOMSVGZoomEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMSVGZoomEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGZoomEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGZoomEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)






NS_IMETHODIMP nsDOMSVGZoomEvent::GetZoomRectScreen(nsIDOMSVGRect **aZoomRectScreen)
{
  
  
  
  
  
  
  
  
  
  
  
  

  *aZoomRectScreen = nullptr;
  NS_NOTYETIMPLEMENTED("nsDOMSVGZoomEvent::GetZoomRectScreen");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDOMSVGZoomEvent::GetPreviousScale(float *aPreviousScale)
{
  *aPreviousScale = mPreviousScale;
  return NS_OK;
}


NS_IMETHODIMP
nsDOMSVGZoomEvent::GetPreviousTranslate(nsISupports **aPreviousTranslate)
{
  *aPreviousTranslate = mPreviousTranslate;
  NS_IF_ADDREF(*aPreviousTranslate);
  return NS_OK;
}


NS_IMETHODIMP nsDOMSVGZoomEvent::GetNewScale(float *aNewScale)
{
  *aNewScale = mNewScale;
  return NS_OK;
}


NS_IMETHODIMP
nsDOMSVGZoomEvent::GetNewTranslate(nsISupports **aNewTranslate)
{
  *aNewTranslate = mNewTranslate;
  NS_IF_ADDREF(*aNewTranslate);
  return NS_OK;
}





nsresult
NS_NewDOMSVGZoomEvent(nsIDOMEvent** aInstancePtrResult,
                      nsPresContext* aPresContext,
                      nsGUIEvent *aEvent)
{
  nsDOMSVGZoomEvent* it = new nsDOMSVGZoomEvent(aPresContext, aEvent);
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;

  return CallQueryInterface(it, aInstancePtrResult);
}
