




#ifndef nsDOMScrollAreaEvent_h__
#define nsDOMScrollAreaEvent_h__

#include "nsIDOMScrollAreaEvent.h"
#include "nsDOMUIEvent.h"

#include "nsGUIEvent.h"
#include "nsClientRect.h"

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

protected:
  nsClientRect mClientArea;
};

#endif 
