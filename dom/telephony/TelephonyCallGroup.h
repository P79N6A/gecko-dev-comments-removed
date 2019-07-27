





#ifndef mozilla_dom_telephony_telephonycallgroup_h__
#define mozilla_dom_telephony_telephonycallgroup_h__

#include "mozilla/dom/Promise.h"
#include "mozilla/dom/telephony/TelephonyCommon.h"

namespace mozilla {
namespace dom {

class TelephonyCallGroup final : public DOMEventTargetHelper
{
  nsRefPtr<Telephony> mTelephony;

  nsTArray<nsRefPtr<TelephonyCall> > mCalls;

  nsRefPtr<CallsList> mCallsList;

  nsString mState;

  uint16_t mCallState;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TelephonyCallGroup,
                                           DOMEventTargetHelper)

  nsPIDOMWindow*
  GetParentObject() const
  {
    return GetOwner();
  }

  
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  already_AddRefed<CallsList>
  Calls() const;

  already_AddRefed<Promise>
  Add(TelephonyCall& aCall, ErrorResult& aRv);

  already_AddRefed<Promise>
  Add(TelephonyCall& aCall, TelephonyCall& aSecondCall, ErrorResult& aRv);

  already_AddRefed<Promise>
  Remove(TelephonyCall& aCall, ErrorResult& aRv);

  already_AddRefed<Promise>
  HangUp(ErrorResult& aRv);

  already_AddRefed<Promise>
  Hold(ErrorResult& aRv);

  already_AddRefed<Promise>
  Resume(ErrorResult& aRv);

  void
  GetState(nsString& aState) const
  {
    aState = mState;
  }

  IMPL_EVENT_HANDLER(statechange)
  IMPL_EVENT_HANDLER(connected)
  IMPL_EVENT_HANDLER(held)
  IMPL_EVENT_HANDLER(callschanged)
  IMPL_EVENT_HANDLER(error)

  static already_AddRefed<TelephonyCallGroup>
  Create(Telephony* aTelephony);

  void
  AddCall(TelephonyCall* aCall);

  void
  RemoveCall(TelephonyCall* aCall);

  already_AddRefed<TelephonyCall>
  GetCall(uint32_t aServiceId, uint32_t aCallIndex);

  const nsTArray<nsRefPtr<TelephonyCall> >&
  CallsArray() const
  {
    return mCalls;
  }

  void
  ChangeState(uint16_t aCallState);

  uint16_t
  CallState() const
  {
    return mCallState;
  }

  nsresult
  NotifyError(const nsAString& aName, const nsAString& aMessage);

private:
  explicit TelephonyCallGroup(nsPIDOMWindow* aOwner);
  ~TelephonyCallGroup();

  nsresult
  NotifyCallsChanged(TelephonyCall* aCall);

  nsresult
  DispatchCallEvent(const nsAString& aType,
                    TelephonyCall* aCall);

  already_AddRefed<Promise>
  CreatePromise(ErrorResult& aRv);

  bool CanConference(const TelephonyCall& aCall, const TelephonyCall* aSecondCall);
};

} 
} 

#endif 
