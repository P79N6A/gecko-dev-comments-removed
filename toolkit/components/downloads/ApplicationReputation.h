





#ifndef ApplicationReputation_h__
#define ApplicationReputation_h__

#include "nsIApplicationReputation.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsISupports.h"

#include "nsCOMPtr.h"
#include "nsString.h"

class nsIRequest;
class PendingDBLookup;
class PendingLookup;
struct PRLogModuleInfo;

class ApplicationReputationService final :
  public nsIApplicationReputationService {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPLICATIONREPUTATIONSERVICE

public:
  static ApplicationReputationService* GetSingleton();

private:
  friend class PendingLookup;
  friend class PendingDBLookup;
  


  static ApplicationReputationService* gApplicationReputationService;
  


  static PRLogModuleInfo* prlog;
  


  ApplicationReputationService();
  ~ApplicationReputationService();
  



  nsresult QueryReputationInternal(nsIApplicationReputationQuery* aQuery,
                                   nsIApplicationReputationCallback* aCallback);
};
#endif 
