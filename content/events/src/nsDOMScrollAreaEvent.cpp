




































#include "base/basictypes.h"
#include "IPC/IPCMessageUtils.h"

#include "nsDOMScrollAreaEvent.h"
#include "nsGUIEvent.h"
#include "nsClientRect.h"
#include "nsDOMClassInfoID.h"
#include "nsIClassInfo.h"
#include "nsIXPCScriptable.h"

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

DOMCI_DATA(ScrollAreaEvent, nsDOMScrollAreaEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMScrollAreaEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMScrollAreaEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ScrollAreaEvent)
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
                                          nsIDOMWindow *aView,
                                          PRInt32 aDetail,
                                          float aX, float aY,
                                          float aWidth, float aHeight)
{
  nsresult rv = nsDOMUIEvent::InitUIEvent(aEventType, aCanBubble, aCancelable, aView, aDetail);
  NS_ENSURE_SUCCESS(rv, rv);

  mClientArea.SetRect(aX, aY, aWidth, aHeight);

  return NS_OK;
}

void
nsDOMScrollAreaEvent::Serialize(IPC::Message* aMsg,
                                PRBool aSerializeInterfaceType)
{
  if (aSerializeInterfaceType) {
    IPC::WriteParam(aMsg, NS_LITERAL_STRING("scrollareaevent"));
  }

  nsDOMEvent::Serialize(aMsg, PR_FALSE);

  float val;
  mClientArea.GetLeft(&val);
  IPC::WriteParam(aMsg, val);
  mClientArea.GetTop(&val);
  IPC::WriteParam(aMsg, val);
  mClientArea.GetWidth(&val);
  IPC::WriteParam(aMsg, val);
  mClientArea.GetHeight(&val);
  IPC::WriteParam(aMsg, val);
}

PRBool
nsDOMScrollAreaEvent::Deserialize(const IPC::Message* aMsg, void** aIter)
{
  NS_ENSURE_TRUE(nsDOMEvent::Deserialize(aMsg, aIter), PR_FALSE);

  float x, y, width, height;
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &x), PR_FALSE);
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &y), PR_FALSE);
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &width), PR_FALSE);
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &height), PR_FALSE);
  mClientArea.SetRect(x, y, width, height);

  return PR_TRUE;
}

nsresult
NS_NewDOMScrollAreaEvent(nsIDOMEvent **aInstancePtrResult,
                         nsPresContext *aPresContext,
                         nsScrollAreaEvent *aEvent)
{
  nsDOMScrollAreaEvent *ev = new nsDOMScrollAreaEvent(aPresContext, aEvent);
  return CallQueryInterface(ev, aInstancePtrResult);
}
