




#ifndef nsUrlClassifierDBService_h_
#define nsUrlClassifierDBService_h_

#include <nsISupportsUtils.h>

#include "nsID.h"
#include "nsInterfaceHashtable.h"
#include "nsIObserver.h"
#include "nsUrlClassifierPrefixSet.h"
#include "nsIUrlClassifierHashCompleter.h"
#include "nsIUrlListManager.h"
#include "nsIUrlClassifierDBService.h"
#include "nsIURIClassifier.h"
#include "nsToolkitCompsCID.h"
#include "nsICryptoHMAC.h"
#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"

#include "Entries.h"
#include "LookupCache.h"


#define DOMAIN_LENGTH 4


#define PARTIAL_LENGTH 4


#define COMPLETE_LENGTH 32

using namespace mozilla::safebrowsing;

class nsUrlClassifierDBServiceWorker;
class nsIThread;
class nsIURI;
class UrlClassifierDBServiceWorkerProxy;
namespace mozilla {
namespace safebrowsing {
class Classifier;
class ProtocolParser;
class TableUpdate;
} 
} 



class nsUrlClassifierDBService final : public nsIUrlClassifierDBService,
                                       public nsIURIClassifier,
                                       public nsIObserver
{
public:
  
  nsUrlClassifierDBService();

  nsresult Init();

  static nsUrlClassifierDBService* GetInstance(nsresult *result);

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_URLCLASSIFIERDBSERVICE_CID)

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERDBSERVICE
  NS_DECL_NSIURICLASSIFIER
  NS_DECL_NSIOBSERVER

  bool GetCompleter(const nsACString& tableName,
                      nsIUrlClassifierHashCompleter** completer);
  nsresult CacheCompletions(mozilla::safebrowsing::CacheResultArray *results);
  nsresult CacheMisses(mozilla::safebrowsing::PrefixArray *results);

  static nsIThread* BackgroundThread();

private:
  
  ~nsUrlClassifierDBService();

  
  nsUrlClassifierDBService(nsUrlClassifierDBService&);

  nsresult LookupURI(nsIPrincipal* aPrincipal,
                     const nsACString& tables,
                     nsIUrlClassifierCallback* c,
                     bool forceCheck, bool *didCheck);

  
  nsresult Shutdown();

  
  nsresult CheckClean(const nsACString &lookupKey,
                      bool *clean);

  
  nsresult ReadTablesFromPrefs();

  
  void BuildTables(bool trackingProtectionEnabled, nsCString& tables);

  nsRefPtr<nsUrlClassifierDBServiceWorker> mWorker;
  nsRefPtr<UrlClassifierDBServiceWorkerProxy> mWorkerProxy;

  nsInterfaceHashtable<nsCStringHashKey, nsIUrlClassifierHashCompleter> mCompleters;

  
  
  bool mCheckMalware;

  
  
  bool mCheckPhishing;

  
  
  bool mCheckTracking;

  
  
  
  
  bool mInUpdate;

  
  nsTArray<nsCString> mGethashTables;

  
  nsTArray<nsCString> mDisallowCompletionsTables;

  
  static nsIThread* gDbBackgroundThread;
};

class nsUrlClassifierDBServiceWorker final :
  public nsIUrlClassifierDBServiceWorker
{
public:
  nsUrlClassifierDBServiceWorker();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERDBSERVICE
  NS_DECL_NSIURLCLASSIFIERDBSERVICEWORKER

  nsresult Init(uint32_t aGethashNoise, nsCOMPtr<nsIFile> aCacheDir);

  
  
  nsresult QueueLookup(const nsACString& lookupKey,
                       const nsACString& tables,
                       nsIUrlClassifierLookupCallback* callback);

  
  
  nsresult HandlePendingLookups();

  
  
  nsresult DoLocalLookup(const nsACString& spec,
                         const nsACString& tables,
                         LookupResultArray* results);

private:
  
  ~nsUrlClassifierDBServiceWorker();

  
  nsUrlClassifierDBServiceWorker(nsUrlClassifierDBServiceWorker&);

  
  nsresult ApplyUpdate();

  
  void ResetStream();

  
  void ResetUpdate();

  
  nsresult DoLookup(const nsACString& spec,
                    const nsACString& tables,
                    nsIUrlClassifierLookupCallback* c);

  nsresult AddNoise(const Prefix aPrefix,
                    const nsCString tableName,
                    uint32_t aCount,
                    LookupResultArray& results);

  
  nsCOMPtr<nsICryptoHash> mCryptoHash;

  nsAutoPtr<mozilla::safebrowsing::Classifier> mClassifier;
  
  nsAutoPtr<ProtocolParser> mProtocolParser;

  
  nsCOMPtr<nsIFile> mCacheDir;

  
  
  nsTArray<mozilla::safebrowsing::TableUpdate*> mTableUpdates;

  int32_t mUpdateWait;

  
  
  PrefixArray mMissCache;

  nsresult mUpdateStatus;
  nsTArray<nsCString> mUpdateTables;

  nsCOMPtr<nsIUrlClassifierUpdateObserver> mUpdateObserver;
  bool mInStream;

  
  uint32_t mGethashNoise;

  
  
  mozilla::Mutex mPendingLookupLock;

  class PendingLookup {
  public:
    mozilla::TimeStamp mStartTime;
    nsCString mKey;
    nsCString mTables;
    nsCOMPtr<nsIUrlClassifierLookupCallback> mCallback;
  };

  
  nsTArray<PendingLookup> mPendingLookups;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsUrlClassifierDBService, NS_URLCLASSIFIERDBSERVICE_CID)

#endif 
