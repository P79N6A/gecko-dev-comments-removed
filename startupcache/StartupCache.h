





































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









































class nsIMemoryReporter;

namespace mozilla {
namespace scache {

struct CacheEntry 
{
  nsAutoArrayPtr<char> data;
  PRUint32 size;

  CacheEntry() : data(nsnull), size(0) { }

  
  CacheEntry(char* buf, PRUint32 len) : data(buf), size(len) { }

  ~CacheEntry()
  {
  }
};



class StartupCacheListener : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

class StartupCache
{

friend class StartupCacheListener;
friend class StartupCacheWrapper;
                                
public:

  

  
  nsresult GetBuffer(const char* id, char** outbuf, PRUint32* length);

  
  nsresult PutBuffer(const char* id, const char* inbuf, PRUint32 length);

  
  void InvalidateCache();

  
  
  nsresult GetDebugObjectOutputStream(nsIObjectOutputStream* aStream,
                                      nsIObjectOutputStream** outStream);

  static StartupCache* GetSingleton();
  static void DeleteSingleton();

  PRInt64 SizeOfMapping();

private:
  StartupCache();
  ~StartupCache();

  nsresult LoadArchive();
  nsresult Init();
  void WriteToDisk();
  nsresult ResetStartupWriteTimer();
  void WaitOnWriteThread();

  static nsresult InitSingleton();
  static void WriteTimeout(nsITimer *aTimer, void *aClosure);
  static void ThreadedWrite(void *aClosure);

  nsClassHashtable<nsCStringHashKey, CacheEntry> mTable;
  nsAutoPtr<nsZipArchive> mArchive;
  nsCOMPtr<nsILocalFile> mFile;
  
  nsCOMPtr<nsIObserverService> mObserverService;
  nsRefPtr<StartupCacheListener> mListener;
  nsCOMPtr<nsITimer> mTimer;

  bool mStartupWriteInitiated;

  static StartupCache *gStartupCache;
  static bool gShutdownInitiated;
  PRThread *mWriteThread;
#ifdef DEBUG
  nsTHashtable<nsISupportsHashKey> mWriteObjectMap;
#endif

  nsIMemoryReporter* mMemoryReporter;
};




#ifdef DEBUG
class StartupCacheDebugOutputStream
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


class StartupCacheWrapper 
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
