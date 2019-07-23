




































#include "nsDOMSVGZoomEvent.h"
#include "nsContentUtils.h"
#include "nsSVGRect.h"
#include "nsSVGPoint.h"
#include "nsSVGSVGElement.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"




nsDOMSVGZoomEvent::nsDOMSVGZoomEvent(nsPresContext* aPresContext,
                                     nsGUIEvent* aEvent)
  : nsDOMUIEvent(aPresContext,
                 aEvent ? aEvent : new nsGUIEvent(PR_FALSE, NS_SVG_ZOOM, 0))
{
  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->eventStructType = NS_SVGZOOM_EVENT;
    mEvent->time = PR_Now();
  }

  mEvent->flags |= NS_EVENT_FLAG_CANT_CANCEL;

  
  
  
  
  nsIPresShell *presShell;
  if (mPresContext && (presShell = mPresContext->GetPresShell())) {
    nsIDocument *doc = presShell->GetDocument();
    if (doc) {
      nsIContent *rootContent = doc->GetRootContent();
      if (rootContent) {
        
        
        
        
        nsCOMPtr<nsIDOMSVGSVGElement> svgElement = do_QueryInterface(rootContent);
        if (svgElement) {
          nsSVGSVGElement *SVGSVGElement =
            static_cast<nsSVGSVGElement*>(rootContent);
  
          mNewScale = SVGSVGElement->GetCurrentScale();
          mPreviousScale = SVGSVGElement->GetPreviousScale();

          const nsSVGTranslatePoint& translate =
            SVGSVGElement->GetCurrentTranslate();
          NS_NewSVGReadonlyPoint(getter_AddRefs(mNewTranslate),
                                 translate.GetX(), translate.GetY());

          const nsSVGTranslatePoint& prevTranslate =
            SVGSVGElement->GetPreviousTranslate();
          NS_NewSVGReadonlyPoint(getter_AddRefs(mPreviousTranslate),
                                 prevTranslate.GetX(), prevTranslate.GetY());
        }
      }
    }
  }
}





NS_IMPL_ADDREF_INHERITED(nsDOMSVGZoomEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMSVGZoomEvent, nsDOMUIEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMSVGZoomEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGZoomEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGZoomEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)






NS_IMETHODIMP nsDOMSVGZoomEvent::GetZoomRectScreen(nsIDOMSVGRect **aZoomRectScreen)
{
  
  
  
  
  
  
  
  
  
  
  
  

  *aZoomRectScreen = nsnull;
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
nsDOMSVGZoomEvent::GetPreviousTranslate(nsIDOMSVGPoint **aPreviousTranslate)
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
nsDOMSVGZoomEvent::GetNewTranslate(nsIDOMSVGPoint **aNewTranslate)
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
