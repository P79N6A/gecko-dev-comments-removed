




#include "base/basictypes.h"
#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/DOMRect.h"
#include "mozilla/dom/NotifyPaintEvent.h"
#include "mozilla/GfxMessageUtils.h"
#include "nsContentUtils.h"
#include "nsPaintRequest.h"

namespace mozilla {
namespace dom {

NotifyPaintEvent::NotifyPaintEvent(EventTarget* aOwner,
                                   nsPresContext* aPresContext,
                                   WidgetEvent* aEvent,
                                   uint32_t aEventType,
                                   nsInvalidateRequestList* aInvalidateRequests)
: nsDOMEvent(aOwner, aPresContext, aEvent)
{
  if (mEvent) {
    mEvent->message = aEventType;
  }
  if (aInvalidateRequests) {
    mInvalidateRequests.MoveElementsFrom(aInvalidateRequests->mRequests);
  }
}

NS_INTERFACE_MAP_BEGIN(NotifyPaintEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNotifyPaintEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(NotifyPaintEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(NotifyPaintEvent, nsDOMEvent)

nsRegion
NotifyPaintEvent::GetRegion()
{
  nsRegion r;
  if (!nsContentUtils::IsCallerChrome()) {
    return r;
  }
  for (uint32_t i = 0; i < mInvalidateRequests.Length(); ++i) {
    r.Or(r, mInvalidateRequests[i].mRect);
    r.SimplifyOutward(10);
  }
  return r;
}

NS_IMETHODIMP
NotifyPaintEvent::GetBoundingClientRect(nsIDOMClientRect** aResult)
{
  *aResult = BoundingClientRect().get();
  return NS_OK;
}

already_AddRefed<DOMRect>
NotifyPaintEvent::BoundingClientRect()
{
  nsRefPtr<DOMRect> rect = new DOMRect(ToSupports(this));

  if (mPresContext) {
    rect->SetLayoutRect(GetRegion().GetBounds());
  }

  return rect.forget();
}

NS_IMETHODIMP
NotifyPaintEvent::GetClientRects(nsIDOMClientRectList** aResult)
{
  *aResult = ClientRects().get();
  return NS_OK;
}

already_AddRefed<DOMRectList>
NotifyPaintEvent::ClientRects()
{
  nsISupports* parent = ToSupports(this);
  nsRefPtr<DOMRectList> rectList = new DOMRectList(parent);

  nsRegion r = GetRegion();
  nsRegionRectIterator iter(r);
  for (const nsRect* rgnRect = iter.Next(); rgnRect; rgnRect = iter.Next()) {
    nsRefPtr<DOMRect> rect = new DOMRect(parent);
    
    rect->SetLayoutRect(*rgnRect);
    rectList->Append(rect);
  }

  return rectList.forget();
}

NS_IMETHODIMP
NotifyPaintEvent::GetPaintRequests(nsISupports** aResult)
{
  nsRefPtr<nsPaintRequestList> requests = PaintRequests();
  requests.forget(aResult);
  return NS_OK;
}

already_AddRefed<nsPaintRequestList>
NotifyPaintEvent::PaintRequests()
{
  nsDOMEvent* parent = this;
  nsRefPtr<nsPaintRequestList> requests = new nsPaintRequestList(parent);

  if (nsContentUtils::IsCallerChrome()) {
    for (uint32_t i = 0; i < mInvalidateRequests.Length(); ++i) {
      nsRefPtr<nsPaintRequest> r = new nsPaintRequest(parent);
      r->SetRequest(mInvalidateRequests[i]);
      requests->Append(r);
    }
  }

  return requests.forget();
}

NS_IMETHODIMP_(void)
NotifyPaintEvent::Serialize(IPC::Message* aMsg,
                            bool aSerializeInterfaceType)
{
  if (aSerializeInterfaceType) {
    IPC::WriteParam(aMsg, NS_LITERAL_STRING("notifypaintevent"));
  }

  nsDOMEvent::Serialize(aMsg, false);

  uint32_t length = mInvalidateRequests.Length();
  IPC::WriteParam(aMsg, length);
  for (uint32_t i = 0; i < length; ++i) {
    IPC::WriteParam(aMsg, mInvalidateRequests[i].mRect);
    IPC::WriteParam(aMsg, mInvalidateRequests[i].mFlags);
  }
}

NS_IMETHODIMP_(bool)
NotifyPaintEvent::Deserialize(const IPC::Message* aMsg, void** aIter)
{
  NS_ENSURE_TRUE(nsDOMEvent::Deserialize(aMsg, aIter), false);

  uint32_t length = 0;
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &length), false);
  mInvalidateRequests.SetCapacity(length);
  for (uint32_t i = 0; i < length; ++i) {
    nsInvalidateRequestList::Request req;
    NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &req.mRect), false);
    NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &req.mFlags), false);
    mInvalidateRequests.AppendElement(req);
  }

  return true;
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMNotifyPaintEvent(nsIDOMEvent** aInstancePtrResult,
                          EventTarget* aOwner,
                          nsPresContext* aPresContext,
                          WidgetEvent* aEvent,
                          uint32_t aEventType,
                          nsInvalidateRequestList* aInvalidateRequests) 
{
  NotifyPaintEvent* it = new NotifyPaintEvent(aOwner, aPresContext, aEvent,
                                              aEventType, aInvalidateRequests);
  return CallQueryInterface(it, aInstancePtrResult);
}
