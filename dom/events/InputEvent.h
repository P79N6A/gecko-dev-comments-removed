




#ifndef mozilla_dom_InputEvent_h_
#define mozilla_dom_InputEvent_h_

#include "mozilla/dom/UIEvent.h"
#include "mozilla/dom/InputEventBinding.h"
#include "mozilla/EventForwards.h"

namespace mozilla {
namespace dom {

class InputEvent : public UIEvent
{
public:
  InputEvent(EventTarget* aOwner,
             nsPresContext* aPresContext,
             InternalEditorInputEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_TO_UIEVENT


  static already_AddRefed<InputEvent> Constructor(const GlobalObject& aGlobal,
                                                  const nsAString& aType,
                                                  const InputEventInit& aParam,
                                                  ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return InputEventBinding::Wrap(aCx, this);
  }

  bool IsComposing();

protected:
  ~InputEvent() {}
};

} 
} 

#endif 
