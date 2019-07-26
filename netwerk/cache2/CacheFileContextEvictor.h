



#ifndef CacheFileContextEvictor__h__
#define CacheFileContextEvictor__h__

#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

class nsIFile;
class nsILoadContextInfo;

namespace mozilla {
namespace net {

class CacheIndexIterator;

struct CacheFileContextEvictorEntry
{
  nsCOMPtr<nsILoadContextInfo> mInfo;
  PRTime                       mTimeStamp; 
  nsRefPtr<CacheIndexIterator> mIterator;
};

class CacheFileContextEvictor
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CacheFileContextEvictor)

  CacheFileContextEvictor();

private:
  virtual ~CacheFileContextEvictor();

public:
  nsresult Init(nsIFile *aCacheDirectory);

  
  uint32_t ContextsCount();
  
  nsresult AddContext(nsILoadContextInfo *aLoadContextInfo);
  
  
  
  nsresult CacheIndexStateChanged();
  
  
  
  
  
  nsresult WasEvicted(const nsACString &aKey, nsIFile *aFile, bool *_retval);

private:
  
  
  
  
  nsresult PersistEvictionInfoToDisk(nsILoadContextInfo *aLoadContextInfo);
  
  
  nsresult RemoveEvictInfoFromDisk(nsILoadContextInfo *aLoadContextInfo);
  
  
  nsresult LoadEvictInfoFromDisk();
  nsresult GetContextFile(nsILoadContextInfo *aLoadContextInfo,
                          nsIFile **_retval);

  void     CreateIterators();
  void     CloseIterators();
  void     StartEvicting();
  nsresult EvictEntries();

  
  bool mEvicting;
  
  
  
  
  bool mIndexIsUpToDate;
  
  
  static bool sDiskAlreadySearched;
  
  nsTArray<nsAutoPtr<CacheFileContextEvictorEntry> > mEntries;
  nsCOMPtr<nsIFile> mCacheDirectory;
  nsCOMPtr<nsIFile> mEntriesDir;
};

} 
} 

#endif
