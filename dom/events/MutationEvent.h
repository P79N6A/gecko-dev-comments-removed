




#ifndef mozilla_dom_MutationEvent_h_
#define mozilla_dom_MutationEvent_h_

#include "mozilla/EventForwards.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/MutationEventBinding.h"
#include "nsIDOMMutationEvent.h"
#include "nsINode.h"

namespace mozilla {
namespace dom {

class MutationEvent : public Event,
                      public nsIDOMMutationEvent
{
public:
  MutationEvent(EventTarget* aOwner,
                nsPresContext* aPresContext,
                InternalMutationEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMMUTATIONEVENT

  
  NS_FORWARD_TO_EVENT

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return MutationEventBinding::Wrap(aCx, this);
  }

  
  
  
  

  already_AddRefed<nsINode> GetRelatedNode();

  uint16_t AttrChange();

  void InitMutationEvent(const nsAString& aType,
                         bool& aCanBubble, bool& aCancelable,
                         nsINode* aRelatedNode,
                         const nsAString& aPrevValue,
                         const nsAString& aNewValue,
                         const nsAString& aAttrName,
                         uint16_t& aAttrChange, ErrorResult& aRv)
  {
    aRv = InitMutationEvent(aType, aCanBubble, aCancelable,
                            aRelatedNode ? aRelatedNode->AsDOMNode() : nullptr,
                            aPrevValue, aNewValue, aAttrName, aAttrChange);
  }

protected:
  ~MutationEvent() {}
};

} 
} 

#endif 
