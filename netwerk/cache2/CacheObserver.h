



#ifndef CacheObserver__h__
#define CacheObserver__h__

#include "nsIObserver.h"
#include "nsWeakReference.h"
#include <algorithm>

namespace mozilla {
namespace net {

class CacheObserver : public nsIObserver
                    , public nsSupportsWeakReference
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  virtual ~CacheObserver() {}

  static nsresult Init();
  static nsresult Shutdown();
  static CacheObserver* Self() { return sSelf; }

  
  static uint32_t const MemoryLimit() 
    { return std::max(512U, std::min(1048576U, sMemoryLimit)) << 10; }
  static bool const UseNewCache();

private:
  static CacheObserver* sSelf;

  void AttachToPreferences();

  static uint32_t sMemoryLimit;
  static uint32_t sUseNewCache;
};

} 
} 

#endif
