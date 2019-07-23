





































#ifndef nsUrlClassifierStreamUpdater_h_
#define nsUrlClassifierStreamUpdater_h_

#include <nsISupportsUtils.h>

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIUrlClassifierStreamUpdater.h"
#include "nsIStreamListener.h"
#include "nsNetUtil.h"
#include "nsTArray.h"


class nsIURI;

class nsUrlClassifierStreamUpdater : public nsIUrlClassifierStreamUpdater,
                                     public nsIUrlClassifierUpdateObserver,
                                     public nsIStreamListener,
                                     public nsIObserver
{
public:
  nsUrlClassifierStreamUpdater();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERSTREAMUPDATER
  NS_DECL_NSIURLCLASSIFIERUPDATEOBSERVER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIOBSERVER

private:
  
  ~nsUrlClassifierStreamUpdater() {}

  
  
  void DownloadDone();

  
  nsUrlClassifierStreamUpdater(nsUrlClassifierStreamUpdater&);

  nsresult AddRequestBody(const nsACString &aRequestBody);

  nsresult FetchUpdate(nsIURI *aURI, const nsACString &aRequestBody);
  nsresult FetchUpdate(const nsACString &aURI, const nsACString &aRequestBody);

  PRBool mIsUpdating;
  PRBool mInitialized;
  nsCOMPtr<nsIURI> mUpdateUrl;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIUrlClassifierDBService> mDBService;

  nsTArray<nsCAutoString> mPendingUpdateUrls;

  nsCOMPtr<nsIUrlClassifierCallback> mSuccessCallback;
  nsCOMPtr<nsIUrlClassifierCallback> mUpdateErrorCallback;
  nsCOMPtr<nsIUrlClassifierCallback> mDownloadErrorCallback;
};

#endif 
