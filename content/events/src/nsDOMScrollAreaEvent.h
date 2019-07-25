




































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
  nsDOMScrollAreaEvent(nsPresContext *aPresContext,
                       nsScrollAreaEvent *aEvent);
  virtual ~nsDOMScrollAreaEvent();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMSCROLLAREAEVENT

  NS_FORWARD_TO_NSDOMUIEVENT

    virtual void Serialize(IPC::Message* aMsg, bool aSerializeInterfaceType);
    virtual bool Deserialize(const IPC::Message* aMsg, void** aIter);

protected:
  nsClientRect mClientArea;
};

#endif 
