





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

  virtual JSObject* WrapObjectInternal(JSContext* aCx) MOZ_OVERRIDE
  {
    return PointerEventBinding::Wrap(aCx, this);
  }

  static already_AddRefed<PointerEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const PointerEventInit& aParam,
              ErrorResult& aRv);

  static already_AddRefed<PointerEvent>
  Constructor(EventTarget* aOwner,
              const nsAString& aType,
              const PointerEventInit& aParam);

  int32_t PointerId();
  int32_t Width();
  int32_t Height();
  float Pressure();
  int32_t TiltX();
  int32_t TiltY();
  bool IsPrimary();
  void GetPointerType(nsAString& aPointerType);
};

void ConvertPointerTypeToString(uint16_t aPointerTypeSrc, nsAString& aPointerTypeDest);

} 
} 

#endif 
