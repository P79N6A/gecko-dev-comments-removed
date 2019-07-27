





#ifndef mozilla_net_PackagedAppService_h
#define mozilla_net_PackagedAppService_h

#include "nsIPackagedAppService.h"
#include "nsILoadContextInfo.h"
#include "nsICacheStorage.h"

namespace mozilla {
namespace net {
















class PackagedAppService final
  : public nsIPackagedAppService
{
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPACKAGEDAPPSERVICE

  PackagedAppService();

private:
  ~PackagedAppService();

  
  
  
  
  
  
  
  
  
  
  nsresult OpenNewPackageInternal(nsIURI *aURI,
                                  nsICacheEntryOpenCallback *aCallback,
                                  nsILoadContextInfo *aInfo);

  
  
  
  nsresult NotifyPackageDownloaded(nsCString aKey);

  
  
  
  
  class CacheEntryWriter final
    : public nsIStreamListener
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    
    
    
    
    static nsresult Create(nsIURI*, nsICacheStorage*, CacheEntryWriter**);

    nsCOMPtr<nsICacheEntry> mEntry;
  private:
    CacheEntryWriter() { }
    ~CacheEntryWriter() { }

    
    
    nsresult CopySecurityInfo(nsIChannel *aChannel);

    
    
    static NS_METHOD ConsumeData(nsIInputStream *in, void *closure,
                                 const char *fromRawSegment, uint32_t toOffset,
                                 uint32_t count, uint32_t *writeCount);
    
    
    nsCOMPtr<nsIOutputStream> mOutputStream;
  };

  
  
  
  
  
  
  
  
  class PackagedAppDownloader final
    : public nsIStreamListener
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    
    
    
    
    nsresult Init(nsILoadContextInfo* aInfo, const nsCString &aKey);
    
    
    nsresult AddCallback(nsIURI *aURI, nsICacheEntryOpenCallback *aCallback);

  private:
    ~PackagedAppDownloader() { }

    
    
    
    nsresult CallCallbacks(nsIURI *aURI, nsICacheEntry *aEntry, nsresult aResult);
    
    
    
    
    nsresult ClearCallbacks(nsresult aResult);
    static PLDHashOperator ClearCallbacksEnumerator(const nsACString& key,
      nsAutoPtr<nsCOMArray<nsICacheEntryOpenCallback>>& callbackArray,
      void* arg);
    
    
    
    static nsresult GetSubresourceURI(nsIRequest * aRequest, nsIURI **aResult);
    
    
    nsRefPtr<CacheEntryWriter> mWriter;
    
    nsCOMPtr<nsICacheStorage> mCacheStorage;
    
    
    
    
    nsClassHashtable<nsCStringHashKey, nsCOMArray<nsICacheEntryOpenCallback>> mCallbacks;
    
    
    nsCString mPackageKey;
  };

  
  
  
  
  
  class CacheEntryChecker final
    : public nsICacheEntryOpenCallback
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHEENTRYOPENCALLBACK

    CacheEntryChecker(nsIURI *aURI, nsICacheEntryOpenCallback * aCallback,
                      nsILoadContextInfo *aInfo)
      : mURI(aURI)
      , mCallback(aCallback)
      , mLoadContextInfo(aInfo)
    {
    }
  private:
    ~CacheEntryChecker() { }

    nsCOMPtr<nsIURI> mURI;
    nsCOMPtr<nsICacheEntryOpenCallback> mCallback;
    nsCOMPtr<nsILoadContextInfo> mLoadContextInfo;
  };

  
  
  
  nsRefPtrHashtable<nsCStringHashKey, PackagedAppDownloader> mDownloadingPackages;
};


} 
} 

#endif 