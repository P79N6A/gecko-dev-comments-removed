




#ifndef nsUrlClassifierProxies_h
#define nsUrlClassifierProxies_h

#include "nsIUrlClassifierDBService.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"
#include "nsIPrincipal.h"
#include "LookupCache.h"





class UrlClassifierDBServiceWorkerProxy MOZ_FINAL :
  public nsIUrlClassifierDBServiceWorker
{
public:
  UrlClassifierDBServiceWorkerProxy(nsIUrlClassifierDBServiceWorker* aTarget)
    : mTarget(aTarget)
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERDBSERVICE
  NS_DECL_NSIURLCLASSIFIERDBSERVICEWORKER

  class LookupRunnable : public nsRunnable
  {
  public:
    LookupRunnable(nsIUrlClassifierDBServiceWorker* aTarget,
                   nsIPrincipal* aPrincipal,
                   nsIUrlClassifierCallback* aCB)
      : mTarget(aTarget)
      , mPrincipal(aPrincipal)
      , mCB(aCB)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
    nsCOMPtr<nsIPrincipal> mPrincipal;
    nsCOMPtr<nsIUrlClassifierCallback> mCB;
  };

  class GetTablesRunnable : public nsRunnable
  {
  public:
    GetTablesRunnable(nsIUrlClassifierDBServiceWorker* aTarget,
                      nsIUrlClassifierCallback* aCB)
      : mTarget(aTarget)
      , mCB(aCB)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
    nsCOMPtr<nsIUrlClassifierCallback> mCB;
  };

  class BeginUpdateRunnable : public nsRunnable
  {
  public:
    BeginUpdateRunnable(nsIUrlClassifierDBServiceWorker* aTarget,
                        nsIUrlClassifierUpdateObserver* aUpdater,
                        const nsACString& aTables,
                        const nsACString& aClientKey)
      : mTarget(aTarget)
      , mUpdater(aUpdater)
      , mTables(aTables)
      , mClientKey(aClientKey)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
    nsCOMPtr<nsIUrlClassifierUpdateObserver> mUpdater;
    nsCString mTables, mClientKey;
  };

  class BeginStreamRunnable : public nsRunnable
  {
  public:
    BeginStreamRunnable(nsIUrlClassifierDBServiceWorker* aTarget,
                        const nsACString& aTable,
                        const nsACString& aServerMAC)
      : mTarget(aTarget)
      , mTable(aTable)
      , mServerMAC(aServerMAC)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
    nsCString mTable, mServerMAC;
  };

  class UpdateStreamRunnable : public nsRunnable
  {
  public:
    UpdateStreamRunnable(nsIUrlClassifierDBServiceWorker* aTarget,
                         const nsACString& aUpdateChunk)
      : mTarget(aTarget)
      , mUpdateChunk(aUpdateChunk)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
    nsCString mUpdateChunk;
  };

  class CacheCompletionsRunnable : public nsRunnable
  {
  public:
    CacheCompletionsRunnable(nsIUrlClassifierDBServiceWorker* aTarget,
                             mozilla::safebrowsing::CacheResultArray *aEntries)
      : mTarget(aTarget)
      , mEntries(aEntries)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
     mozilla::safebrowsing::CacheResultArray *mEntries;
  };

  class CacheMissesRunnable : public nsRunnable
  {
  public:
    CacheMissesRunnable(nsIUrlClassifierDBServiceWorker* aTarget,
                        mozilla::safebrowsing::PrefixArray *aEntries)
      : mTarget(aTarget)
      , mEntries(aEntries)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
    mozilla::safebrowsing::PrefixArray *mEntries;
  };

private:
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
};



class UrlClassifierLookupCallbackProxy MOZ_FINAL :
  public nsIUrlClassifierLookupCallback
{
public:
  UrlClassifierLookupCallbackProxy(nsIUrlClassifierLookupCallback* aTarget)
    : mTarget(new nsMainThreadPtrHolder<nsIUrlClassifierLookupCallback>(aTarget))
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERLOOKUPCALLBACK

  class LookupCompleteRunnable : public nsRunnable
  {
  public:
    LookupCompleteRunnable(const nsMainThreadPtrHandle<nsIUrlClassifierLookupCallback>& aTarget,
                           mozilla::safebrowsing::LookupResultArray *aResults)
      : mTarget(aTarget)
      , mResults(aResults)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIUrlClassifierLookupCallback> mTarget;
    mozilla::safebrowsing::LookupResultArray * mResults;
  };

private:
  nsMainThreadPtrHandle<nsIUrlClassifierLookupCallback> mTarget;
};

class UrlClassifierCallbackProxy MOZ_FINAL : public nsIUrlClassifierCallback
{
public:
  UrlClassifierCallbackProxy(nsIUrlClassifierCallback* aTarget)
    : mTarget(new nsMainThreadPtrHolder<nsIUrlClassifierCallback>(aTarget))
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERCALLBACK

  class HandleEventRunnable : public nsRunnable
  {
  public:
    HandleEventRunnable(const nsMainThreadPtrHandle<nsIUrlClassifierCallback>& aTarget,
                        const nsACString& aValue)
      : mTarget(aTarget)
      , mValue(aValue)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIUrlClassifierCallback> mTarget;
    nsCString mValue;
  };

private:
  nsMainThreadPtrHandle<nsIUrlClassifierCallback> mTarget;
};

class UrlClassifierUpdateObserverProxy MOZ_FINAL :
  public nsIUrlClassifierUpdateObserver
{
public:
  UrlClassifierUpdateObserverProxy(nsIUrlClassifierUpdateObserver* aTarget)
    : mTarget(new nsMainThreadPtrHolder<nsIUrlClassifierUpdateObserver>(aTarget))
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERUPDATEOBSERVER

  class UpdateUrlRequestedRunnable : public nsRunnable
  {
  public:
    UpdateUrlRequestedRunnable(const nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver>& aTarget,
                               const nsACString& aURL,
                               const nsACString& aTable,
                               const nsACString& aServerMAC)
      : mTarget(aTarget)
      , mURL(aURL)
      , mTable(aTable)
      , mServerMAC(aServerMAC)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver> mTarget;
    nsCString mURL, mTable, mServerMAC;
  };

  class RekeyRequestedRunnable : public nsRunnable
  {
  public:
    RekeyRequestedRunnable(const nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver>& aTarget)
      : mTarget(aTarget)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver> mTarget;
  };

  class StreamFinishedRunnable : public nsRunnable
  {
  public:
    StreamFinishedRunnable(const nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver>& aTarget,
                           nsresult aStatus, uint32_t aDelay)
      : mTarget(aTarget)
      , mStatus(aStatus)
      , mDelay(aDelay)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver> mTarget;
    nsresult mStatus;
    uint32_t mDelay;
  };

  class UpdateErrorRunnable : public nsRunnable
  {
  public:
    UpdateErrorRunnable(const nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver>& aTarget,
                        nsresult aError)
      : mTarget(aTarget)
      , mError(aError)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver> mTarget;
    nsresult mError;
  };

  class UpdateSuccessRunnable : public nsRunnable
  {
  public:
    UpdateSuccessRunnable(const nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver>& aTarget,
                          uint32_t aRequestedTimeout)
      : mTarget(aTarget)
      , mRequestedTimeout(aRequestedTimeout)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver> mTarget;
    uint32_t mRequestedTimeout;
  };

private:
  nsMainThreadPtrHandle<nsIUrlClassifierUpdateObserver> mTarget;
};

#endif 
