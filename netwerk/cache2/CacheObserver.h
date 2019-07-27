



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
  virtual ~CacheObserver() {}

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

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
  static void SetDiskCacheCapacity(uint32_t); 
  static uint32_t const DiskFreeSpaceSoftLimit() 
    { return sDiskFreeSpaceSoftLimit << 10; }
  static uint32_t const DiskFreeSpaceHardLimit() 
    { return sDiskFreeSpaceHardLimit << 10; }
  static bool const SmartCacheSizeEnabled()
    { return sSmartCacheSizeEnabled; }
  static uint32_t const PreloadChunkCount()
    { return sPreloadChunkCount; }
  static uint32_t const MaxMemoryEntrySize() 
    { return sMaxMemoryEntrySize << 10; }
  static uint32_t const MaxDiskEntrySize() 
    { return sMaxDiskEntrySize << 10; }
  static uint32_t const MaxDiskChunksMemoryUsage(bool aPriority) 
    { return aPriority ? sMaxDiskPriorityChunksMemoryUsage << 10
                       : sMaxDiskChunksMemoryUsage << 10; }
  static uint32_t const CompressionLevel()
    { return sCompressionLevel; }
  static uint32_t const HalfLifeSeconds()
    { return sHalfLifeHours * 60.0F * 60.0F; }
  static int32_t const HalfLifeExperiment()
    { return sHalfLifeExperiment; }
  static bool const ClearCacheOnShutdown()
    { return sSanitizeOnShutdown && sClearCacheOnShutdown; }
  static bool const CacheFSReported()
    { return sCacheFSReported; }
  static void SetCacheFSReported();
  static void ParentDirOverride(nsIFile ** aDir);

  static bool const EntryIsTooBig(int64_t aSize, bool aUsingDisk);

private:
  static CacheObserver* sSelf;

  void StoreDiskCacheCapacity();
  void StoreCacheFSReported();
  void AttachToPreferences();

  static uint32_t sUseNewCache;
  static bool sUseMemoryCache;
  static bool sUseDiskCache;
  static uint32_t sMetadataMemoryLimit;
  static int32_t sMemoryCacheCapacity;
  static int32_t sAutoMemoryCacheCapacity;
  static uint32_t sDiskCacheCapacity;
  static uint32_t sDiskFreeSpaceSoftLimit;
  static uint32_t sDiskFreeSpaceHardLimit;
  static bool sSmartCacheSizeEnabled;
  static uint32_t sPreloadChunkCount;
  static uint32_t sMaxMemoryEntrySize;
  static uint32_t sMaxDiskEntrySize;
  static uint32_t sMaxDiskChunksMemoryUsage;
  static uint32_t sMaxDiskPriorityChunksMemoryUsage;
  static uint32_t sCompressionLevel;
  static float sHalfLifeHours;
  static int32_t sHalfLifeExperiment;
  static bool sSanitizeOnShutdown;
  static bool sClearCacheOnShutdown;
  static bool sCacheFSReported;

  
  nsCOMPtr<nsIFile> mCacheParentDirectoryOverride;
};

} 
} 

#endif
