





































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

class nsUrlClassifierHashCompleter;

class nsUrlClassifierHashCompleterRequest : public nsIStreamListener
                                          , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIOBSERVER

  nsUrlClassifierHashCompleterRequest(nsUrlClassifierHashCompleter *completer)
    : mShuttingDown(PR_FALSE)
    , mCompleter(completer)
    , mVerified(PR_FALSE)
    , mRescheduled(PR_FALSE)
    { }
  ~nsUrlClassifierHashCompleterRequest() { }

  nsresult Begin();
  nsresult Add(const nsACString &partialHash,
               nsIUrlClassifierHashCompleterCallback *c);

  void SetURI(nsIURI *uri) { mURI = uri; }
  void SetClientKey(const nsACString &clientKey) { mClientKey = clientKey; }

private:
  nsresult OpenChannel();
  nsresult BuildRequest(nsCAutoString &request);
  nsresult AddRequestBody(const nsACString &requestBody);
  void RescheduleItems();
  nsresult HandleMAC(nsACString::const_iterator &begin,
                     const nsACString::const_iterator &end);
  nsresult HandleItem(const nsACString &item,
                      const nsACString &table,
                      PRUint32 chunkId);
  nsresult HandleTable(nsACString::const_iterator &begin,
                       const nsACString::const_iterator &end);
  nsresult HandleResponse();
  void NotifySuccess();
  void NotifyFailure(nsresult status);

  PRBool mShuttingDown;
  nsRefPtr<nsUrlClassifierHashCompleter> mCompleter;
  nsCOMPtr<nsIURI> mURI;
  nsCString mClientKey;
  nsCOMPtr<nsIChannel> mChannel;
  nsCString mResponse;
  PRBool mVerified;
  PRBool mRescheduled;

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

  nsUrlClassifierHashCompleter()
    : mBackoff(PR_FALSE)
    , mBackoffTime(0)
    , mNextRequestTime(0)
    , mShuttingDown(PR_FALSE)
    {}
  ~nsUrlClassifierHashCompleter() {}

  nsresult Init();

  nsresult RekeyRequested();

  void NoteServerResponse(PRBool success);
  PRIntervalTime GetNextRequestTime() { return mNextRequestTime; }

private:
  nsRefPtr<nsUrlClassifierHashCompleterRequest> mRequest;
  nsCString mGethashUrl;
  nsCString mClientKey;
  nsCString mWrappedKey;

  nsTArray<PRIntervalTime> mErrorTimes;
  PRBool mBackoff;
  PRUint32 mBackoffTime;
  PRIntervalTime mNextRequestTime;

  PRBool mShuttingDown;
};

#endif 
