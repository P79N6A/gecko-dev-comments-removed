




#ifndef mozilla_dom_ScrollAreaEvent_h_
#define mozilla_dom_ScrollAreaEvent_h_

#include "mozilla/dom/DOMRect.h"
#include "mozilla/dom/ScrollAreaEventBinding.h"
#include "mozilla/dom/UIEvent.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsIDOMScrollAreaEvent.h"

namespace mozilla {
namespace dom {

class ScrollAreaEvent : public UIEvent,
                        public nsIDOMScrollAreaEvent
{
public:
  ScrollAreaEvent(EventTarget* aOwner,
                  nsPresContext* aPresContext,
                  InternalScrollAreaEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMSCROLLAREAEVENT

  NS_FORWARD_NSIDOMUIEVENT(UIEvent::)

  NS_FORWARD_TO_EVENT_NO_SERIALIZATION_NO_DUPLICATION
  NS_IMETHOD DuplicatePrivateData()
  {
    return Event::DuplicatePrivateData();
  }
  NS_IMETHOD_(void) Serialize(IPC::Message* aMsg, bool aSerializeInterfaceType) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) Deserialize(const IPC::Message* aMsg, void** aIter) MOZ_OVERRIDE;

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return ScrollAreaEventBinding::Wrap(aCx, this);
  }

  float X() const
  {
    return mClientArea.Left();
  }

  float Y() const
  {
    return mClientArea.Top();
  }

  float Width() const
  {
    return mClientArea.Width();
  }

  float Height() const
  {
    return mClientArea.Height();
  }

  void InitScrollAreaEvent(const nsAString& aType,
                           bool aCanBubble,
                           bool aCancelable,
                           nsIDOMWindow* aView,
                           int32_t aDetail,
                           float aX, float aY,
                           float aWidth, float aHeight,
                           ErrorResult& aRv)
  {
    aRv = InitScrollAreaEvent(aType, aCanBubble, aCancelable, aView,
                              aDetail, aX, aY, aWidth, aHeight);
  }

protected:
  DOMRect mClientArea;
};

} 
} 

#endif 
