




#ifndef nsUrlClassifierDBService_h_
#define nsUrlClassifierDBService_h_

#include <nsISupportsUtils.h>

#include "nsID.h"
#include "nsInterfaceHashtable.h"
#include "nsIObserver.h"
#include "nsUrlClassifierPrefixSet.h"
#include "nsIUrlClassifierHashCompleter.h"
#include "nsIUrlClassifierDBService.h"
#include "nsIURIClassifier.h"
#include "nsToolkitCompsCID.h"
#include "nsICryptoHash.h"
#include "nsICryptoHMAC.h"
#include "mozilla/Attributes.h"

#include "LookupCache.h"


#define DOMAIN_LENGTH 4


#define PARTIAL_LENGTH 4


#define COMPLETE_LENGTH 32

class nsUrlClassifierDBServiceWorker;
class nsIThread;
class nsIURI;



class nsUrlClassifierDBService MOZ_FINAL : public nsIUrlClassifierDBService,
                                           public nsIURIClassifier,
                                           public nsIObserver
{
public:
  
  nsUrlClassifierDBService();

  nsresult Init();

  static nsUrlClassifierDBService* GetInstance(nsresult *result);

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_URLCLASSIFIERDBSERVICE_CID)

  NS_DECL_ISUPPORTS
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

  nsresult LookupURI(nsIPrincipal* aPrincipal, nsIUrlClassifierCallback* c,
                     bool forceCheck, bool *didCheck);

  
  nsresult Shutdown();

  
  nsresult CheckClean(const nsACString &lookupKey,
                      bool *clean);

  nsCOMPtr<nsUrlClassifierDBServiceWorker> mWorker;
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> mWorkerProxy;

  nsInterfaceHashtable<nsCStringHashKey, nsIUrlClassifierHashCompleter> mCompleters;

  
  
  bool mCheckMalware;

  
  
  bool mCheckPhishing;

  
  
  bool mPerClientRandomize;

  
  
  
  
  bool mInUpdate;

  
  nsTArray<nsCString> mGethashWhitelist;

  
  static nsIThread* gDbBackgroundThread;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsUrlClassifierDBService, NS_URLCLASSIFIERDBSERVICE_CID)

#endif 
