














































#include "nsFaviconService.h"
#include "nsICacheVisitor.h"
#include "nsICachingChannel.h"
#include "nsICategoryManager.h"
#include "nsIChannelEventSink.h"
#include "nsIContentSniffer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIStreamListener.h"
#include "nsISupportsPrimitives.h"
#include "nsNavBookmarks.h"
#include "nsNavHistory.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsStreamUtils.h"
#include "mozStorageHelper.h"






#define MAX_FAVICON_SIZE 32768

#define FAVICON_BUFFER_INCREMENT 8192

#define MAX_FAVICON_CACHE_SIZE 512
#define FAVICON_CACHE_REDUCE_COUNT 64

#define CONTENT_SNIFFING_SERVICES "content-sniffing-services"


class FaviconLoadListener : public nsIStreamListener,
                            public nsIInterfaceRequestor,
                            public nsIChannelEventSink
{
public:
  FaviconLoadListener(nsFaviconService* aFaviconService,
                      nsIURI* aPageURI, nsIURI* aFaviconURI,
                      nsIChannel* aChannel);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

private:
  ~FaviconLoadListener();

  nsCOMPtr<nsFaviconService> mFaviconService;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIURI> mPageURI;
  nsCOMPtr<nsIURI> mFaviconURI;

  nsCString mData;
};


nsFaviconService* nsFaviconService::gFaviconService;

NS_IMPL_ISUPPORTS1(nsFaviconService, nsIFaviconService)



nsFaviconService::nsFaviconService() : mFailedFaviconSerial(0)
{
  NS_ASSERTION(! gFaviconService, "ATTEMPTING TO CREATE TWO FAVICON SERVICES!");
  gFaviconService = this;
}




nsFaviconService::~nsFaviconService()
{
  NS_ASSERTION(gFaviconService == this, "Deleting a non-singleton favicon service");
  if (gFaviconService == this)
    gFaviconService = nsnull;
}






nsresult
nsFaviconService::Init()
{
  nsresult rv;

  nsNavHistory* historyService = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(historyService, NS_ERROR_OUT_OF_MEMORY);
  mDBConn = historyService->GetStorageConnection();
  NS_ENSURE_TRUE(mDBConn, NS_ERROR_FAILURE);

  

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("SELECT id, length(data), expiration FROM moz_favicons WHERE url = ?1"),
                                getter_AddRefs(mDBGetIconInfo));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("SELECT f.id, f.url, length(f.data), f.expiration FROM moz_places h JOIN moz_favicons f ON h.favicon_id = f.id WHERE h.url = ?1"),
                                getter_AddRefs(mDBGetURL));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("SELECT f.data, f.mime_type FROM moz_favicons f WHERE url = ?1"),
                                getter_AddRefs(mDBGetData));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("INSERT INTO moz_favicons (url, data, mime_type, expiration) VALUES (?1, ?2, ?3, ?4)"),
                                getter_AddRefs(mDBInsertIcon));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("UPDATE moz_favicons SET data = ?2, mime_type = ?3, expiration = ?4 where id = ?1"),
                                getter_AddRefs(mDBUpdateIcon));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("UPDATE moz_places SET favicon_id = ?2 WHERE id = ?1"),
                                getter_AddRefs(mDBSetPageFavicon));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (! mFailedFavicons.Init(256))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}








nsresult 
nsFaviconService::InitTables(mozIStorageConnection* aDBConn)
{
  nsresult rv;
  PRBool exists = PR_FALSE;
  aDBConn->TableExists(NS_LITERAL_CSTRING("moz_favicons"), &exists);
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_favicons (id INTEGER PRIMARY KEY, url LONGVARCHAR UNIQUE, data BLOB, mime_type VARCHAR(32), expiration LONG)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}




NS_IMETHODIMP
nsFaviconService::SetFaviconUrlForPage(nsIURI* aPage, nsIURI* aFavicon)
{
  
  PRBool hasData;
  PRTime expiration;
  nsresult rv = SetFaviconUrlForPageInternal(aPage, aFavicon,
                                             &hasData, &expiration);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (hasData)
    SendFaviconNotifications(aPage, aFavicon);
  return NS_OK;
}



NS_IMETHODIMP
nsFaviconService::GetDefaultFavicon(nsIURI** _retval)
{
  
  if (!mDefaultIcon) {
    nsresult rv = NS_NewURI(getter_AddRefs(mDefaultIcon),
                            NS_LITERAL_CSTRING(FAVICON_DEFAULT_URL));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return mDefaultIcon->Clone(_retval);
}

















nsresult
nsFaviconService::SetFaviconUrlForPageInternal(nsIURI* aPage, nsIURI* aFavicon,
                                               PRBool* aHasData,
                                               PRTime* aExpiration)
{
  mozStorageStatementScoper scoper(mDBGetIconInfo);
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  nsresult rv = BindStatementURI(mDBGetIconInfo, 0, aFavicon);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = mDBGetIconInfo->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 iconId;
  if (hasResult) {
    
    rv = mDBGetIconInfo->GetInt64(0, &iconId);
    NS_ENSURE_SUCCESS(rv, rv);

    
    PRInt32 dataSize;
    rv = mDBGetIconInfo->GetInt32(1, &dataSize);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dataSize > 0)
      *aHasData = PR_TRUE;

    
    rv = mDBGetIconInfo->GetInt64(2, aExpiration);
    NS_ENSURE_SUCCESS(rv, rv);

    mDBGetIconInfo->Reset();
    scoper.Abandon();
  } else {
    
    mDBGetIconInfo->Reset();
    scoper.Abandon();

    *aHasData = PR_FALSE;
    *aExpiration = 0;

    
    mozStorageStatementScoper scoper2(mDBInsertIcon);
    rv = BindStatementURI(mDBInsertIcon, 0, aFavicon);
    NS_ENSURE_SUCCESS(rv, rv);
    mDBInsertIcon->BindNullParameter(1);
    mDBInsertIcon->BindNullParameter(2);
    mDBInsertIcon->BindNullParameter(3);
    rv = mDBInsertIcon->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->GetLastInsertRowID(&iconId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsNavHistory* historyService = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(historyService, NS_ERROR_NO_INTERFACE);

  PRInt64 pageId;
  rv = historyService->GetUrlIdFor(aPage, &pageId, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  mozStorageStatementScoper scoper2(mDBSetPageFavicon);
  rv = mDBSetPageFavicon->BindInt64Parameter(0, pageId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBSetPageFavicon->BindInt64Parameter(1, iconId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBSetPageFavicon->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  transaction.Commit();
  return NS_OK;
}














nsresult
nsFaviconService::UpdateBookmarkRedirectFavicon(nsIURI* aPage, nsIURI* aFavicon)
{
  NS_ENSURE_TRUE(aPage, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(aFavicon, NS_ERROR_INVALID_ARG);

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIURI> bookmarkURI;
  nsresult rv = bookmarks->GetBookmarkedURIFor(aPage, getter_AddRefs(bookmarkURI));
  NS_ENSURE_SUCCESS(rv, rv);
  if (! bookmarkURI)
    return NS_OK; 

  PRBool sameAsBookmark;
  if (NS_SUCCEEDED(bookmarkURI->Equals(aPage, &sameAsBookmark)) && sameAsBookmark)
    return NS_OK; 

  PRBool hasData = PR_FALSE;
  PRTime expiration = 0;
  rv = SetFaviconUrlForPageInternal(bookmarkURI, aFavicon, &hasData, &expiration);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasData) {
    
    SendFaviconNotifications(bookmarkURI, aFavicon);
  } else {
    NS_WARNING("Calling UpdateBookmarkRedirectFavicon when you don't have data for the favicon yet.");
  }
  return NS_OK;
}







void
nsFaviconService::SendFaviconNotifications(nsIURI* aPage, nsIURI* aFaviconURI)
{
  nsCAutoString faviconSpec;
  nsNavHistory* historyService = nsNavHistory::GetHistoryService();
  if (historyService && NS_SUCCEEDED(aFaviconURI->GetSpec(faviconSpec))) {
    historyService->SendPageChangedNotification(aPage,
                                       nsINavHistoryObserver::ATTRIBUTE_FAVICON,
                                       NS_ConvertUTF8toUTF16(faviconSpec));
  }
}




NS_IMETHODIMP
nsFaviconService::SetAndLoadFaviconForPage(nsIURI* aPage, nsIURI* aFavicon,
                                           PRBool aForceReload)
{
#ifdef LAZY_ADD
  nsNavHistory* historyService = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(historyService, NS_ERROR_OUT_OF_MEMORY);
  return historyService->AddLazyLoadFaviconMessage(aPage, aFavicon,
                                                   aForceReload);
#else
  return DoSetAndLoadFaviconForPage(aPage, aFavicon, aForceReload);
#endif
}




nsresult
nsFaviconService::DoSetAndLoadFaviconForPage(nsIURI* aPage, nsIURI* aFavicon,
                                             PRBool aForceReload)
{
  nsCOMPtr<nsIURI> page(aPage);

  
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_FAILURE);
  if (history->IsHistoryDisabled()) {
    
    
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIURI> bookmarkURI;
    nsresult rv = bookmarks->GetBookmarkedURIFor(aPage,
                                                 getter_AddRefs(bookmarkURI));
    NS_ENSURE_SUCCESS(rv, rv);
    if (! bookmarkURI) {
      
      return NS_OK;
    }

    
    
    
    
    page = bookmarkURI;
  }

  
  PRBool previouslyFailed;
  nsresult rv = IsFailedFavicon(aFavicon, &previouslyFailed);
  NS_ENSURE_SUCCESS(rv, rv);
  if (previouslyFailed) {
    if (aForceReload)
      RemoveFailedFavicon(aFavicon); 
    else
      return NS_OK; 
  }

  
  PRBool canAdd;
  rv = history->CanAddURI(page, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! canAdd)
    return NS_OK; 

  
  
  
  
  PRBool pageEqualsFavicon;
  rv = page->Equals(aFavicon, &pageEqualsFavicon);
  NS_ENSURE_SUCCESS(rv, rv);
  if (pageEqualsFavicon)
    return NS_OK;

  
  
  
  PRBool isDataURL;
  rv = aFavicon->SchemeIs("data", &isDataURL);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isDataURL)
    return NS_OK;

  
  
  
  
  
  
  PRBool hasData = PR_FALSE;
  PRTime expiration = 0;
  { 
    mozStorageStatementScoper scoper(mDBGetIconInfo);
    rv = BindStatementURI(mDBGetIconInfo, 0, aFavicon);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMatch;
    rv = mDBGetIconInfo->ExecuteStep(&hasMatch);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasMatch) {
      PRInt32 dataSize;
      mDBGetIconInfo->GetInt32(1, &dataSize);
      hasData = dataSize > 0;
      mDBGetIconInfo->GetInt64(2, &expiration);
    }
  }

  
  
  PRTime now = PR_Now();
  if (hasData && now < expiration && ! aForceReload) {
    

    
    
    
    
    nsCOMPtr<nsIURI> oldFavicon;
    PRBool faviconsEqual;
    if (NS_SUCCEEDED(GetFaviconForPage(page, getter_AddRefs(oldFavicon))) &&
        NS_SUCCEEDED(aFavicon->Equals(oldFavicon, &faviconsEqual)) &&
        faviconsEqual)
      return NS_OK; 

    
    rv = SetFaviconUrlForPageInternal(page, aFavicon, &hasData, &expiration);
    NS_ENSURE_SUCCESS(rv, rv);

    SendFaviconNotifications(page, aFavicon);
    UpdateBookmarkRedirectFavicon(page, aFavicon);
    return NS_OK;
  }

  nsCOMPtr<nsIIOService> ioservice = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel;
  rv = ioservice->NewChannelFromURI(aFavicon, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStreamListener> listener =
    new FaviconLoadListener(this, page, aFavicon, channel);
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
  nsCOMPtr<nsIInterfaceRequestor> listenerRequestor =
    do_QueryInterface(listener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = channel->SetNotificationCallbacks(listenerRequestor);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = channel->AsyncOpen(listener, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  return NS_OK;
}







NS_IMETHODIMP
nsFaviconService::SetFaviconData(nsIURI* aFavicon, const PRUint8* aData,
                                 PRUint32 aDataLen, const nsACString& aMimeType,
                                 PRTime aExpiration)
{
  nsresult rv;
  mozIStorageStatement* statement;
  {
    
    
    mozStorageStatementScoper scoper(mDBGetIconInfo);
    rv = BindStatementURI(mDBGetIconInfo, 0, aFavicon);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = mDBGetIconInfo->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);

    if (hasResult) {
      
      PRInt64 id;
      rv = mDBGetIconInfo->GetInt64(0, &id);
      NS_ENSURE_SUCCESS(rv, rv);
      statement = mDBUpdateIcon;
      rv = statement->BindInt64Parameter(0, id);
    } else {
      
      statement = mDBInsertIcon;
      rv = BindStatementURI(statement, 0, aFavicon);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mozStorageStatementScoper scoper(statement);

  
  rv = statement->BindBlobParameter(1, aData, aDataLen);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(2, aMimeType);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(3, aExpiration);
  NS_ENSURE_SUCCESS(rv, rv);
  return statement->Execute();
}




NS_IMETHODIMP
nsFaviconService::GetFaviconData(nsIURI* aFavicon, nsACString& aMimeType,
                                 PRUint32* aDataLen, PRUint8** aData)
{
  mozStorageStatementScoper scoper(mDBGetData);
  nsresult rv = BindStatementURI(mDBGetData, 0, aFavicon);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = mDBGetData->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! hasResult)
    return NS_ERROR_NOT_AVAILABLE;

  rv = mDBGetData->GetUTF8String(1, aMimeType);
  NS_ENSURE_SUCCESS(rv, rv);
  return mDBGetData->GetBlob(0, aDataLen, aData);
}




NS_IMETHODIMP
nsFaviconService::GetFaviconForPage(nsIURI* aPage, nsIURI** _retval)
{
  mozStorageStatementScoper scoper(mDBGetURL);
  nsresult rv = BindStatementURI(mDBGetURL, 0, aPage);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = mDBGetURL->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! hasResult)
    return NS_ERROR_NOT_AVAILABLE;

  nsCAutoString url;
  rv = mDBGetURL->GetUTF8String(1, url);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_NewURI(_retval, url);
}




NS_IMETHODIMP
nsFaviconService::GetFaviconImageForPage(nsIURI* aPage, nsIURI** _retval)
{
  mozStorageStatementScoper scoper(mDBGetURL);
  nsresult rv = BindStatementURI(mDBGetURL, 0, aPage);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  rv = mDBGetURL->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> faviconURI;
  if (hasResult)
  {
    PRInt32 dataLen;
    rv = mDBGetURL->GetInt32(2, &dataLen);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dataLen > 0) {
      
      nsCAutoString favIconUri;
      rv = mDBGetURL->GetUTF8String(1, favIconUri);
      NS_ENSURE_SUCCESS(rv, rv);

      return GetFaviconLinkForIconString(favIconUri, _retval);
    }
  }

  
  return GetDefaultFavicon(_retval);
}




nsresult
nsFaviconService::GetFaviconLinkForIcon(nsIURI* aIcon, nsIURI** aOutput)
{
  nsCAutoString spec;
  if (aIcon) {
    nsresult rv = aIcon->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return GetFaviconLinkForIconString(spec, aOutput);
}




PR_STATIC_CALLBACK(PLDHashOperator)
ExpireFailedFaviconsCallback(nsCStringHashKey::KeyType aKey,
                             PRUint32& aData,
                             void* userArg)
{
  PRUint32* threshold = NS_REINTERPRET_CAST(PRUint32*, userArg);
  if (aData < *threshold)
    return PL_DHASH_REMOVE;
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsFaviconService::AddFailedFavicon(nsIURI* aIcon)
{
  nsCAutoString spec;
  nsresult rv = aIcon->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  if (! mFailedFavicons.Put(spec, mFailedFaviconSerial))
    return NS_ERROR_OUT_OF_MEMORY;
  mFailedFaviconSerial ++;

  if (mFailedFavicons.Count() > MAX_FAVICON_CACHE_SIZE) {
    
    
    PRUint32 threshold = mFailedFaviconSerial -
      MAX_FAVICON_CACHE_SIZE + FAVICON_CACHE_REDUCE_COUNT;
    mFailedFavicons.Enumerate(ExpireFailedFaviconsCallback, &threshold);
  }
  return NS_OK;
}




NS_IMETHODIMP
nsFaviconService::RemoveFailedFavicon(nsIURI* aIcon)
{
  nsCAutoString spec;
  nsresult rv = aIcon->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mFailedFavicons.Remove(spec);
  return NS_OK;
}




NS_IMETHODIMP
nsFaviconService::IsFailedFavicon(nsIURI* aIcon, PRBool* _retval)
{
  nsCAutoString spec;
  nsresult rv = aIcon->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 serial;
  *_retval = mFailedFavicons.Get(spec, &serial);
  return NS_OK;
}







nsresult
nsFaviconService::GetFaviconLinkForIconString(const nsCString& aSpec, nsIURI** aOutput)
{
  if (aSpec.IsEmpty()) {
    
    if (! mDefaultIcon) {
      nsresult rv = NS_NewURI(getter_AddRefs(mDefaultIcon),
                              NS_LITERAL_CSTRING(FAVICON_DEFAULT_URL));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return mDefaultIcon->Clone(aOutput);
  }

  if (StringBeginsWith(aSpec, NS_LITERAL_CSTRING("chrome:"))) {
    
    
    return NS_NewURI(aOutput, aSpec);
  }

  nsCAutoString annoUri;
  annoUri.AssignLiteral("moz-anno:" FAVICON_ANNOTATION_NAME ":");
  annoUri += aSpec;
  return NS_NewURI(aOutput, annoUri);
}







void
nsFaviconService::GetFaviconSpecForIconString(const nsCString& aSpec, nsACString& aOutput)
{
  if (aSpec.IsEmpty()) {
    aOutput.AssignLiteral(FAVICON_DEFAULT_URL);
  } else if (StringBeginsWith(aSpec, NS_LITERAL_CSTRING("chrome:"))) {
    aOutput = aSpec;
  } else {
    aOutput.AssignLiteral("moz-anno:" FAVICON_ANNOTATION_NAME ":");
    aOutput += aSpec;
  }
}


NS_IMPL_ISUPPORTS3(FaviconLoadListener,
                   nsIStreamListener, 
                   nsIInterfaceRequestor,
                   nsIChannelEventSink)



FaviconLoadListener::FaviconLoadListener(nsFaviconService* aFaviconService,
                                         nsIURI* aPageURI, nsIURI* aFaviconURI,
                                         nsIChannel* aChannel) :
  mFaviconService(aFaviconService),
  mChannel(aChannel),
  mPageURI(aPageURI),
  mFaviconURI(aFaviconURI)
{
}




FaviconLoadListener::~FaviconLoadListener()
{
}




NS_IMETHODIMP
FaviconLoadListener::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  return NS_OK;
}




NS_IMETHODIMP
FaviconLoadListener::OnStopRequest(nsIRequest *aRequest, nsISupports *aContext,
                                 nsresult aStatusCode)
{
  if (NS_FAILED(aStatusCode) || mData.Length() == 0) {
    
    mFaviconService->AddFailedFavicon(mFaviconURI);
    return NS_OK;
  }

  
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISimpleEnumerator> sniffers;
  rv = categoryManager->EnumerateCategory(CONTENT_SNIFFING_SERVICES,
                                          getter_AddRefs(sniffers));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString mimeType;
  PRBool hasMore = PR_FALSE;
  while (mimeType.IsEmpty() && NS_SUCCEEDED(sniffers->HasMoreElements(&hasMore))
         && hasMore) {
    nsCOMPtr<nsISupports> snifferCIDSupports;
    rv = sniffers->GetNext(getter_AddRefs(snifferCIDSupports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupportsCString> snifferCIDSupportsCString =
      do_QueryInterface(snifferCIDSupports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString snifferCID;
    rv = snifferCIDSupportsCString->GetData(snifferCID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIContentSniffer> sniffer = do_GetService(snifferCID.get(), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    sniffer->GetMIMETypeFromContent(
#ifndef MOZILLA_1_8_BRANCH
               aRequest,
#endif
               NS_REINTERPRET_CAST(PRUint8*, NS_CONST_CAST(char*, mData.get())),
               mData.Length(), mimeType);
    
  }

  if (mimeType.IsEmpty()) {
    
    mFaviconService->AddFailedFavicon(mFaviconURI);
    return NS_OK;
  }

  
  
  
  
  
  
  
  PRTime expiration = PR_Now() +
                      (PRInt64)(24 * 60 * 60) * (PRInt64)PR_USEC_PER_SEC;

  
  rv = mFaviconService->SetFaviconData(mFaviconURI,
               NS_REINTERPRET_CAST(PRUint8*, NS_CONST_CAST(char*, mData.get())),
               mData.Length(), mimeType, expiration);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool hasData;
  rv = mFaviconService->SetFaviconUrlForPageInternal(mPageURI, mFaviconURI,
                                                     &hasData, &expiration);
  NS_ENSURE_SUCCESS(rv, rv);

  mFaviconService->SendFaviconNotifications(mPageURI, mFaviconURI);
  mFaviconService->UpdateBookmarkRedirectFavicon(mPageURI, mFaviconURI);
  return NS_OK;
}




NS_IMETHODIMP
FaviconLoadListener::OnDataAvailable(nsIRequest *aRequest, nsISupports *aContext,
                                  nsIInputStream *aInputStream,
                                  PRUint32 aOffset, PRUint32 aCount)
{
  if (aOffset + aCount > MAX_FAVICON_SIZE)
    return NS_ERROR_FAILURE; 

  nsCString buffer;
  nsresult rv = NS_ConsumeStream(aInputStream, aCount, buffer);
  if (rv != NS_BASE_STREAM_WOULD_BLOCK && NS_FAILED(rv))
    return rv;

  mData.Append(buffer);
  return NS_OK;
}




NS_IMETHODIMP
FaviconLoadListener::GetInterface(const nsIID& uuid, void** aResult)
{
  return QueryInterface(uuid, aResult);
}




NS_IMETHODIMP
FaviconLoadListener::OnChannelRedirect(nsIChannel* oldChannel,
                                       nsIChannel* newChannel, PRUint32 flags)
{
  mChannel = newChannel;
  return NS_OK;
}


