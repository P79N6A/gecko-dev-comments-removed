





































#ifdef MOZ_IPC
#include "base/basictypes.h"
#include "IPC/IPCMessageUtils.h"
#endif
#include "nsDOMNotifyPaintEvent.h"
#include "nsContentUtils.h"
#include "nsClientRect.h"
#include "nsPaintRequest.h"
#include "nsIFrame.h"

nsDOMNotifyPaintEvent::nsDOMNotifyPaintEvent(nsPresContext* aPresContext,
                                             nsEvent* aEvent,
                                             PRUint32 aEventType,
                                             nsInvalidateRequestList* aInvalidateRequests)
: nsDOMEvent(aPresContext, aEvent)
{
  if (mEvent) {
    mEvent->message = aEventType;
  }
  if (aInvalidateRequests) {
    mInvalidateRequests.SwapElements(aInvalidateRequests->mRequests);
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
  nsRegion r;
  PRBool isTrusted = nsContentUtils::IsCallerTrustedForRead();
  for (PRUint32 i = 0; i < mInvalidateRequests.Length(); ++i) {
    if (!isTrusted &&
        (mInvalidateRequests[i].mFlags & nsIFrame::INVALIDATE_CROSS_DOC))
      continue;

    r.Or(r, mInvalidateRequests[i].mRect);
    r.SimplifyOutward(10);
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

  rect->SetLayoutRect(GetRegion().GetBounds());
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
    
    rect->SetLayoutRect(*rgnRect);
    rectList->Append(rect);
  }

  rectList.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMNotifyPaintEvent::GetPaintRequests(nsIDOMPaintRequestList** aResult)
{
  nsRefPtr<nsPaintRequestList> requests = new nsPaintRequestList();
  if (!requests)
    return NS_ERROR_OUT_OF_MEMORY;

  PRBool isTrusted = nsContentUtils::IsCallerTrustedForRead();
  for (PRUint32 i = 0; i < mInvalidateRequests.Length(); ++i) {
    if (!isTrusted &&
        (mInvalidateRequests[i].mFlags & nsIFrame::INVALIDATE_CROSS_DOC))
      continue;

    nsRefPtr<nsPaintRequest> r = new nsPaintRequest();
    if (!r)
      return NS_ERROR_OUT_OF_MEMORY;
    r->SetRequest(mInvalidateRequests[i]);
    requests->Append(r);
  }

  requests.forget(aResult);
  return NS_OK;
}

#ifdef MOZ_IPC
void
nsDOMNotifyPaintEvent::Serialize(IPC::Message* aMsg,
                                 PRBool aSerializeInterfaceType)
{
  if (aSerializeInterfaceType) {
    IPC::WriteParam(aMsg, NS_LITERAL_STRING("notifypaintevent"));
  }

  nsDOMEvent::Serialize(aMsg, PR_FALSE);

  PRUint32 length = mInvalidateRequests.Length();
  IPC::WriteParam(aMsg, length);
  for (PRUint32 i = 0; i < length; ++i) {
    IPC::WriteParam(aMsg, mInvalidateRequests[i].mRect.x);
    IPC::WriteParam(aMsg, mInvalidateRequests[i].mRect.y);
    IPC::WriteParam(aMsg, mInvalidateRequests[i].mRect.width);
    IPC::WriteParam(aMsg, mInvalidateRequests[i].mRect.height);
    IPC::WriteParam(aMsg, mInvalidateRequests[i].mFlags);
  }
}

PRBool
nsDOMNotifyPaintEvent::Deserialize(const IPC::Message* aMsg, void** aIter)
{
  NS_ENSURE_TRUE(nsDOMEvent::Deserialize(aMsg, aIter), PR_FALSE);

  PRUint32 length = 0;
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &length), PR_FALSE);
  mInvalidateRequests.SetCapacity(length);
  for (PRUint32 i = 0; i < length; ++i) {
    nsInvalidateRequestList::Request req;
    NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &req.mRect.x), PR_FALSE);
    NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &req.mRect.y), PR_FALSE);
    NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &req.mRect.width), PR_FALSE);
    NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &req.mRect.height), PR_FALSE);
    NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &req.mFlags), PR_FALSE);
    mInvalidateRequests.AppendElement(req);
  }

  return PR_TRUE;
}
#endif

nsresult NS_NewDOMNotifyPaintEvent(nsIDOMEvent** aInstancePtrResult,
                                   nsPresContext* aPresContext,
                                   nsEvent *aEvent,
                                   PRUint32 aEventType,
                                   nsInvalidateRequestList* aInvalidateRequests) 
{
  nsDOMNotifyPaintEvent* it =
    new nsDOMNotifyPaintEvent(aPresContext, aEvent, aEventType,
                              aInvalidateRequests);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
