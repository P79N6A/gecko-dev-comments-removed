




#ifndef nsDOMScrollAreaEvent_h__
#define nsDOMScrollAreaEvent_h__

#include "nsIDOMScrollAreaEvent.h"
#include "nsDOMUIEvent.h"

#include "nsGUIEvent.h"
#include "nsClientRect.h"
#include "mozilla/dom/ScrollAreaEventBinding.h"

class nsDOMScrollAreaEvent : public nsDOMUIEvent,
                             public nsIDOMScrollAreaEvent
{
public:
  nsDOMScrollAreaEvent(mozilla::dom::EventTarget* aOwner,
                       nsPresContext *aPresContext,
                       nsScrollAreaEvent *aEvent);
  virtual ~nsDOMScrollAreaEvent();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMSCROLLAREAEVENT

  NS_FORWARD_NSIDOMUIEVENT(nsDOMUIEvent::)

  NS_FORWARD_TO_NSDOMEVENT_NO_SERIALIZATION_NO_DUPLICATION
  NS_IMETHOD DuplicatePrivateData()
  {
    return nsDOMEvent::DuplicatePrivateData();
  }
  NS_IMETHOD_(void) Serialize(IPC::Message* aMsg, bool aSerializeInterfaceType);
  NS_IMETHOD_(bool) Deserialize(const IPC::Message* aMsg, void** aIter);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::ScrollAreaEventBinding::Wrap(aCx, aScope, this);
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
                           mozilla::ErrorResult& aRv)
  {
    aRv = InitScrollAreaEvent(aType, aCanBubble, aCancelable, aView,
                              aDetail, aX, aY, aWidth, aHeight);
  }

protected:
  nsClientRect mClientArea;
};

#endif 
