





#ifndef mozilla_dom_telephony_telephony_h__
#define mozilla_dom_telephony_telephony_h__

#include "mozilla/dom/telephony/TelephonyCommon.h"

#include "nsITelephonyProvider.h"



#include "TelephonyCall.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class OwningTelephonyCallOrTelephonyCallGroup;

class Telephony MOZ_FINAL : public nsDOMEventTargetHelper
{
  






  class Listener;

  class EnumerationAck;
  friend class EnumerationAck;

  nsCOMPtr<nsITelephonyProvider> mProvider;
  nsRefPtr<Listener> mListener;

  TelephonyCall* mActiveCall;
  nsTArray<nsRefPtr<TelephonyCall> > mCalls;
  nsRefPtr<CallsList> mCallsList;

  nsRefPtr<TelephonyCallGroup> mGroup;

  bool mEnumerated;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSITELEPHONYLISTENER
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(Telephony,
                                           nsDOMEventTargetHelper)

  nsPIDOMWindow*
  GetParentObject() const
  {
    return GetOwner();
  }

  
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  already_AddRefed<TelephonyCall>
  Dial(const nsAString& aNumber, ErrorResult& aRv);

  already_AddRefed<TelephonyCall>
  DialEmergency(const nsAString& aNumber, ErrorResult& aRv);

  bool
  GetMuted(ErrorResult& aRv) const;

  void
  SetMuted(bool aMuted, ErrorResult& aRv);

  bool
  GetSpeakerEnabled(ErrorResult& aRv) const;

  void
  SetSpeakerEnabled(bool aEnabled, ErrorResult& aRv);

  void
  GetActive(Nullable<OwningTelephonyCallOrTelephonyCallGroup>& aValue);

  already_AddRefed<CallsList>
  Calls() const;

  already_AddRefed<TelephonyCallGroup>
  ConferenceGroup() const;

  void
  StartTone(const nsAString& aDTMF, ErrorResult& aRv);

  void
  StopTone(ErrorResult& aRv);

  IMPL_EVENT_HANDLER(incoming)
  IMPL_EVENT_HANDLER(callschanged)
  IMPL_EVENT_HANDLER(remoteheld)
  IMPL_EVENT_HANDLER(remoteresumed)

  static already_AddRefed<Telephony>
  Create(nsPIDOMWindow* aOwner, ErrorResult& aRv);

  void
  AddCall(TelephonyCall* aCall)
  {
    NS_ASSERTION(!mCalls.Contains(aCall), "Already know about this one!");
    mCalls.AppendElement(aCall);
    UpdateActiveCall(aCall, true);
    NotifyCallsChanged(aCall);
  }

  void
  RemoveCall(TelephonyCall* aCall)
  {
    NS_ASSERTION(mCalls.Contains(aCall), "Didn't know about this one!");
    mCalls.RemoveElement(aCall);
    UpdateActiveCall(aCall, false);
    NotifyCallsChanged(aCall);
  }

  nsITelephonyProvider*
  Provider() const
  {
    return mProvider;
  }

  const nsTArray<nsRefPtr<TelephonyCall> >&
  CallsArray() const
  {
    return mCalls;
  }

  virtual void EventListenerAdded(nsIAtom* aType) MOZ_OVERRIDE;

private:
  Telephony();
  ~Telephony();

  already_AddRefed<TelephonyCall>
  CreateNewDialingCall(const nsAString& aNumber);

  void
  NoteDialedCallFromOtherInstance(const nsAString& aNumber);

  nsresult
  NotifyCallsChanged(TelephonyCall* aCall);

  already_AddRefed<TelephonyCall>
  DialInternal(bool isEmergency,
               const nsAString& aNumber,
               ErrorResult& aRv);

  nsresult
  DispatchCallEvent(const nsAString& aType,
                    TelephonyCall* aCall);

  void
  EnqueueEnumerationAck();

  void
  UpdateActiveCall(TelephonyCall* aCall, bool aIsAdding);

  already_AddRefed<TelephonyCall>
  GetCall(uint32_t aCallIndex);

  bool
  MoveCall(uint32_t aCallIndex, bool aIsConference);

  void
  Shutdown();
};

} 
} 

#endif 
