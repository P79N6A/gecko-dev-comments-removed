





#ifndef ApplicationReputation_h__
#define ApplicationReputation_h__

#include "nsIApplicationReputation.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsISupports.h"

#include "nsCOMPtr.h"
#include "nsString.h"

class nsIRequest;
class nsIUrlClassifierDBService;
class nsIScriptSecurityManager;
class PendingLookup;
class PRLogModuleInfo;

class ApplicationReputationService MOZ_FINAL :
  public nsIApplicationReputationService {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPLICATIONREPUTATIONSERVICE

public:
  static ApplicationReputationService* GetSingleton();

private:
  friend class PendingLookup;
  


  static ApplicationReputationService* gApplicationReputationService;
  


  static PRLogModuleInfo* prlog;
  


  nsCOMPtr<nsIUrlClassifierDBService> mDBService;
  nsCOMPtr<nsIScriptSecurityManager> mSecurityManager;
  


  ApplicationReputationService();
  ~ApplicationReputationService();
  



  nsresult QueryReputationInternal(nsIApplicationReputationQuery* aQuery,
                                   nsIApplicationReputationCallback* aCallback);
};
#endif 
