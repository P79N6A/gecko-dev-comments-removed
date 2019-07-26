




#ifndef StartupCache_h_
#define StartupCache_h_

#include "prio.h"
#include "prtypes.h"

#include "nsClassHashtable.h"
#include "nsIZipWriter.h"
#include "nsIZipReader.h"
#include "nsComponentManagerUtils.h"
#include "nsZipArchive.h"
#include "nsIStartupCache.h"
#include "nsIStorageStream.h"
#include "nsITimer.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsIOutputStream.h"
#include "nsIFile.h"
#include "mozilla/Attributes.h"















































class nsIMemoryReporter;

namespace mozilla {
namespace scache {

struct CacheEntry 
{
  nsAutoArrayPtr<char> data;
  uint32_t size;

  CacheEntry() : data(nullptr), size(0) { }

  
  CacheEntry(char* buf, uint32_t len) : data(buf), size(len) { }

  ~CacheEntry()
  {
  }

  size_t SizeOfExcludingThis(nsMallocSizeOfFun mallocSizeOf) {
    return mallocSizeOf(data);
  }
};



class StartupCacheListener MOZ_FINAL : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

class StartupCache
{

friend class StartupCacheListener;
friend class StartupCacheWrapper;
                                
public:

  

  
  nsresult GetBuffer(const char* id, char** outbuf, uint32_t* length);

  
  nsresult PutBuffer(const char* id, const char* inbuf, uint32_t length);

  
  void InvalidateCache();

  
  static void IgnoreDiskCache();

  
  
  nsresult GetDebugObjectOutputStream(nsIObjectOutputStream* aStream,
                                      nsIObjectOutputStream** outStream);

  nsresult RecordAgesAlways();

  static StartupCache* GetSingleton();
  static void DeleteSingleton();

  
  
  size_t HeapSizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf);

  size_t SizeOfMapping();

private:
  StartupCache();
  ~StartupCache();

  enum TelemetrifyAge {
    IGNORE_AGE = 0,
    RECORD_AGE = 1
  };
  static enum TelemetrifyAge gPostFlushAgeAction;

  nsresult LoadArchive(enum TelemetrifyAge flag);
  nsresult Init();
  void WriteToDisk();
  nsresult ResetStartupWriteTimer();
  void WaitOnWriteThread();

  static nsresult InitSingleton();
  static void WriteTimeout(nsITimer *aTimer, void *aClosure);
  static void ThreadedWrite(void *aClosure);

  static size_t SizeOfEntryExcludingThis(const nsACString& key,
                                         const nsAutoPtr<CacheEntry>& data,
                                         nsMallocSizeOfFun mallocSizeOf,
                                         void *);

  nsClassHashtable<nsCStringHashKey, CacheEntry> mTable;
  nsRefPtr<nsZipArchive> mArchive;
  nsCOMPtr<nsIFile> mFile;
  
  nsCOMPtr<nsIObserverService> mObserverService;
  nsRefPtr<StartupCacheListener> mListener;
  nsCOMPtr<nsITimer> mTimer;

  bool mStartupWriteInitiated;

  static StartupCache *gStartupCache;
  static bool gShutdownInitiated;
  static bool gIgnoreDiskCache;
  PRThread *mWriteThread;
#ifdef DEBUG
  nsTHashtable<nsISupportsHashKey> mWriteObjectMap;
#endif

  nsIMemoryReporter* mMappingMemoryReporter;
  nsIMemoryReporter* mDataMemoryReporter;
};




#ifdef DEBUG
class StartupCacheDebugOutputStream MOZ_FINAL
  : public nsIObjectOutputStream
{  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBJECTOUTPUTSTREAM

  StartupCacheDebugOutputStream (nsIObjectOutputStream* binaryStream,
                                   nsTHashtable<nsISupportsHashKey>* objectMap)
  : mBinaryStream(binaryStream), mObjectMap(objectMap) { }
  
  NS_FORWARD_SAFE_NSIBINARYOUTPUTSTREAM(mBinaryStream)
  NS_FORWARD_SAFE_NSIOUTPUTSTREAM(mBinaryStream)
  
  bool CheckReferences(nsISupports* aObject);
  
  nsCOMPtr<nsIObjectOutputStream> mBinaryStream;
  nsTHashtable<nsISupportsHashKey> *mObjectMap;
};
#endif 


#define NS_STARTUPCACHE_CID \
      {0xae4505a9, 0x87ab, 0x477c, \
      {0xb5, 0x77, 0xf9, 0x23, 0x57, 0xed, 0xa8, 0x84}}


class StartupCacheWrapper MOZ_FINAL
  : public nsIStartupCache
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTARTUPCACHE

  static StartupCacheWrapper* GetSingleton();
  static StartupCacheWrapper *gStartupCacheWrapper;
};

} 
} 
#endif
