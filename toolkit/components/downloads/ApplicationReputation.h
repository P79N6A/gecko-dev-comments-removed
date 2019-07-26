





#ifndef ApplicationReputation_h__
#define ApplicationReputation_h__

#include "nsIApplicationReputation.h"
#include "nsIRequestObserver.h"
#include "nsIStreamListener.h"
#include "nsISupports.h"

#include "nsCOMPtr.h"
#include "nsString.h"

class nsIApplicationReputationListener;
class nsIChannel;
class nsIObserverService;
class nsIRequest;
class PRLogModuleInfo;

class ApplicationReputationService MOZ_FINAL :
  public nsIApplicationReputationService {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPLICATIONREPUTATIONSERVICE

public:
  static ApplicationReputationService* GetSingleton();

private:
  


  static ApplicationReputationService* gApplicationReputationService;

  ApplicationReputationService();
  ~ApplicationReputationService();

  



  nsresult QueryReputationInternal(nsIApplicationReputationQuery* aQuery,
                                   nsIApplicationReputationCallback* aCallback);
};






class ApplicationReputationQuery MOZ_FINAL :
  public nsIApplicationReputationQuery,
  public nsIStreamListener {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIAPPLICATIONREPUTATIONQUERY

  ApplicationReputationQuery();
  ~ApplicationReputationQuery();

private:
  


  nsString mSuggestedFileName;
  nsCOMPtr<nsIURI> mURI;
  uint32_t mFileSize;
  nsCString mSha256Hash;

  


  nsCOMPtr<nsIApplicationReputationCallback> mCallback;

  




  nsCString mResponse;

  



  nsresult OnStopRequestInternal(nsIRequest *aRequest,
                                 nsISupports *aContext,
                                 nsresult aResult,
                                 bool* aShouldBlock);
};

#endif 
