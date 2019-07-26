




#ifndef nsDOMMutationEvent_h__
#define nsDOMMutationEvent_h__

#include "nsIDOMMutationEvent.h"
#include "nsINode.h"
#include "nsDOMEvent.h"
#include "mozilla/dom/MutationEventBinding.h"
#include "mozilla/EventForwards.h"

class nsDOMMutationEvent : public nsDOMEvent,
                           public nsIDOMMutationEvent
{
public:
  nsDOMMutationEvent(mozilla::dom::EventTarget* aOwner,
                     nsPresContext* aPresContext,
                     mozilla::InternalMutationEvent* aEvent);

  virtual ~nsDOMMutationEvent();

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMMUTATIONEVENT

  
  NS_FORWARD_TO_NSDOMEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::MutationEventBinding::Wrap(aCx, aScope, this);
  }

  
  
  
  

  already_AddRefed<nsINode> GetRelatedNode();

  uint16_t AttrChange();

  void InitMutationEvent(const nsAString& aType,
                         bool& aCanBubble, bool& aCancelable,
                         nsINode* aRelatedNode,
                         const nsAString& aPrevValue,
                         const nsAString& aNewValue,
                         const nsAString& aAttrName,
                         uint16_t& aAttrChange, mozilla::ErrorResult& aRv)
  {
    aRv = InitMutationEvent(aType, aCanBubble, aCancelable,
                            aRelatedNode ? aRelatedNode->AsDOMNode() : nullptr,
                            aPrevValue, aNewValue, aAttrName, aAttrChange);
  }
};

#endif 
