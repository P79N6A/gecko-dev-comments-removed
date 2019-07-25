






































#ifndef mozilla_dom_telephony_telephony_h__
#define mozilla_dom_telephony_telephony_h__

#include "TelephonyCommon.h"

#include "nsIDOMTelephony.h"
#include "nsIDOMTelephonyCall.h"
#include "nsIRadioInterfaceLayer.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_TELEPHONY_NAMESPACE

class Telephony : public nsDOMEventTargetHelper,
                  public nsIDOMTelephony
{
  nsCOMPtr<nsIRadioInterfaceLayer> mRIL;
  nsCOMPtr<nsIRILTelephonyCallback> mRILTelephonyCallback;

  NS_DECL_EVENT_HANDLER(incoming)

  TelephonyCall* mActiveCall;
  nsTArray<nsRefPtr<TelephonyCall> > mCalls;

  nsRefPtr<TelephonyCallArray> mCallsArray;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMTELEPHONY
  NS_DECL_NSIRILTELEPHONYCALLBACK
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(Telephony, nsDOMEventTargetHelper)

  static already_AddRefed<Telephony>
  Create(nsPIDOMWindow* aOwner, nsIRadioInterfaceLayer* aRIL);

  nsIDOMEventTarget*
  ToIDOMEventTarget() const
  {
    return static_cast<nsDOMEventTargetHelper*>(
             const_cast<Telephony*>(this));
  }

  nsISupports*
  ToISupports() const
  {
    return ToIDOMEventTarget();
  }

  void
  AddCall(TelephonyCall* aCall)
  {
    NS_ASSERTION(!mCalls.Contains(aCall), "Already know about this one!");
    mCalls.AppendElement(aCall);
  }

  void
  RemoveCall(TelephonyCall* aCall)
  {
    NS_ASSERTION(mCalls.Contains(aCall), "Didn't know about this one!");
    mCalls.RemoveElement(aCall);
  }

  nsIRadioInterfaceLayer*
  RIL() const
  {
    return mRIL;
  }

  nsPIDOMWindow*
  Owner() const
  {
    return mOwner;
  }

  nsIScriptContext*
  ScriptContext() const
  {
    return mScriptContext;
  }

  const nsTArray<nsRefPtr<TelephonyCall> >&
  Calls() const
  {
    return mCalls;
  }

private:
  Telephony()
  : mActiveCall(nsnull)
  { }

  ~Telephony();

  void
  SwitchActiveCall(TelephonyCall* aCall);

  class RILTelephonyCallback : public nsIRILTelephonyCallback
  {
    Telephony* mTelephony;

  public:
    NS_DECL_ISUPPORTS
    NS_FORWARD_NSIRILTELEPHONYCALLBACK(mTelephony->)

    RILTelephonyCallback(Telephony* aTelephony)
    : mTelephony(aTelephony)
    {
      NS_ASSERTION(mTelephony, "Null pointer!");
    }
  };
};

END_TELEPHONY_NAMESPACE

#endif 
