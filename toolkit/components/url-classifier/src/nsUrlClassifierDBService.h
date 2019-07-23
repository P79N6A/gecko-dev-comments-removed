






































#ifndef nsUrlClassifierDBService_h_
#define nsUrlClassifierDBService_h_

#include <nsISupportsUtils.h>

#include "nsID.h"
#include "nsIObserver.h"
#include "nsIUrlClassifierDBService.h"

class nsUrlClassifierDBServiceWorker;



class nsUrlClassifierDBService : public nsIUrlClassifierDBService,
                                 public nsIObserver
{
public:
  
  nsUrlClassifierDBService();

  nsresult Init();

  static nsUrlClassifierDBService* GetInstance();

#ifdef MOZILLA_1_8_BRANCH
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_URLCLASSIFIERDBSERVICE_CID)
#else
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_URLCLASSIFIERDBSERVICE_CID)
#endif

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERDBSERVICE
  NS_DECL_NSIOBSERVER

private:
  
  ~nsUrlClassifierDBService();

  
  nsUrlClassifierDBService(nsUrlClassifierDBService&);

  
  void EnsureThreadStarted();
  
  
  nsresult Shutdown();
  
  nsCOMPtr<nsUrlClassifierDBServiceWorker> mWorker;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsUrlClassifierDBService, NS_URLCLASSIFIERDBSERVICE_CID)

#endif 
