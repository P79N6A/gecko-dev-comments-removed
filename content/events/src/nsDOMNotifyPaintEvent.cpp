





































#include "nsDOMNotifyPaintEvent.h"
#include "nsContentUtils.h"
#include "nsClientRect.h"

nsDOMNotifyPaintEvent::nsDOMNotifyPaintEvent(nsPresContext* aPresContext,
                                             nsNotifyPaintEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent ? aEvent :
               new nsNotifyPaintEvent(PR_FALSE, 0, nsRegion(), nsRegion()))
{  
  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else
  {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
  }
}

nsDOMNotifyPaintEvent::~nsDOMNotifyPaintEvent()
{
  if (mEventIsInternal) {
    if (mEvent->eventStructType == NS_NOTIFYPAINT_EVENT) {
      delete static_cast<nsNotifyPaintEvent*>(mEvent);
      mEvent = nsnull;
    }
  }
}

NS_INTERFACE_MAP_BEGIN(nsDOMNotifyPaintEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNotifyPaintEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(NotifyPaintEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMNotifyPaintEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMNotifyPaintEvent, nsDOMEvent)

nsRegion
nsDOMNotifyPaintEvent::GetRegion()
{
  nsNotifyPaintEvent* event = static_cast<nsNotifyPaintEvent*>(mEvent);

  nsRegion r;
  if (nsContentUtils::IsCallerTrustedForRead()) {
    r.Or(event->sameDocRegion, event->crossDocRegion);
  } else {
    r = event->sameDocRegion;
  }
  return r;
}

NS_IMETHODIMP
nsDOMNotifyPaintEvent::GetBoundingClientRect(nsIDOMClientRect** aResult)
{
  
  nsClientRect* rect = new nsClientRect();
  if (!rect)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult = rect);
  if (!mPresContext)
    return NS_OK;

  rect->SetLayoutRect(GetRegion().GetBounds(), mPresContext);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMNotifyPaintEvent::GetClientRects(nsIDOMClientRectList** aResult)
{
  nsRefPtr<nsClientRectList> rectList = new nsClientRectList();
  if (!rectList)
    return NS_ERROR_OUT_OF_MEMORY;

  nsRegion r = GetRegion();
  nsRegionRectIterator iter(r);
  for (const nsRect* rgnRect = iter.Next(); rgnRect; rgnRect = iter.Next()) {
    nsRefPtr<nsClientRect> rect = new nsClientRect();
    if (!rect)
      return NS_ERROR_OUT_OF_MEMORY;
    
    rect->SetLayoutRect(*rgnRect, mPresContext);
    rectList->Append(rect);
  }

  *aResult = rectList.forget().get();
  return NS_OK;
}

nsresult NS_NewDOMNotifyPaintEvent(nsIDOMEvent** aInstancePtrResult,
                                   nsPresContext* aPresContext,
                                   nsNotifyPaintEvent *aEvent) 
{
  nsDOMNotifyPaintEvent* it =
    new nsDOMNotifyPaintEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
