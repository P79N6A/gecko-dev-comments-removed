




































#ifndef nsUrlClassifierProxies_h
#define nsUrlClassifierProxies_h

#include "nsIUrlClassifierDBService.h"
#include "nsThreadUtils.h"
#include "LookupCache.h"

using namespace mozilla::safebrowsing;




class UrlClassifierDBServiceWorkerProxy :
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
                   const nsACString& aSpec,
                   nsIUrlClassifierCallback* aCB)
      : mTarget(aTarget)
      , mSpec(aSpec)
      , mCB(aCB)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
    nsCString mSpec;
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
                             CacheResultArray *aEntries)
      : mTarget(aTarget)
      , mEntries(aEntries)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
     CacheResultArray *mEntries;
  };

  class CacheMissesRunnable : public nsRunnable
  {
  public:
    CacheMissesRunnable(nsIUrlClassifierDBServiceWorker* aTarget,
                        PrefixArray *aEntries)
      : mTarget(aTarget)
      , mEntries(aEntries)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
    PrefixArray *mEntries;
  };

private:
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> mTarget;
};



class UrlClassifierLookupCallbackProxy : public nsIUrlClassifierLookupCallback
{
public:
  UrlClassifierLookupCallbackProxy(nsIUrlClassifierLookupCallback* aTarget)
    : mTarget(aTarget)
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERLOOKUPCALLBACK

  class LookupCompleteRunnable : public nsRunnable
  {
  public:
    LookupCompleteRunnable(nsIUrlClassifierLookupCallback* aTarget,
                           LookupResultArray *aResults)
      : mTarget(aTarget)
      , mResults(aResults)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierLookupCallback> mTarget;
    LookupResultArray * mResults;
  };

private:
  nsCOMPtr<nsIUrlClassifierLookupCallback> mTarget;
};

class UrlClassifierCallbackProxy : public nsIUrlClassifierCallback
{
public:
  UrlClassifierCallbackProxy(nsIUrlClassifierCallback* aTarget)
    : mTarget(aTarget)
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERCALLBACK

  class HandleEventRunnable : public nsRunnable
  {
  public:
    HandleEventRunnable(nsIUrlClassifierCallback* aTarget,
                        const nsACString& aValue)
      : mTarget(aTarget)
      , mValue(aValue)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierCallback> mTarget;
    nsCString mValue;
  };

private:
  nsCOMPtr<nsIUrlClassifierCallback> mTarget;
};

class UrlClassifierUpdateObserverProxy : public nsIUrlClassifierUpdateObserver
{
public:
  UrlClassifierUpdateObserverProxy(nsIUrlClassifierUpdateObserver* aTarget)
    : mTarget(aTarget)
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERUPDATEOBSERVER

  class UpdateUrlRequestedRunnable : public nsRunnable
  {
  public:
    UpdateUrlRequestedRunnable(nsIUrlClassifierUpdateObserver* aTarget,
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
    nsCOMPtr<nsIUrlClassifierUpdateObserver> mTarget;
    nsCString mURL, mTable, mServerMAC;
  };

  class StreamFinishedRunnable : public nsRunnable
  {
  public:
    StreamFinishedRunnable(nsIUrlClassifierUpdateObserver* aTarget,
                           nsresult aStatus, PRUint32 aDelay)
      : mTarget(aTarget)
      , mStatus(aStatus)
      , mDelay(aDelay)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierUpdateObserver> mTarget;
    nsresult mStatus;
    PRUint32 mDelay;
  };

  class UpdateErrorRunnable : public nsRunnable
  {
  public:
    UpdateErrorRunnable(nsIUrlClassifierUpdateObserver* aTarget,
                        nsresult aError)
      : mTarget(aTarget)
      , mError(aError)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierUpdateObserver> mTarget;
    nsresult mError;
  };

  class UpdateSuccessRunnable : public nsRunnable
  {
  public:
    UpdateSuccessRunnable(nsIUrlClassifierUpdateObserver* aTarget,
                          PRUint32 aRequestedTimeout)
      : mTarget(aTarget)
      , mRequestedTimeout(aRequestedTimeout)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIUrlClassifierUpdateObserver> mTarget;
    PRUint32 mRequestedTimeout;
  };

private:
  nsCOMPtr<nsIUrlClassifierUpdateObserver> mTarget;
};

#endif 
