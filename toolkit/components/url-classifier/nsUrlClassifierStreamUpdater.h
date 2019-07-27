




#ifndef nsUrlClassifierStreamUpdater_h_
#define nsUrlClassifierStreamUpdater_h_

#include <nsISupportsUtils.h>

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIUrlClassifierStreamUpdater.h"
#include "nsIStreamListener.h"
#include "nsIChannel.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "mozilla/Attributes.h"


class nsIURI;

class nsUrlClassifierStreamUpdater final : public nsIUrlClassifierStreamUpdater,
                                           public nsIUrlClassifierUpdateObserver,
                                           public nsIStreamListener,
                                           public nsIObserver,
                                           public nsIInterfaceRequestor,
                                           public nsITimerCallback
{
public:
  nsUrlClassifierStreamUpdater();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERSTREAMUPDATER
  NS_DECL_NSIURLCLASSIFIERUPDATEOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK

private:
  
  ~nsUrlClassifierStreamUpdater() {}

  
  
  void DownloadDone();

  
  nsUrlClassifierStreamUpdater(nsUrlClassifierStreamUpdater&);

  nsresult AddRequestBody(const nsACString &aRequestBody);

  
  nsresult FetchUpdate(nsIURI *aURI,
                       const nsACString &aRequestBody,
                       const nsACString &aTable);
  
  nsresult FetchUpdate(const nsACString &aURI,
                       const nsACString &aRequestBody,
                       const nsACString &aTable);

  
  nsresult FetchNext();
  
  nsresult FetchNextRequest();


  bool mIsUpdating;
  bool mInitialized;
  bool mDownloadError;
  bool mBeganStream;
  nsCString mStreamTable;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIUrlClassifierDBService> mDBService;
  nsCOMPtr<nsITimer> mTimer;

  struct PendingRequest {
    nsCString mTables;
    nsCString mRequest;
    nsCString mUrl;
    nsCOMPtr<nsIUrlClassifierCallback> mSuccessCallback;
    nsCOMPtr<nsIUrlClassifierCallback> mUpdateErrorCallback;
    nsCOMPtr<nsIUrlClassifierCallback> mDownloadErrorCallback;
  };
  nsTArray<PendingRequest> mPendingRequests;

  struct PendingUpdate {
    nsCString mUrl;
    nsCString mTable;
  };
  nsTArray<PendingUpdate> mPendingUpdates;

  nsCOMPtr<nsIUrlClassifierCallback> mSuccessCallback;
  nsCOMPtr<nsIUrlClassifierCallback> mUpdateErrorCallback;
  nsCOMPtr<nsIUrlClassifierCallback> mDownloadErrorCallback;
};

#endif 
