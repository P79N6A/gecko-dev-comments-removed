






































#ifndef mozilla_dom_telephony_telephonycallarray_h__
#define mozilla_dom_telephony_telephonycallarray_h__

#include "TelephonyCommon.h"

#include "nsIDOMTelephonyCallArray.h"

BEGIN_TELEPHONY_NAMESPACE

class TelephonyCallArray : public nsIDOMTelephonyCallArray,
                           public nsWrapperCache
{
  nsRefPtr<Telephony> mTelephony;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMTELEPHONYCALLARRAY
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TelephonyCallArray)

  static already_AddRefed<TelephonyCallArray>
  Create(Telephony* aTelephony);

  nsISupports*
  GetParentObject() const;

  virtual JSObject*
  WrapObject(JSContext* aCx, XPCWrappedNativeScope* aScope, bool* aTriedToWrap);

private:
  TelephonyCallArray(Telephony* aTelephony);
  ~TelephonyCallArray();
};

END_TELEPHONY_NAMESPACE

#endif 
