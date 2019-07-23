






































#ifndef nsUrlClassifierDBService_h_
#define nsUrlClassifierDBService_h_

#include <nsISupportsUtils.h>

#include "nsID.h"
#include "nsInterfaceHashtable.h"
#include "nsIObserver.h"
#include "nsIUrlClassifierHashCompleter.h"
#include "nsIUrlClassifierDBService.h"
#include "nsIURIClassifier.h"
#include "nsToolkitCompsCID.h"


#define DOMAIN_LENGTH 4


#define PARTIAL_LENGTH 4


#define COMPLETE_LENGTH 32

class nsUrlClassifierDBServiceWorker;



class nsUrlClassifierDBService : public nsIUrlClassifierDBService,
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

  PRBool GetCompleter(const nsACString& tableName,
                      nsIUrlClassifierHashCompleter** completer);
  nsresult CacheCompletions(nsTArray<nsUrlClassifierLookupResult> *results);

private:
  
  ~nsUrlClassifierDBService();

  
  nsUrlClassifierDBService(nsUrlClassifierDBService&);

  nsresult LookupURI(nsIURI* uri, nsIUrlClassifierCallback* c,
                     PRBool forceCheck, PRBool *didCheck);

  
  nsresult Shutdown();
  
  nsCOMPtr<nsUrlClassifierDBServiceWorker> mWorker;
  nsCOMPtr<nsUrlClassifierDBServiceWorker> mWorkerProxy;

  nsInterfaceHashtable<nsCStringHashKey, nsIUrlClassifierHashCompleter> mCompleters;

  
  
  PRBool mCheckMalware;

  
  
  PRBool mCheckPhishing;

  
  
  
  
  PRBool mInUpdate;

  
  nsTArray<nsCString> mGethashWhitelist;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsUrlClassifierDBService, NS_URLCLASSIFIERDBSERVICE_CID)

#endif 
