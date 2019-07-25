






































#ifndef mozilla_dom_telephony_telephony_h__
#define mozilla_dom_telephony_telephony_h__

#include "TelephonyCommon.h"

#include "nsIDOMTelephony.h"
#include "nsIDOMTelephonyCall.h"
#include "nsIRadioInterfaceLayer.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_TELEPHONY_NAMESPACE

class Telephony : public nsDOMEventTargetWrapperCache,
                  public nsIDOMTelephony
{
  nsCOMPtr<nsIRadioInterfaceLayer> mRIL;
  nsCOMPtr<nsIRILTelephonyCallback> mRILTelephonyCallback;

  NS_DECL_EVENT_HANDLER(incoming)

  TelephonyCall* mActiveCall;
  nsTArray<nsRefPtr<TelephonyCall> > mCalls;

  
  
  JSObject* mCallsArray;

  bool mRooted;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMTELEPHONY
  NS_DECL_NSIRILTELEPHONYCALLBACK
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetWrapperCache::)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(
                                                   Telephony,
                                                   nsDOMEventTargetWrapperCache)

  static already_AddRefed<Telephony>
  Create(nsPIDOMWindow* aOwner, nsIRadioInterfaceLayer* aRIL);

  nsIDOMEventTarget*
  ToIDOMEventTarget() const
  {
    return static_cast<nsDOMEventTargetWrapperCache*>(
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
    mCallsArray = nsnull;
  }

  void
  RemoveCall(TelephonyCall* aCall)
  {
    NS_ASSERTION(mCalls.Contains(aCall), "Didn't know about this one!");
    mCalls.RemoveElement(aCall);
    mCallsArray = nsnull;
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

private:
  Telephony()
  : mActiveCall(nsnull), mCallsArray(nsnull), mRooted(false)
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
