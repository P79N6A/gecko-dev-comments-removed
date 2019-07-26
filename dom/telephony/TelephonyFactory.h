




#ifndef mozilla_dom_telephony_TelephonyFactory_h
#define mozilla_dom_telephony_TelephonyFactory_h

#include "nsCOMPtr.h"
#include "mozilla/dom/telephony/TelephonyCommon.h"

class nsITelephonyService;

BEGIN_TELEPHONY_NAMESPACE

class TelephonyFactory
{
public:
  static already_AddRefed<nsITelephonyService> CreateTelephonyService();
};

END_TELEPHONY_NAMESPACE

#endif 
