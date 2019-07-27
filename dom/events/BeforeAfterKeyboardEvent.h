




#ifndef mozilla_dom_BeforeAfterKeyboardEvent_h_
#define mozilla_dom_BeforeAfterKeyboardEvent_h_

#include "mozilla/dom/KeyboardEvent.h"
#include "mozilla/dom/BeforeAfterKeyboardEventBinding.h"

namespace mozilla {
namespace dom {

class BeforeAfterKeyboardEvent : public KeyboardEvent
{
public:
  BeforeAfterKeyboardEvent(EventTarget* aOwner,
                           nsPresContext* aPresContext,
                           InternalBeforeAfterKeyboardEvent* aEvent);

  virtual JSObject* WrapObjectInternal(JSContext* aCx) MOZ_OVERRIDE
  {
    return BeforeAfterKeyboardEventBinding::Wrap(aCx, this);
  }

  static already_AddRefed<BeforeAfterKeyboardEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const BeforeAfterKeyboardEventInit& aParam,
              ErrorResult& aRv);

  static already_AddRefed<BeforeAfterKeyboardEvent>
  Constructor(EventTarget* aOwner, const nsAString& aType,
              const BeforeAfterKeyboardEventInit& aEventInitDict);

  
  
  Nullable<bool> GetEmbeddedCancelled();
};

} 
} 

#endif 
