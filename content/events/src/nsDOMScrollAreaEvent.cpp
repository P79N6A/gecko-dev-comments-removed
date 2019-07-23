




































#include "nsDOMScrollAreaEvent.h"
#include "nsGUIEvent.h"
#include "nsClientRect.h"

nsDOMScrollAreaEvent::nsDOMScrollAreaEvent(nsPresContext *aPresContext,
                                           nsScrollAreaEvent *aEvent)
  : nsDOMUIEvent(aPresContext, aEvent)
{
  mClientArea.SetLayoutRect(aEvent ? aEvent->mArea : nsRect());
}

nsDOMScrollAreaEvent::~nsDOMScrollAreaEvent()
{
  if (mEventIsInternal && mEvent) {
    if (mEvent->eventStructType == NS_SCROLLAREA_EVENT) {
      delete static_cast<nsScrollAreaEvent *>(mEvent);
      mEvent = nsnull;
    }
  }
}

NS_IMPL_ADDREF_INHERITED(nsDOMScrollAreaEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMScrollAreaEvent, nsDOMUIEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMScrollAreaEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMScrollAreaEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(ScrollAreaEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)


NS_IMETHODIMP
nsDOMScrollAreaEvent::GetX(float *aX)
{
  return mClientArea.GetLeft(aX);
}

NS_IMETHODIMP
nsDOMScrollAreaEvent::GetY(float *aY)
{
  return mClientArea.GetTop(aY);
}

NS_IMETHODIMP
nsDOMScrollAreaEvent::GetWidth(float *aWidth)
{
  return mClientArea.GetWidth(aWidth);
}

NS_IMETHODIMP
nsDOMScrollAreaEvent::GetHeight(float *aHeight)
{
  return mClientArea.GetHeight(aHeight);
}

NS_IMETHODIMP
nsDOMScrollAreaEvent::InitScrollAreaEvent(const nsAString &aEventType,
                                          PRBool aCanBubble,
                                          PRBool aCancelable,
                                          nsIDOMAbstractView *aView,
                                          PRInt32 aDetail,
                                          float aX, float aY,
                                          float aWidth, float aHeight)
{
  nsresult rv = nsDOMUIEvent::InitUIEvent(aEventType, aCanBubble, aCancelable, aView, aDetail);
  NS_ENSURE_SUCCESS(rv, rv);

  mClientArea.SetRect(aX, aY, aWidth, aHeight);

  return NS_OK;
}

nsresult
NS_NewDOMScrollAreaEvent(nsIDOMEvent **aInstancePtrResult,
                         nsPresContext *aPresContext,
                         nsScrollAreaEvent *aEvent)
{
  nsDOMScrollAreaEvent *ev = new nsDOMScrollAreaEvent(aPresContext, aEvent);

  if (!ev) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(ev, aInstancePtrResult);
}
