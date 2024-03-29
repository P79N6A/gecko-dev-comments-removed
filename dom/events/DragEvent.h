





#ifndef mozilla_dom_DragEvent_h_
#define mozilla_dom_DragEvent_h_

#include "nsIDOMDragEvent.h"
#include "mozilla/dom/MouseEvent.h"
#include "mozilla/dom/DragEventBinding.h"
#include "mozilla/EventForwards.h"

namespace mozilla {
namespace dom {

class DataTransfer;

class DragEvent : public MouseEvent,
                  public nsIDOMDragEvent
{
public:
  DragEvent(EventTarget* aOwner,
            nsPresContext* aPresContext,
            WidgetDragEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOMDRAGEVENT

  NS_FORWARD_TO_MOUSEEVENT

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override
  {
    return DragEventBinding::Wrap(aCx, this, aGivenProto);
  }

  DataTransfer* GetDataTransfer();

  void InitDragEvent(const nsAString& aType,
                     bool aCanBubble, bool aCancelable,
                     nsIDOMWindow* aView, int32_t aDetail,
                     int32_t aScreenX, int32_t aScreenY,
                     int32_t aClientX, int32_t aClientY,
                     bool aCtrlKey, bool aAltKey, bool aShiftKey,
                     bool aMetaKey, uint16_t aButton,
                     EventTarget* aRelatedTarget,
                     DataTransfer* aDataTransfer,
                     ErrorResult& aError);

  static already_AddRefed<DragEvent> Constructor(const GlobalObject& aGlobal,
                                                 const nsAString& aType,
                                                 const DragEventInit& aParam,
                                                 ErrorResult& aRv);

protected:
  ~DragEvent() {}
};

} 
} 

#endif 
