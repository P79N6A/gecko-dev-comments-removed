

















































#include "nsFaviconService.h"
#include "nsICacheService.h"
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
#include "nsStringStream.h"
#include "mozStorageHelper.h"
#include "plbase64.h"
#include "nsPlacesTables.h"
#include "mozIStoragePendingStatement.h"
#include "mozIStorageStatementCallback.h"
#include "mozIStorageError.h"
#include "nsPlacesTables.h"
#include "nsIPrefService.h"
#include "Helpers.h"


#include "imgITools.h"
#include "imgIContainer.h"


#define OPTIMIZED_FAVICON_DIMENSION 16



#define MAX_ICON_FILESIZE(s) ((PRUint32) s*s*4)

#define MAX_FAVICON_CACHE_SIZE 256
#define FAVICON_CACHE_REDUCE_COUNT 64

#define CONTENT_SNIFFING_SERVICES "content-sniffing-services"







#define MAX_FAVICON_EXPIRATION ((PRTime)7 * 24 * 60 * 60 * PR_USEC_PER_SEC)

using namespace mozilla::places;







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

  nsRefPtr<nsFaviconService> mFaviconService;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIURI> mPageURI;
  nsCOMPtr<nsIURI> mFaviconURI;

  nsCString mData;
};






class ExpireFaviconsStatementCallbackNotifier : public AsyncStatementCallback
{
public:
  ExpireFaviconsStatementCallbackNotifier(bool *aFaviconsExpirationRunning);
  NS_DECL_ISUPPORTS
  NS_DECL_ASYNCSTATEMENTCALLBACK

private:
  bool *mFaviconsExpirationRunning;
};

nsFaviconService* nsFaviconService::gFaviconService;

NS_IMPL_ISUPPORTS1(
  nsFaviconService
, nsIFaviconService
)

namespace {











already_AddRefed<nsIURI>
GetEffectivePageForFavicon(nsIURI *aPageURI,
                              nsIURI *aFaviconURI)
{
  NS_ASSERTION(aPageURI, "Must provide a pageURI!");
  NS_ASSERTION(aFaviconURI, "Must provide a favicon URI!");

  nsCOMPtr<nsIURI> pageURI(aPageURI);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, nsnull);

  PRBool canAddToHistory;
  nsresult rv = history->CanAddURI(pageURI, &canAddToHistory);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv), nsnull);

  
  
  if (!canAddToHistory || history->IsHistoryDisabled()) {
    nsNavBookmarks *bmSvc = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bmSvc, nsnull);

    
    nsCOMPtr<nsIURI> bookmarkedURI;
    rv = bmSvc->GetBookmarkedURIFor(aPageURI, getter_AddRefs(bookmarkedURI));
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && bookmarkedURI, nsnull);

    
    pageURI = bookmarkedURI.forget();
  }

  
  
  
  
  PRBool pageEqualsFavicon;
  rv = pageURI->Equals(aFaviconURI, &pageEqualsFavicon);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && !pageEqualsFavicon, nsnull);

  
  nsCOMPtr<nsIURI> errorPageFaviconURI;
  rv = NS_NewURI(getter_AddRefs(errorPageFaviconURI),
                 NS_LITERAL_CSTRING(FAVICON_ERRORPAGE_URL));
  NS_ENSURE_SUCCESS(rv, nsnull);
  PRBool isErrorPage;
  rv = aFaviconURI->Equals(errorPageFaviconURI, &isErrorPage);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && !isErrorPage, nsnull);

  
  return pageURI.forget();
}

} 


nsFaviconService::nsFaviconService() : mFaviconsExpirationRunning(false)
                                     , mOptimizedIconDimension(OPTIMIZED_FAVICON_DIMENSION)
                                     , mFailedFaviconSerial(0)
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
  
  nsNavHistory* historyService = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(historyService, NS_ERROR_OUT_OF_MEMORY);
  mDBConn = historyService->GetStorageConnection();
  NS_ENSURE_TRUE(mDBConn, NS_ERROR_FAILURE);

  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id, length(data), expiration FROM moz_favicons WHERE url = ?1"),
    getter_AddRefs(mDBGetIconInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT f.id, f.url, length(f.data), f.expiration "
      "FROM ( "
        "SELECT " MOZ_PLACES_COLUMNS " FROM moz_places_temp "
        "WHERE url = ?1 "
        "UNION ALL "
        "SELECT " MOZ_PLACES_COLUMNS " FROM moz_places "
        "WHERE url = ?1 "
      ") AS h JOIN moz_favicons f ON h.favicon_id = f.id "
      "LIMIT 1"),
    getter_AddRefs(mDBGetURL));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT f.data, f.mime_type FROM moz_favicons f WHERE url = ?1"),
    getter_AddRefs(mDBGetData));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_favicons (url, data, mime_type, expiration) "
      "VALUES (?1, ?2, ?3, ?4)"),
    getter_AddRefs(mDBInsertIcon));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_favicons SET data = ?2, mime_type = ?3, expiration = ?4 "
      "WHERE id = ?1"),
    getter_AddRefs(mDBUpdateIcon));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places_view SET favicon_id = ?2 WHERE id = ?1"),
    getter_AddRefs(mDBSetPageFavicon));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (! mFailedFavicons.Init(MAX_FAVICON_CACHE_SIZE))
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIPrefBranch> pb = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (pb)
    pb->GetIntPref("places.favicons.optimizeToDimension", &mOptimizedIconDimension);

  return NS_OK;
}







nsresult 
nsFaviconService::InitTables(mozIStorageConnection* aDBConn)
{
  nsresult rv;
  PRBool exists = PR_FALSE;
  aDBConn->TableExists(NS_LITERAL_CSTRING("moz_favicons"), &exists);
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(CREATE_MOZ_FAVICONS);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::ExpireAllFavicons()
{
  mFaviconsExpirationRunning = true;

  
  
  
  
  nsCOMPtr<mozIStorageStatement> removeOnDiskReferences;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places "
      "SET favicon_id = NULL "
      "WHERE favicon_id NOT NULL"
    ), getter_AddRefs(removeOnDiskReferences));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> removeTempReferences;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places_temp "
      "SET favicon_id = NULL "
      "WHERE favicon_id NOT NULL"
    ), getter_AddRefs(removeTempReferences));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsCOMPtr<mozIStorageStatement> removeFavicons;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_favicons WHERE id NOT IN ("
        "SELECT favicon_id FROM moz_places_temp WHERE favicon_id NOT NULL "
        "UNION ALL "
        "SELECT favicon_id FROM moz_places WHERE favicon_id NOT NULL "
      ")"
    ), getter_AddRefs(removeFavicons));
  NS_ENSURE_SUCCESS(rv, rv);

  mozIStorageStatement *stmts[] = {
    removeOnDiskReferences,
    removeTempReferences,
    removeFavicons
  };
  nsCOMPtr<mozIStoragePendingStatement> ps;
  nsCOMPtr<ExpireFaviconsStatementCallbackNotifier> callback =
    new ExpireFaviconsStatementCallbackNotifier(&mFaviconsExpirationRunning);
  rv = mDBConn->ExecuteAsync(stmts, NS_ARRAY_LENGTH(stmts), callback,
                             getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




NS_IMETHODIMP
nsFaviconService::SetFaviconUrlForPage(nsIURI* aPageURI, nsIURI* aFaviconURI)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG(aFaviconURI);

  if (mFaviconsExpirationRunning)
    return NS_OK;

  PRBool hasData;
  nsresult rv = SetFaviconUrlForPageInternal(aPageURI, aFaviconURI, &hasData);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (hasData)
    SendFaviconNotifications(aPageURI, aFaviconURI);
  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::GetDefaultFavicon(nsIURI** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  
  if (!mDefaultIcon) {
    nsresult rv = NS_NewURI(getter_AddRefs(mDefaultIcon),
                            NS_LITERAL_CSTRING(FAVICON_DEFAULT_URL));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return mDefaultIcon->Clone(_retval);
}















nsresult
nsFaviconService::SetFaviconUrlForPageInternal(nsIURI* aPageURI,
                                               nsIURI* aFaviconURI,
                                               PRBool* aHasData)
{
  nsresult rv;
  PRInt64 iconId = -1;
  *aHasData = PR_FALSE;

  nsNavHistory* historyService = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(historyService, NS_ERROR_OUT_OF_MEMORY);

  if (historyService->InPrivateBrowsingMode())
    return NS_OK;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  {
    mozStorageStatementScoper scoper(mDBGetIconInfo);
    rv = BindStatementURI(mDBGetIconInfo, 0, aFaviconURI);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult = PR_FALSE;
    if (NS_SUCCEEDED(mDBGetIconInfo->ExecuteStep(&hasResult)) && hasResult) {
      
      rv = mDBGetIconInfo->GetInt64(0, &iconId);
      NS_ENSURE_SUCCESS(rv, rv);

      
      PRInt32 dataSize;
      rv = mDBGetIconInfo->GetInt32(1, &dataSize);
      NS_ENSURE_SUCCESS(rv, rv);
      if (dataSize > 0)
        *aHasData = PR_TRUE;
    }
  }

  if (iconId == -1) {
    

    
    mozStorageStatementScoper scoper(mDBInsertIcon);
    rv = BindStatementURI(mDBInsertIcon, 0, aFaviconURI);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBInsertIcon->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    {
      mozStorageStatementScoper scoper(mDBGetIconInfo);

      rv = BindStatementURI(mDBGetIconInfo, 0, aFaviconURI);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasResult;
      rv = mDBGetIconInfo->ExecuteStep(&hasResult);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ASSERTION(hasResult, "hasResult is false but the call succeeded?");
      iconId = mDBGetIconInfo->AsInt64(0);
    }
  }

  
  PRInt64 pageId;
  rv = historyService->GetUrlIdFor(aPageURI, &pageId, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  mozStorageStatementScoper scoper(mDBSetPageFavicon);
  rv = mDBSetPageFavicon->BindInt64Parameter(0, pageId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBSetPageFavicon->BindInt64Parameter(1, iconId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBSetPageFavicon->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}













nsresult
nsFaviconService::UpdateBookmarkRedirectFavicon(nsIURI* aPageURI,
                                                nsIURI* aFaviconURI)
{
  NS_ENSURE_ARG_POINTER(aPageURI);
  NS_ENSURE_ARG_POINTER(aFaviconURI);

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIURI> bookmarkURI;
  nsresult rv = bookmarks->GetBookmarkedURIFor(aPageURI,
                                               getter_AddRefs(bookmarkURI));
  NS_ENSURE_SUCCESS(rv, rv);
  if (! bookmarkURI)
    return NS_OK; 

  PRBool sameAsBookmark;
  if (NS_SUCCEEDED(bookmarkURI->Equals(aPageURI, &sameAsBookmark)) &&
      sameAsBookmark)
    return NS_OK; 

  PRBool hasData = PR_FALSE;
  rv = SetFaviconUrlForPageInternal(bookmarkURI, aFaviconURI, &hasData);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasData) {
    
    SendFaviconNotifications(bookmarkURI, aFaviconURI);
  } else {
    NS_WARNING("Calling UpdateBookmarkRedirectFavicon when you don't have data for the favicon yet.");
  }
  return NS_OK;
}






void
nsFaviconService::SendFaviconNotifications(nsIURI* aPageURI,
                                           nsIURI* aFaviconURI)
{
  nsCAutoString faviconSpec;
  nsNavHistory* historyService = nsNavHistory::GetHistoryService();
  if (historyService && NS_SUCCEEDED(aFaviconURI->GetSpec(faviconSpec))) {
    historyService->SendPageChangedNotification(aPageURI,
                                       nsINavHistoryObserver::ATTRIBUTE_FAVICON,
                                       NS_ConvertUTF8toUTF16(faviconSpec));
  }
}

NS_IMETHODIMP
nsFaviconService::SetAndLoadFaviconForPage(nsIURI* aPageURI,
                                           nsIURI* aFaviconURI,
                                           PRBool aForceReload)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG(aFaviconURI);

  if (mFaviconsExpirationRunning)
    return NS_OK;

#ifdef LAZY_ADD
  nsNavHistory* historyService = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(historyService, NS_ERROR_OUT_OF_MEMORY);
  return historyService->AddLazyLoadFaviconMessage(aPageURI,
                                                   aFaviconURI,
                                                   aForceReload);
#else
  return DoSetAndLoadFaviconForPage(aPageURI, aFaviconURI, aForceReload);
#endif
}

nsresult
nsFaviconService::DoSetAndLoadFaviconForPage(nsIURI* aPageURI,
                                             nsIURI* aFaviconURI,
                                             PRBool aForceReload)
{
  if (mFaviconsExpirationRunning)
    return NS_OK;

  
  
  PRBool previouslyFailed;
  nsresult rv = IsFailedFavicon(aFaviconURI, &previouslyFailed);
  NS_ENSURE_SUCCESS(rv, rv);
  if (previouslyFailed) {
    if (aForceReload)
      RemoveFailedFavicon(aFaviconURI); 
    else
      return NS_OK; 
  }

  
  
  nsCOMPtr<nsIURI> page = GetEffectivePageForFavicon(aPageURI, aFaviconURI);
  NS_ENSURE_TRUE(page, NS_OK);

  
  
  
  
  
  PRBool hasData = PR_FALSE;
  PRTime expiration = 0;
  {
    mozStorageStatementScoper scoper(mDBGetIconInfo);
    rv = BindStatementURI(mDBGetIconInfo, 0, aFaviconURI);
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
  if (hasData && now < expiration && !aForceReload) {
    

    
    
    
    
    nsCOMPtr<nsIURI> oldFavicon;
    PRBool faviconsEqual;
    if (NS_SUCCEEDED(GetFaviconForPage(page, getter_AddRefs(oldFavicon))) &&
        NS_SUCCEEDED(aFaviconURI->Equals(oldFavicon, &faviconsEqual)) &&
        faviconsEqual)
      return NS_OK; 

    
    rv = SetFaviconUrlForPageInternal(page, aFaviconURI, &hasData);
    NS_ENSURE_SUCCESS(rv, rv);

    SendFaviconNotifications(page, aFaviconURI);
    UpdateBookmarkRedirectFavicon(page, aFaviconURI);
    return NS_OK;
  }

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), aFaviconURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStreamListener> listener =
    new FaviconLoadListener(this, page, aFaviconURI, channel);
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
nsFaviconService::SetFaviconData(nsIURI* aFaviconURI, const PRUint8* aData,
                                 PRUint32 aDataLen, const nsACString& aMimeType,
                                 PRTime aExpiration)
{
  NS_ENSURE_ARG(aFaviconURI);

  if (mFaviconsExpirationRunning)
    return NS_OK;

  nsresult rv;
  PRUint32 dataLen = aDataLen;
  const PRUint8* data = aData;
  const nsACString* mimeType = &aMimeType;
  nsCString newData, newMimeType;

  
  
  
  if (aDataLen > MAX_ICON_FILESIZE(mOptimizedIconDimension)) {
    rv = OptimizeFaviconImage(aData, aDataLen, aMimeType, newData, newMimeType);
    if (NS_SUCCEEDED(rv) && newData.Length() < aDataLen) {
      data = reinterpret_cast<PRUint8*>(const_cast<char*>(newData.get())),
      dataLen = newData.Length();
      mimeType = &newMimeType;
    }
    else if (aDataLen > MAX_FAVICON_SIZE) {
      
      
      return NS_ERROR_FAILURE;
    }
  }

  mozIStorageStatement* statement;
  {
    
    
    mozStorageStatementScoper scoper(mDBGetIconInfo);
    rv = BindStatementURI(mDBGetIconInfo, 0, aFaviconURI);
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
      rv = BindStatementURI(statement, 0, aFaviconURI);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mozStorageStatementScoper scoper(statement);

  
  rv = statement->BindBlobParameter(1, data, dataLen);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(2, *mimeType);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(3, aExpiration);
  NS_ENSURE_SUCCESS(rv, rv);
  return statement->Execute();
}

NS_IMETHODIMP
nsFaviconService::SetFaviconDataFromDataURL(nsIURI* aFaviconURI,
                                            const nsAString& aDataURL,
                                            PRTime aExpiration)
{
  NS_ENSURE_ARG(aFaviconURI);
  if (mFaviconsExpirationRunning)
    return NS_OK;

  nsCOMPtr<nsIURI> dataURI;
  nsresult rv = NS_NewURI(getter_AddRefs(dataURI), aDataURL);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIProtocolHandler> protocolHandler;
  rv = ioService->GetProtocolHandler("data", getter_AddRefs(protocolHandler));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel;
  rv = protocolHandler->NewChannel(dataURI, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 available;
  rv = stream->Available(&available);
  NS_ENSURE_SUCCESS(rv, rv);
  if (available == 0)
    return NS_ERROR_FAILURE;

  
  PRUint8* buffer = static_cast<PRUint8*>
                               (nsMemory::Alloc(sizeof(PRUint8) * available));
  if (!buffer)
    return NS_ERROR_OUT_OF_MEMORY;
  PRUint32 numRead;
  rv = stream->Read(reinterpret_cast<char*>(buffer), available, &numRead);
  if (NS_FAILED(rv) || numRead != available) {
    nsMemory::Free(buffer);
    return rv;
  }

  nsCAutoString mimeType;
  rv = channel->GetContentType(mimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = SetFaviconData(aFaviconURI, buffer, available, mimeType, aExpiration);
  nsMemory::Free(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::GetFaviconData(nsIURI* aFaviconURI, nsACString& aMimeType,
                                 PRUint32* aDataLen, PRUint8** aData)
{
  NS_ENSURE_ARG(aFaviconURI);
  NS_ENSURE_ARG_POINTER(aDataLen);
  NS_ENSURE_ARG_POINTER(aData);

  mozStorageStatementScoper scoper(mDBGetData);
  nsresult rv = BindStatementURI(mDBGetData, 0, aFaviconURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult = PR_FALSE;
  if (NS_SUCCEEDED(mDBGetData->ExecuteStep(&hasResult)) && hasResult) {
    rv = mDBGetData->GetUTF8String(1, aMimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    return mDBGetData->GetBlob(0, aDataLen, aData);
  }
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsFaviconService::GetFaviconDataAsDataURL(nsIURI* aFaviconURI,
                                          nsAString& aDataURL)
{
  NS_ENSURE_ARG(aFaviconURI);

  PRUint8* data;
  PRUint32 dataLen;
  nsCAutoString mimeType;

  nsresult rv = GetFaviconData(aFaviconURI, mimeType, &dataLen, &data);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!data) {
    aDataURL.SetIsVoid(PR_TRUE);
    return NS_OK;
  }

  char* encoded = PL_Base64Encode(reinterpret_cast<const char*>(data),
                                  dataLen, nsnull);
  nsMemory::Free(data);

  if (!encoded)
    return NS_ERROR_OUT_OF_MEMORY;

  aDataURL.AssignLiteral("data:");
  AppendUTF8toUTF16(mimeType, aDataURL);
  aDataURL.AppendLiteral(";base64,");
  AppendUTF8toUTF16(encoded, aDataURL);

  nsMemory::Free(encoded);
  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::GetFaviconForPage(nsIURI* aPageURI, nsIURI** _retval)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG_POINTER(_retval);

  mozStorageStatementScoper scoper(mDBGetURL);
  nsresult rv = BindStatementURI(mDBGetURL, 0, aPageURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  if (NS_SUCCEEDED(mDBGetURL->ExecuteStep(&hasResult)) && hasResult) {
    nsCAutoString url;
    rv = mDBGetURL->GetUTF8String(1, url);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_NewURI(_retval, url);
  }
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsFaviconService::GetFaviconImageForPage(nsIURI* aPageURI, nsIURI** _retval)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG_POINTER(_retval);

  mozStorageStatementScoper scoper(mDBGetURL);
  nsresult rv = BindStatementURI(mDBGetURL, 0, aPageURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  nsCOMPtr<nsIURI> faviconURI;
  if (NS_SUCCEEDED(mDBGetURL->ExecuteStep(&hasResult)) && hasResult) {
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
nsFaviconService::GetFaviconLinkForIcon(nsIURI* aFaviconURI,
                                        nsIURI** aOutputURI)
{
  NS_ENSURE_ARG(aFaviconURI);
  NS_ENSURE_ARG_POINTER(aOutputURI);

  nsCAutoString spec;
  if (aFaviconURI) {
    nsresult rv = aFaviconURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return GetFaviconLinkForIconString(spec, aOutputURI);
}

static PLDHashOperator
ExpireFailedFaviconsCallback(nsCStringHashKey::KeyType aKey,
                             PRUint32& aData,
                             void* userArg)
{
  PRUint32* threshold = reinterpret_cast<PRUint32*>(userArg);
  if (aData < *threshold)
    return PL_DHASH_REMOVE;
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsFaviconService::AddFailedFavicon(nsIURI* aFaviconURI)
{
  NS_ENSURE_ARG(aFaviconURI);

  nsCAutoString spec;
  nsresult rv = aFaviconURI->GetSpec(spec);
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
nsFaviconService::RemoveFailedFavicon(nsIURI* aFaviconURI)
{
  NS_ENSURE_ARG(aFaviconURI);

  nsCAutoString spec;
  nsresult rv = aFaviconURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mFailedFavicons.Remove(spec);
  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::IsFailedFavicon(nsIURI* aFaviconURI, PRBool* _retval)
{
  NS_ENSURE_ARG(aFaviconURI);
  nsCAutoString spec;
  nsresult rv = aFaviconURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 serial;
  *_retval = mFailedFavicons.Get(spec, &serial);
  return NS_OK;
}






nsresult
nsFaviconService::GetFaviconLinkForIconString(const nsCString& aSpec,
                                              nsIURI** aOutput)
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
nsFaviconService::GetFaviconSpecForIconString(const nsCString& aSpec,
                                              nsACString& aOutput)
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





nsresult
nsFaviconService::OptimizeFaviconImage(const PRUint8* aData, PRUint32 aDataLen,
                                       const nsACString& aMimeType,
                                       nsACString& aNewData,
                                       nsACString& aNewMimeType)
{
  nsresult rv;
  

  nsCOMPtr<imgITools> imgtool = do_CreateInstance("@mozilla.org/image/tools;1");

  nsCOMPtr<nsIInputStream> stream;
  rv = NS_NewByteInputStream(getter_AddRefs(stream),
                reinterpret_cast<const char*>(aData), aDataLen,
                NS_ASSIGNMENT_DEPEND);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<imgIContainer> container;
  rv = imgtool->DecodeImageData(stream, aMimeType, getter_AddRefs(container));
  NS_ENSURE_SUCCESS(rv, rv);

  aNewMimeType.AssignLiteral("image/png");

  
  nsCOMPtr<nsIInputStream> iconStream;
  rv = imgtool->EncodeScaledImage(container, aNewMimeType,
                                  mOptimizedIconDimension,
                                  mOptimizedIconDimension,
                                  getter_AddRefs(iconStream));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = NS_ConsumeStream(iconStream, PR_UINT32_MAX, aNewData);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsFaviconService::FinalizeStatements() {
  mozIStorageStatement* stmts[] = {
    mDBGetURL,
    mDBGetData,
    mDBGetIconInfo,
    mDBInsertIcon,
    mDBUpdateIcon,
    mDBSetPageFavicon
  };

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(stmts); i++) {
    nsresult rv = nsNavHistory::FinalizeStatement(stmts[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsFaviconService::GetFaviconDataAsync(nsIURI* aFaviconURI,
                                      mozIStorageStatementCallback *aCallback)
{
  NS_ASSERTION(aCallback, "Doesn't make sense to call this without a callback");
  nsresult rv = BindStatementURI(mDBGetData, 0, aFaviconURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStoragePendingStatement> pendingStatement;
  return mDBGetData->ExecuteAsync(aCallback, getter_AddRefs(pendingStatement));
}




NS_IMPL_ISUPPORTS4(FaviconLoadListener,
                   nsIRequestObserver,
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
               aRequest,
               reinterpret_cast<PRUint8*>(const_cast<char*>(mData.get())),
               mData.Length(), mimeType);
    
  }

  if (mimeType.IsEmpty()) {
    
    mFaviconService->AddFailedFavicon(mFaviconURI);
    return NS_OK;
  }

  
  
  PRTime expiration = -1;
  nsCOMPtr<nsICachingChannel> cachingChannel(do_QueryInterface(mChannel));
  if (cachingChannel) {
    nsCOMPtr<nsISupports> cacheToken;
    rv = cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsICacheEntryInfo> cacheEntry(do_QueryInterface(cacheToken));
      PRUint32 seconds;
      rv = cacheEntry->GetExpirationTime(&seconds);
      if (NS_SUCCEEDED(rv)) {
        
        expiration = PR_Now() + PR_MIN(seconds * PR_USEC_PER_SEC,
                                       MAX_FAVICON_EXPIRATION);
      }
    }
  }
  
  if (expiration < 0)
    expiration = PR_Now() + MAX_FAVICON_EXPIRATION;

  mozStorageTransaction transaction(mFaviconService->mDBConn, PR_FALSE);
  
  
  
  (void) mFaviconService->SetFaviconData(mFaviconURI,
               reinterpret_cast<PRUint8*>(const_cast<char*>(mData.get())),
               mData.Length(), mimeType, expiration);

  
  PRBool hasData;
  rv = mFaviconService->SetFaviconUrlForPageInternal(mPageURI, mFaviconURI,
                                                     &hasData);
  NS_ENSURE_SUCCESS(rv, rv);

  mFaviconService->UpdateBookmarkRedirectFavicon(mPageURI, mFaviconURI);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  mFaviconService->SendFaviconNotifications(mPageURI, mFaviconURI);
  return NS_OK;
}

NS_IMETHODIMP
FaviconLoadListener::OnDataAvailable(nsIRequest *aRequest,
                                     nsISupports *aContext,
                                     nsIInputStream *aInputStream,
                                     PRUint32 aOffset,
                                     PRUint32 aCount)
{
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
                                       nsIChannel* newChannel,
                                       PRUint32 flags)
{
  mChannel = newChannel;
  return NS_OK;
}




NS_IMPL_ISUPPORTS1(ExpireFaviconsStatementCallbackNotifier,
                   mozIStorageStatementCallback)

ExpireFaviconsStatementCallbackNotifier::ExpireFaviconsStatementCallbackNotifier(bool *aFaviconsExpirationRunning)
  : mFaviconsExpirationRunning(aFaviconsExpirationRunning)
{
  NS_ASSERTION(mFaviconsExpirationRunning, "Pointer to bool mFaviconsExpirationRunning can't be null");
}

NS_IMETHODIMP
ExpireFaviconsStatementCallbackNotifier::HandleCompletion(PRUint16 aReason)
{
  *mFaviconsExpirationRunning = false;

  
  if (aReason != mozIStorageStatementCallback::REASON_FINISHED)
    return NS_OK;

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");
  if (observerService) {
    (void)observerService->NotifyObservers(nsnull,
                                           NS_PLACES_FAVICONS_EXPIRED_TOPIC_ID,
                                           nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
ExpireFaviconsStatementCallbackNotifier::HandleResult(mozIStorageResultSet *aResultSet)
{
  NS_ASSERTION(PR_FALSE, "You cannot use this statement callback to get async statements resultset");
  return NS_OK;
}
