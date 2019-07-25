





































#ifndef AsyncFaviconHelpers_h_
#define AsyncFaviconHelpers_h_

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIURI.h"
#include "nsThreadUtils.h"

#include "nsFaviconService.h"
#include "Helpers.h"

#include "mozilla/storage.h"

#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIStreamListener.h"

#define ICON_STATUS_UNKNOWN 0
#define ICON_STATUS_CHANGED 1 << 0
#define ICON_STATUS_SAVED 1 << 1
#define ICON_STATUS_ASSOCIATED 1 << 2

namespace mozilla {
namespace places {




enum AsyncFaviconFetchMode {
  FETCH_NEVER = 0
, FETCH_IF_MISSING
, FETCH_ALWAYS
};




struct IconData
{
  IconData()
  : id(0)
  , expiration(0)
  , fetchMode(FETCH_NEVER)
  , status(ICON_STATUS_UNKNOWN)
  {
  }

  PRInt64 id;
  nsCString spec;
  nsCString data;
  nsCString mimeType;
  PRTime expiration;
  enum AsyncFaviconFetchMode fetchMode;
  PRUint16 status; 
};




struct PageData
{
  PageData()
  : id(0)
  , canAddToHistory(true)
  , iconId(0)
  {
  }

  PRInt64 id;
  nsCString spec;
  nsCString bookmarkedSpec;
  nsString revHost;
  bool canAddToHistory; 
  PRInt64 iconId;
};






class AsyncFaviconHelperBase : public nsRunnable
{
protected:
  AsyncFaviconHelperBase(nsCOMPtr<mozIStorageConnection>& aDBConn,
                         nsRefPtr<nsFaviconService>& aFaviconSvc,
                         nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncFaviconHelperBase();

  nsCOMPtr<mozIStorageConnection>& mDBConn;
  
  nsRefPtr<nsFaviconService> mFaviconSvc;
  
  nsCOMPtr<nsIFaviconDataCallback> mCallback;
};





class AsyncFetchAndSetIconForPage : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  














  static nsresult start(nsIURI* aFaviconURI,
                        nsIURI* aPageURI,
                        enum AsyncFaviconFetchMode aFetchMode,
                        nsCOMPtr<mozIStorageConnection>& aDBConn,
                        nsIFaviconDataCallback* aCallback);

  











  AsyncFetchAndSetIconForPage(IconData& aIcon,
                              PageData& aPage,
                              nsCOMPtr<mozIStorageConnection>& aDBConn,
                              nsRefPtr<nsFaviconService>& aFaviconSvc,
                              nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncFetchAndSetIconForPage();

protected:
  IconData mIcon;
  PageData mPage;
};






class AsyncFetchAndSetIconFromNetwork : public AsyncFaviconHelperBase
                                      , public nsIStreamListener
                                      , public nsIInterfaceRequestor
                                      , public nsIChannelEventSink
{
public:
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIRUNNABLE
  NS_DECL_ISUPPORTS_INHERITED

  











  AsyncFetchAndSetIconFromNetwork(IconData& aIcon,
                                  PageData& aPage,
                                  nsCOMPtr<mozIStorageConnection>& aDBConn,
                                  nsRefPtr<nsFaviconService>& aFaviconSvc,
                                  nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncFetchAndSetIconFromNetwork();

protected:
  IconData mIcon;
  PageData mPage;
  nsCOMPtr<nsIChannel> mChannel;
};





class AsyncAssociateIconToPage : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  











  AsyncAssociateIconToPage(IconData& aIcon,
                           PageData& aPage,
                           nsCOMPtr<mozIStorageConnection>& aDBConn,
                           nsRefPtr<nsFaviconService>& aFaviconSvc,
                           nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncAssociateIconToPage();

protected:
  IconData mIcon;
  PageData mPage;
};





class AsyncGetFaviconURLForPage : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  









  static nsresult start(nsIURI* aPageURI,
                        nsCOMPtr<mozIStorageConnection>& aDBConn,
                        nsIFaviconDataCallback* aCallback);

  











  AsyncGetFaviconURLForPage(const nsACString& aPageSpec,
                            nsCOMPtr<mozIStorageConnection>& aDBConn,
                            nsRefPtr<nsFaviconService>& aFaviconSvc,
                            nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncGetFaviconURLForPage();

private:
  nsCString mPageSpec;
};




class NotifyIconObservers : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  NotifyIconObservers(IconData& aIcon,
                      PageData& aPage,
                      nsCOMPtr<mozIStorageConnection>& aDBConn,
                      nsRefPtr<nsFaviconService>& aFaviconSvc,
                      nsCOMPtr<nsIFaviconDataCallback>& aCallback);
  virtual ~NotifyIconObservers();

protected:
  IconData mIcon;
  PageData mPage;
};

} 
} 

#endif 
