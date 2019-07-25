






































#ifndef mozilla_dom_telephony_telephony_h__
#define mozilla_dom_telephony_telephony_h__

#include "TelephonyCommon.h"

#include "nsIDOMTelephony.h"
#include "nsIDOMTelephonyCall.h"
#include "nsITelephone.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_TELEPHONY_NAMESPACE

class Telephony : public nsDOMEventTargetWrapperCache,
                  public nsIDOMTelephony
{
  nsCOMPtr<nsITelephone> mTelephone;
  nsCOMPtr<nsITelephoneCallback> mTelephoneCallback;

  NS_DECL_EVENT_HANDLER(incoming);

  TelephonyCall* mActiveCall;
  nsTArray<nsRefPtr<TelephonyCall> > mCalls;

  
  
  JSObject* mCallsArray;

  bool mRooted;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMTELEPHONY
  NS_DECL_NSITELEPHONECALLBACK
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetWrapperCache::)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(
                                                   Telephony,
                                                   nsDOMEventTargetWrapperCache)

  static already_AddRefed<Telephony>
  Create(nsPIDOMWindow* aOwner, nsITelephone* aTelephone);

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

  nsITelephone*
  Telephone() const
  {
    return mTelephone;
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

  class TelephoneCallback : public nsITelephoneCallback
  {
    Telephony* mTelephony;

  public:
    NS_DECL_ISUPPORTS
    NS_FORWARD_NSITELEPHONECALLBACK(mTelephony->)

    TelephoneCallback(Telephony* aTelephony)
    : mTelephony(aTelephony)
    {
      NS_ASSERTION(mTelephony, "Null pointer!");
    }
  };
};

END_TELEPHONY_NAMESPACE

#endif 
