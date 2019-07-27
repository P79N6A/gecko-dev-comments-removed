





#ifndef mozilla_dom_cache_Types_h
#define mozilla_dom_cache_Types_h

#include <stdint.h>
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsString.h"

namespace mozilla {
namespace dom {
namespace cache {

enum Namespace
{
  DEFAULT_NAMESPACE,
  CHROME_ONLY_NAMESPACE,
  NUMBER_OF_NAMESPACES
};

typedef uintptr_t RequestId;
static const RequestId INVALID_REQUEST_ID = 0;

typedef int32_t CacheId;

struct QuotaInfo
{
  QuotaInfo() : mIsApp(false) { }
  nsCOMPtr<nsIFile> mDir;
  nsCString mGroup;
  nsCString mOrigin;
  nsCString mStorageId;
  bool mIsApp;
};

} 
} 
} 

#endif 
