





#ifndef mozilla_dom_quota_acquirelistener_h__
#define mozilla_dom_quota_acquirelistener_h__

#include "mozilla/dom/quota/QuotaCommon.h"

BEGIN_QUOTA_NAMESPACE

class AcquireListener
{
public:
  NS_IMETHOD_(nsrefcnt)
  AddRef() = 0;

  NS_IMETHOD_(nsrefcnt)
  Release() = 0;

  virtual nsresult
  OnExclusiveAccessAcquired() = 0;
};

END_QUOTA_NAMESPACE

#endif 
