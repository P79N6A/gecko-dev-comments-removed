





#ifndef mozilla_dom_PointerEvent_h_
#define mozilla_dom_PointerEvent_h_

#include "mozilla/dom/MouseEvent.h"
#include "mozilla/dom/PointerEventBinding.h"

class nsPresContext;

namespace mozilla {
namespace dom {

class PointerEvent : public MouseEvent
{
public:
  PointerEvent(EventTarget* aOwner,
               nsPresContext* aPresContext,
               WidgetPointerEvent* aEvent);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return PointerEventBinding::Wrap(aCx, aScope, this);
  }

  static already_AddRefed<PointerEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const PointerEventInit& aParam,
              ErrorResult& aRv);

  int32_t PointerId();
  int32_t Width();
  int32_t Height();
  float Pressure();
  int32_t TiltX();
  int32_t TiltY();
  bool IsPrimary();
  void GetPointerType(nsAString& aPointerType);
};

} 
} 

#endif 
