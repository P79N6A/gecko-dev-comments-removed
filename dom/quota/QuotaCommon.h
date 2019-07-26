





#ifndef mozilla_dom_quota_quotacommon_h__
#define mozilla_dom_quota_quotacommon_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsString.h"
#include "nsTArray.h"

#define BEGIN_QUOTA_NAMESPACE \
  namespace mozilla { namespace dom { namespace quota {
#define END_QUOTA_NAMESPACE \
  } /* namespace quota */ } /* namespace dom */ } /* namespace mozilla */
#define USING_QUOTA_NAMESPACE \
  using namespace mozilla::dom::quota;

#define DSSTORE_FILE_NAME ".DS_Store"

BEGIN_QUOTA_NAMESPACE

void
AssertIsOnIOThread();

void
AssertCurrentThreadOwnsQuotaMutex();

bool
IsOnIOThread();

END_QUOTA_NAMESPACE

#endif 
