





#ifndef ApplicationReputation_h__
#define ApplicationReputation_h__

#include "nsIApplicationReputation.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsISupports.h"
#include "nsIUrlClassifierDBService.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsIRequest;
class nsIUrlClassifierDBService;
class nsIScriptSecurityManager;
class PendingLookup;

class ApplicationReputationService MOZ_FINAL :
  public nsIApplicationReputationService {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPLICATIONREPUTATIONSERVICE

public:
  static ApplicationReputationService* GetSingleton();

private:
  


  static ApplicationReputationService* gApplicationReputationService;
  


  nsCOMPtr<nsIUrlClassifierDBService> mDBService;
  nsCOMPtr<nsIScriptSecurityManager> mSecurityManager;
  


  ApplicationReputationService();
  ~ApplicationReputationService();
  



  nsresult QueryReputationInternal(nsIApplicationReputationQuery* aQuery,
                                   nsIApplicationReputationCallback* aCallback);
};
#endif 
