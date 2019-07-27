





#ifndef AsyncFaviconHelpers_h_
#define AsyncFaviconHelpers_h_

#include "nsIFaviconService.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIStreamListener.h"
#include "nsThreadUtils.h"

#include "Database.h"
#include "mozilla/storage.h"

#define ICON_STATUS_UNKNOWN 0
#define ICON_STATUS_CHANGED 1 << 0
#define ICON_STATUS_SAVED 1 << 1
#define ICON_STATUS_ASSOCIATED 1 << 2
#define ICON_STATUS_CACHED 1 << 3

#define TO_CHARBUFFER(_buffer) \
  reinterpret_cast<char*>(const_cast<uint8_t*>(_buffer))
#define TO_INTBUFFER(_string) \
  reinterpret_cast<uint8_t*>(const_cast<char*>(_string.get()))







#define MAX_FAVICON_EXPIRATION ((PRTime)7 * 24 * 60 * 60 * PR_USEC_PER_SEC)

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
    guid.SetIsVoid(true);
  }

  int64_t id;
  nsCString spec;
  nsCString data;
  nsCString mimeType;
  PRTime expiration;
  enum AsyncFaviconFetchMode fetchMode;
  uint16_t status; 
  nsCString guid;
};




struct PageData
{
  PageData()
  : id(0)
  , canAddToHistory(true)
  , iconId(0)
  {
    guid.SetIsVoid(true);
  }

  int64_t id;
  nsCString spec;
  nsCString bookmarkedSpec;
  nsString revHost;
  bool canAddToHistory; 
  int64_t iconId;
  nsCString guid;
};





class AsyncFaviconHelperBase : public nsRunnable
{
protected:
  explicit AsyncFaviconHelperBase(nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncFaviconHelperBase();

  nsRefPtr<Database> mDB;
  
  nsCOMPtr<nsIFaviconDataCallback> mCallback;
};





class AsyncFetchAndSetIconForPage : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  












  static nsresult start(nsIURI* aFaviconURI,
                        nsIURI* aPageURI,
                        enum AsyncFaviconFetchMode aFetchMode,
                        uint32_t aFaviconLoadType,
                        nsIFaviconDataCallback* aCallback);

  









  AsyncFetchAndSetIconForPage(IconData& aIcon,
                              PageData& aPage,
                              uint32_t aFaviconLoadType,
                              nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncFetchAndSetIconForPage();

protected:
  IconData mIcon;
  PageData mPage;
  const bool mFaviconLoadPrivate;
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
                                  bool aFaviconLoadPrivate,
                                  nsCOMPtr<nsIFaviconDataCallback>& aCallback);

protected:
  virtual ~AsyncFetchAndSetIconFromNetwork();

  IconData mIcon;
  PageData mPage;
  const bool mFaviconLoadPrivate;
};





class AsyncAssociateIconToPage : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  









  AsyncAssociateIconToPage(IconData& aIcon,
                           PageData& aPage,
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
                        nsIFaviconDataCallback* aCallback);

  







  AsyncGetFaviconURLForPage(const nsACString& aPageSpec,
                            nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncGetFaviconURLForPage();

private:
  nsCString mPageSpec;
};






class AsyncGetFaviconDataForPage : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  







  static nsresult start(nsIURI* aPageURI,
                        nsIFaviconDataCallback* aCallback);

  







  AsyncGetFaviconDataForPage(const nsACString& aPageSpec,
                             nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncGetFaviconDataForPage();

private:
  nsCString mPageSpec;
};

class AsyncReplaceFaviconData : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  static nsresult start(IconData *aIcon);

  AsyncReplaceFaviconData(IconData &aIcon,
                          nsCOMPtr<nsIFaviconDataCallback>& aCallback);

  virtual ~AsyncReplaceFaviconData();

protected:
  IconData mIcon;
};

class RemoveIconDataCacheEntry : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  RemoveIconDataCacheEntry(IconData &aIcon,
                           nsCOMPtr<nsIFaviconDataCallback>& aCallback);
  virtual ~RemoveIconDataCacheEntry();

protected:
  IconData mIcon;
};




class NotifyIconObservers : public AsyncFaviconHelperBase
{
public:
  NS_DECL_NSIRUNNABLE

  









  NotifyIconObservers(IconData& aIcon,
                      PageData& aPage,
                      nsCOMPtr<nsIFaviconDataCallback>& aCallback);
  virtual ~NotifyIconObservers();

protected:
  IconData mIcon;
  PageData mPage;

  void SendGlobalNotifications(nsIURI* aIconURI);
};

} 
} 

#endif 
