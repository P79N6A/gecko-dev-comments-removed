





#ifndef mozilla_dom_quota_quotacommon_h__
#define mozilla_dom_quota_quotacommon_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsPrintfCString.h"
#include "nsString.h"
#include "nsTArray.h"

#define BEGIN_QUOTA_NAMESPACE \
  namespace mozilla { namespace dom { namespace quota {
#define END_QUOTA_NAMESPACE \
  } /* namespace quota */ } /* namespace dom */ } /* namespace mozilla */
#define USING_QUOTA_NAMESPACE \
  using namespace mozilla::dom::quota;

#define DSSTORE_FILE_NAME ".DS_Store"

#define QM_WARNING(...)                                                        \
  do {                                                                         \
    nsPrintfCString str(__VA_ARGS__);                                          \
    mozilla::dom::quota::ReportInternalError(__FILE__, __LINE__, str.get());   \
    NS_WARNING(str.get());                                                     \
  } while (0)

BEGIN_QUOTA_NAMESPACE

void
AssertIsOnIOThread();

void
AssertCurrentThreadOwnsQuotaMutex();

bool
IsOnIOThread();

void
ReportInternalError(const char* aFile, uint32_t aLine, const char* aStr);

END_QUOTA_NAMESPACE

#endif 
