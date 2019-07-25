






































#ifndef mozilla_dom_telephony_telephonycall_h__
#define mozilla_dom_telephony_telephonycall_h__

#include "TelephonyCommon.h"

#include "nsIDOMTelephonyCall.h"
#include "nsIRadioInterfaceLayer.h"

class nsPIDOMWindow;

BEGIN_TELEPHONY_NAMESPACE

class TelephonyCall : public nsDOMEventTargetWrapperCache,
                      public nsIDOMTelephonyCall
{
  NS_DECL_EVENT_HANDLER(statechange)
  NS_DECL_EVENT_HANDLER(dialing)
  NS_DECL_EVENT_HANDLER(ringing)
  NS_DECL_EVENT_HANDLER(busy)
  NS_DECL_EVENT_HANDLER(connecting)
  NS_DECL_EVENT_HANDLER(connected)
  NS_DECL_EVENT_HANDLER(disconnecting)
  NS_DECL_EVENT_HANDLER(disconnected)
  NS_DECL_EVENT_HANDLER(incoming)

  nsRefPtr<Telephony> mTelephony;

  nsString mNumber;
  nsString mState;

  PRUint32 mCallIndex;
  PRUint16 mCallState;
  bool mLive;
  bool mOutgoing;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMTELEPHONYCALL
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetWrapperCache::)
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TelephonyCall,
                                           nsDOMEventTargetWrapperCache)

  static already_AddRefed<TelephonyCall>
  Create(Telephony* aTelephony, const nsAString& aNumber, PRUint16 aCallState,
         PRUint32 aCallIndex = kOutgoingPlaceholderCallIndex);

  nsIDOMEventTarget*
  ToIDOMEventTarget() const
  {
    return static_cast<nsDOMEventTargetWrapperCache*>(
             const_cast<TelephonyCall*>(this));
  }

  nsISupports*
  ToISupports() const
  {
    return ToIDOMEventTarget();
  }

  void
  ChangeState(PRUint16 aCallState)
  {
    ChangeStateInternal(aCallState, true);
  }

  PRUint32
  CallIndex() const
  {
    return mCallIndex;
  }

  void
  UpdateCallIndex(PRUint32 aCallIndex)
  {
    NS_ASSERTION(mCallIndex == kOutgoingPlaceholderCallIndex,
                 "Call index should not be set!");
    mCallIndex = aCallIndex;
  }

  PRUint16
  CallState() const
  {
    return mCallState;
  }

  bool
  IsOutgoing() const
  {
    return mOutgoing;
  }

private:
  TelephonyCall()
  : mCallIndex(kOutgoingPlaceholderCallIndex),
    mCallState(nsIRadioInterfaceLayer::CALL_STATE_UNKNOWN), mLive(false), mOutgoing(false)
  { }

  ~TelephonyCall()
  { }

  void
  ChangeStateInternal(PRUint16 aCallState, bool aFireEvents);
};

END_TELEPHONY_NAMESPACE

#endif 
