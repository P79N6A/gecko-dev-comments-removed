





#ifndef mozilla_dom_telephony_callevent_h
#define mozilla_dom_telephony_callevent_h

#include "mozilla/dom/telephony/TelephonyCommon.h"

#include "nsDOMEvent.h"

namespace mozilla {
namespace dom {

struct CallEventInit;

class CallEvent MOZ_FINAL : public nsDOMEvent
{
  nsRefPtr<TelephonyCall> mCall;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(CallEvent, nsDOMEvent)
  NS_FORWARD_TO_NSDOMEVENT

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  static already_AddRefed<CallEvent>
  Constructor(const GlobalObject& aGlobal, const nsAString& aType,
              const CallEventInit& aOptions, ErrorResult& aRv);

  already_AddRefed<TelephonyCall>
  GetCall() const;

  static already_AddRefed<CallEvent>
  Create(EventTarget* aOwner, const nsAString& aType, TelephonyCall* aCall,
         bool aCanBubble, bool aCancelable);

private:
  CallEvent(EventTarget* aOwner,
            nsPresContext* aPresContext,
            WidgetEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext, aEvent)
  {
    SetIsDOMBinding();
  }

  virtual ~CallEvent()
  { }
};

} 
} 

#endif 
