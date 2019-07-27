




#ifndef mozilla_dom_NotifyPaintEvent_h_
#define mozilla_dom_NotifyPaintEvent_h_

#include "mozilla/Attributes.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/NotifyPaintEventBinding.h"
#include "nsIDOMNotifyPaintEvent.h"
#include "nsPresContext.h"

namespace mozilla {
namespace dom {

class DOMRect;
class DOMRectList;
class PaintRequestList;

class NotifyPaintEvent : public Event,
                         public nsIDOMNotifyPaintEvent
{

public:
  NotifyPaintEvent(EventTarget* aOwner,
                   nsPresContext* aPresContext,
                   WidgetEvent* aEvent,
                   uint32_t aEventType,
                   nsInvalidateRequestList* aInvalidateRequests);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMNOTIFYPAINTEVENT

  
  NS_FORWARD_TO_EVENT_NO_SERIALIZATION_NO_DUPLICATION
  NS_IMETHOD DuplicatePrivateData() MOZ_OVERRIDE
  {
    return Event::DuplicatePrivateData();
  }
  NS_IMETHOD_(void) Serialize(IPC::Message* aMsg, bool aSerializeInterfaceType) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) Deserialize(const IPC::Message* aMsg, void** aIter) MOZ_OVERRIDE;

  virtual JSObject* WrapObjectInternal(JSContext* aCx) MOZ_OVERRIDE
  {
    return NotifyPaintEventBinding::Wrap(aCx, this);
  }

  already_AddRefed<DOMRectList> ClientRects();

  already_AddRefed<DOMRect> BoundingClientRect();

  already_AddRefed<PaintRequestList> PaintRequests();

protected:
  ~NotifyPaintEvent() {}

private:
  nsRegion GetRegion();

  nsTArray<nsInvalidateRequestList::Request> mInvalidateRequests;
};

} 
} 

#endif 
