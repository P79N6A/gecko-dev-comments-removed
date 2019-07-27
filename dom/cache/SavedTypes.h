





#ifndef mozilla_dom_cache_SavedTypes_h
#define mozilla_dom_cache_SavedTypes_h




#include "mozilla/dom/cache/PCacheTypes.h"
#include "mozilla/dom/cache/Types.h"
#include "nsCOMPtr.h"
#include "nsID.h"
#include "nsIOutputStream.h"

namespace mozilla {
namespace dom {
namespace cache {

struct SavedRequest
{
  SavedRequest() : mHasBodyId(false) { mValue.body() = void_t(); }
  PCacheRequest mValue;
  bool mHasBodyId;
  nsID mBodyId;
  CacheId mCacheId;
};

struct SavedResponse
{
  SavedResponse() : mHasBodyId(false) { mValue.body() = void_t(); }
  PCacheResponse mValue;
  bool mHasBodyId;
  nsID mBodyId;
  CacheId mCacheId;
};

} 
} 
} 

#endif 
