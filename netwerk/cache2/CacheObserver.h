



#ifndef CacheObserver__h__
#define CacheObserver__h__

#include "nsIObserver.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"
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

  
  static bool const UseNewCache();
  static bool const UseDiskCache()
    { return sUseDiskCache; }
  static bool const UseMemoryCache()
    { return sUseMemoryCache; }
  static uint32_t const MetadataMemoryLimit() 
    { return sMetadataMemoryLimit << 10; }
  static uint32_t const MemoryCacheCapacity(); 
  static uint32_t const DiskCacheCapacity() 
    { return sDiskCacheCapacity << 10; }
  static uint32_t const MaxMemoryEntrySize() 
    { return sMaxMemoryEntrySize << 10; }
  static uint32_t const MaxDiskEntrySize() 
    { return sMaxDiskEntrySize << 10; }
  static uint32_t const CompressionLevel()
    { return sCompressionLevel; }
  static uint32_t const HalfLifeSeconds()
    { return sHalfLifeHours * 60 * 60; }
  static int32_t const HalfLifeExperiment()
    { return sHalfLifeExperiment; }
  static void ParentDirOverride(nsIFile ** aDir);

  static bool const EntryIsTooBig(int64_t aSize, bool aUsingDisk);

private:
  static CacheObserver* sSelf;

  void AttachToPreferences();
  void SchduleAutoDelete();

  static uint32_t sUseNewCache;
  static bool sUseMemoryCache;
  static bool sUseDiskCache;
  static uint32_t sMetadataMemoryLimit;
  static int32_t sMemoryCacheCapacity;
  static int32_t sAutoMemoryCacheCapacity;
  static uint32_t sDiskCacheCapacity;
  static uint32_t sMaxMemoryEntrySize;
  static uint32_t sMaxDiskEntrySize;
  static uint32_t sCompressionLevel;
  static uint32_t sHalfLifeHours;
  static int32_t sHalfLifeExperiment;

  
  nsCOMPtr<nsIFile> mCacheParentDirectoryOverride;
};

} 
} 

#endif
