





































#ifndef nsUrlClassifierHashCompleter_h_
#define nsUrlClassifierHashCompleter_h_

#include "nsIUrlClassifierHashCompleter.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIChannel.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsWeakReference.h"

class nsUrlClassifierHashCompleterRequest : public nsIStreamListener
                                          , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIOBSERVER

  nsUrlClassifierHashCompleterRequest(nsIURI *uri)
    : mShuttingDown(PR_FALSE)
    , mURI(uri) { }
  ~nsUrlClassifierHashCompleterRequest() { }

  nsresult Begin();
  nsresult Add(const nsACString &partialHash,
               nsIUrlClassifierHashCompleterCallback *c);

private:
  nsresult OpenChannel();
  nsresult BuildRequest(nsCAutoString &request);
  nsresult AddRequestBody(const nsACString &requestBody);
  nsresult HandleItem(const nsACString &item,
                      const nsACString &table,
                      PRUint32 item);
  nsresult HandleTable(const nsACString &response,
                       nsACString::const_iterator &begin);
  nsresult HandleResponse();
  void NotifySuccess();
  void NotifyFailure(nsresult status);

  PRBool mShuttingDown;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIChannel> mChannel;
  nsCString mResponse;

  struct Response {
    nsCString completeHash;
    nsCString tableName;
    PRUint32 chunkId;
  };

  struct Request {
    nsCString partialHash;
    nsTArray<Response> responses;
    nsCOMPtr<nsIUrlClassifierHashCompleterCallback> callback;
  };

  nsTArray<Request> mRequests;
};

class nsUrlClassifierHashCompleter : public nsIUrlClassifierHashCompleter
                                   , public nsIRunnable
                                   , public nsIObserver
                                   , public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERHASHCOMPLETER
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIOBSERVER

  nsUrlClassifierHashCompleter() : mShuttingDown(PR_FALSE) {}
  ~nsUrlClassifierHashCompleter() {}

  nsresult Init();

private:
  nsRefPtr<nsUrlClassifierHashCompleterRequest> mRequest;
  nsCOMPtr<nsIURI> mURI;
  PRBool mShuttingDown;

};

#endif 
