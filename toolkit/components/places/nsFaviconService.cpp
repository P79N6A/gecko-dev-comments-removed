


















































#include "nsFaviconService.h"

#include "nsNavHistory.h"
#include "nsPlacesMacros.h"
#include "Helpers.h"
#include "AsyncFaviconHelpers.h"

#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsStreamUtils.h"
#include "nsStringStream.h"
#include "plbase64.h"
#include "nsIClassInfoImpl.h"
#include "mozilla/Preferences.h"
#include "mozilla/Util.h"


#include "imgITools.h"
#include "imgIContainer.h"


#define OPTIMIZED_FAVICON_DIMENSION 16

#define MAX_FAVICON_CACHE_SIZE 256
#define FAVICON_CACHE_REDUCE_COUNT 64







#define MAX_FAVICON_EXPIRATION ((PRTime)7 * 24 * 60 * 60 * PR_USEC_PER_SEC)



#define DEFAULT_MIME_TYPE "image/png"

using namespace mozilla;
using namespace mozilla::places;





class ExpireFaviconsStatementCallbackNotifier : public AsyncStatementCallback
{
public:
  ExpireFaviconsStatementCallbackNotifier(bool* aFaviconsExpirationRunning);
  NS_IMETHOD HandleCompletion(PRUint16 aReason);

private:
  bool* mFaviconsExpirationRunning;
};


PLACES_FACTORY_SINGLETON_IMPLEMENTATION(nsFaviconService, gFaviconService)

NS_IMPL_CLASSINFO(nsFaviconService, NULL, 0, NS_FAVICONSERVICE_CID)
NS_IMPL_ISUPPORTS2_CI(
  nsFaviconService
, nsIFaviconService
, mozIAsyncFavicons
)

nsFaviconService::nsFaviconService()
  : mFaviconsExpirationRunning(false)
  , mOptimizedIconDimension(OPTIMIZED_FAVICON_DIMENSION)
  , mFailedFaviconSerial(0)
{
  NS_ASSERTION(!gFaviconService,
               "Attempting to create two instances of the service!");
  gFaviconService = this;
}


nsFaviconService::~nsFaviconService()
{
  NS_ASSERTION(gFaviconService == this,
               "Deleting a non-singleton instance of the service");
  if (gFaviconService == this)
    gFaviconService = nsnull;
}


nsresult
nsFaviconService::Init()
{
  mDB = Database::GetDatabase();
  NS_ENSURE_STATE(mDB);

  
  if (!mFailedFavicons.Init(MAX_FAVICON_CACHE_SIZE))
    return NS_ERROR_OUT_OF_MEMORY;

  mOptimizedIconDimension = Preferences::GetInt(
    "places.favicons.optimizeToDimension", OPTIMIZED_FAVICON_DIMENSION
  );

  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::ExpireAllFavicons()
{
  mFaviconsExpirationRunning = true;
  nsCOMPtr<mozIStorageAsyncStatement> unlinkIconsStmt = mDB->GetAsyncStatement(
    "UPDATE moz_places "
    "SET favicon_id = NULL "
    "WHERE favicon_id NOT NULL"
  );
  NS_ENSURE_STATE(unlinkIconsStmt);
  nsCOMPtr<mozIStorageAsyncStatement> removeIconsStmt = mDB->GetAsyncStatement(
    "DELETE FROM moz_favicons WHERE id NOT IN ("
      "SELECT favicon_id FROM moz_places WHERE favicon_id NOT NULL "
    ")"
  );
  NS_ENSURE_STATE(removeIconsStmt);
  
  mozIStorageBaseStatement* stmts[] = {
    unlinkIconsStmt.get()
  , removeIconsStmt.get()
  };
  nsCOMPtr<mozIStoragePendingStatement> ps;
  nsRefPtr<ExpireFaviconsStatementCallbackNotifier> callback =
    new ExpireFaviconsStatementCallbackNotifier(&mFaviconsExpirationRunning);
  nsresult rv = mDB->MainConn()->ExecuteAsync(
    stmts, ArrayLength(stmts), callback, getter_AddRefs(ps)
  );
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





NS_IMETHODIMP
nsFaviconService::SetFaviconUrlForPage(nsIURI* aPageURI, nsIURI* aFaviconURI)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG(aFaviconURI);

  
  if (mFaviconsExpirationRunning) {
    return NS_OK;
  }

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);

  if (history->InPrivateBrowsingMode()) {
    return NS_OK;
  }

  nsresult rv;
  PRInt64 iconId = -1;
  bool hasData = false;
  {
    nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
      "SELECT id, length(data), expiration FROM moz_favicons "
      "WHERE url = :icon_url"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);

    rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"), aFaviconURI);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasResult = false;
    if (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
      
      rv = stmt->GetInt64(0, &iconId);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt32 dataSize;
      rv = stmt->GetInt32(1, &dataSize);
      NS_ENSURE_SUCCESS(rv, rv);
      if (dataSize > 0) {
        hasData = true;
      }
    }
  }

  mozStorageTransaction transaction(mDB->MainConn(), false);

  if (iconId == -1) {
    
    nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
      "INSERT INTO moz_favicons (id, url, data, mime_type, expiration, guid) "
      "VALUES (:icon_id, :icon_url, :data, :mime_type, :expiration, "
              "COALESCE(:guid, GENERATE_GUID()))"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);

    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("guid"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("icon_id"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"), aFaviconURI);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("data"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("mime_type"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("expiration"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    {
      nsCOMPtr<mozIStorageStatement> getInfoStmt = mDB->GetStatement(
        "SELECT id, length(data), expiration FROM moz_favicons "
        "WHERE url = :icon_url"
      );
      NS_ENSURE_STATE(getInfoStmt);
      mozStorageStatementScoper scoper(getInfoStmt);

      rv = URIBinder::Bind(getInfoStmt, NS_LITERAL_CSTRING("icon_url"), aFaviconURI);
      NS_ENSURE_SUCCESS(rv, rv);
      bool hasResult;
      rv = getInfoStmt->ExecuteStep(&hasResult);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ASSERTION(hasResult, "hasResult is false but the call succeeded?");
      iconId = getInfoStmt->AsInt64(0);
    }
  }

  
  PRInt64 pageId;
  nsCAutoString guid;
  rv = history->GetOrCreateIdForPage(aPageURI, &pageId, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "UPDATE moz_places SET favicon_id = :icon_id WHERE id = :page_id"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), pageId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("icon_id"), iconId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (hasData) {
    SendFaviconNotifications(aPageURI, aFaviconURI, guid);
  }

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

void
nsFaviconService::SendFaviconNotifications(nsIURI* aPageURI,
                                           nsIURI* aFaviconURI,
                                           const nsACString& aGUID)
{
  nsCAutoString faviconSpec;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  if (history && NS_SUCCEEDED(aFaviconURI->GetSpec(faviconSpec))) {
    history->SendPageChangedNotification(aPageURI,
                                         nsINavHistoryObserver::ATTRIBUTE_FAVICON,
                                         NS_ConvertUTF8toUTF16(faviconSpec),
                                         aGUID);
  }
}


NS_IMETHODIMP
nsFaviconService::SetAndLoadFaviconForPage(nsIURI* aPageURI,
                                           nsIURI* aFaviconURI,
                                           bool aForceReload,
                                           nsIFaviconDataCallback* aCallback)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG(aFaviconURI);

  if (mFaviconsExpirationRunning)
    return NS_OK;

  
  bool previouslyFailed;
  nsresult rv = IsFailedFavicon(aFaviconURI, &previouslyFailed);
  NS_ENSURE_SUCCESS(rv, rv);
  if (previouslyFailed) {
    if (aForceReload)
      RemoveFailedFavicon(aFaviconURI);
    else
      return NS_OK;
  }

  
  
  rv = AsyncFetchAndSetIconForPage::start(
    aFaviconURI, aPageURI, aForceReload ? FETCH_ALWAYS : FETCH_IF_MISSING,
    aCallback
  );
  NS_ENSURE_SUCCESS(rv, rv);

  
  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::SetAndFetchFaviconForPage(nsIURI* aPageURI,
                                            nsIURI* aFaviconURI,
                                            bool aForceReload,
                                            nsIFaviconDataCallback* aCallback)
{
  return SetAndLoadFaviconForPage(aPageURI, aFaviconURI,
                                  aForceReload, aCallback);
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

  nsCOMPtr<mozIStorageStatement> statement;
  {
    
    
    nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
      "SELECT id, length(data), expiration FROM moz_favicons "
      "WHERE url = :icon_url"
    );
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);

    rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"), aFaviconURI);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasResult;
    rv = stmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);

    if (hasResult) {
      
      PRInt64 id;
      rv = stmt->GetInt64(0, &id);
      NS_ENSURE_SUCCESS(rv, rv);
      statement = mDB->GetStatement(
        "UPDATE moz_favicons SET "
               "guid       = COALESCE(:guid, guid), "
               "data       = :data, "
               "mime_type  = :mime_type, "
               "expiration = :expiration "
        "WHERE id = :icon_id"
      );
      NS_ENSURE_STATE(statement);

      rv = statement->BindNullByName(NS_LITERAL_CSTRING("guid"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("icon_id"), id);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->BindBlobByName(NS_LITERAL_CSTRING("data"), data, dataLen);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->BindUTF8StringByName(NS_LITERAL_CSTRING("mime_type"), *mimeType);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("expiration"), aExpiration);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      
      statement = mDB->GetStatement(
       "INSERT INTO moz_favicons (id, url, data, mime_type, expiration, guid) "
       "VALUES (:icon_id, :icon_url, :data, :mime_type, :expiration, "
               "COALESCE(:guid, GENERATE_GUID()))");
      NS_ENSURE_STATE(statement);

      rv = statement->BindNullByName(NS_LITERAL_CSTRING("icon_id"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = URIBinder::Bind(statement, NS_LITERAL_CSTRING("icon_url"), aFaviconURI);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->BindBlobByName(NS_LITERAL_CSTRING("data"), data, dataLen);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->BindUTF8StringByName(NS_LITERAL_CSTRING("mime_type"), *mimeType);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("expiration"), aExpiration);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->BindNullByName(NS_LITERAL_CSTRING("guid"));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  mozStorageStatementScoper statementScoper(statement);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
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

  nsCOMPtr<nsIURI> defaultFaviconURI;
  nsresult rv = GetDefaultFavicon(getter_AddRefs(defaultFaviconURI));
  NS_ENSURE_SUCCESS(rv, rv);

  bool isDefaultFavicon = false;
  rv = defaultFaviconURI->Equals(aFaviconURI, &isDefaultFavicon);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (isDefaultFavicon) {
    nsCAutoString defaultData;
    rv = GetDefaultFaviconData(defaultData);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint8* bytes = reinterpret_cast<PRUint8*>(ToNewCString(defaultData));
    NS_ENSURE_STATE(bytes);

    *aData = bytes;
    *aDataLen = defaultData.Length();
    aMimeType.AssignLiteral(DEFAULT_MIME_TYPE);

    return NS_OK;
  }

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT f.data, f.mime_type FROM moz_favicons f WHERE url = :icon_url"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"), aFaviconURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasResult = false;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    rv = stmt->GetUTF8String(1, aMimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    return stmt->GetBlob(0, aDataLen, aData);
  }
  return NS_ERROR_NOT_AVAILABLE;
}


nsresult
nsFaviconService::GetDefaultFaviconData(nsCString& byteStr)
{
  if (mDefaultFaviconData.IsEmpty()) {
    nsCOMPtr<nsIURI> defaultFaviconURI;
    nsresult rv = GetDefaultFavicon(getter_AddRefs(defaultFaviconURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIInputStream> istream;
    rv = NS_OpenURI(getter_AddRefs(istream), defaultFaviconURI);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_ConsumeStream(istream, PR_UINT32_MAX, mDefaultFaviconData);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = istream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    if (mDefaultFaviconData.IsEmpty())
      return NS_ERROR_UNEXPECTED;
  }

  byteStr.Assign(mDefaultFaviconData);
  return NS_OK;
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
    aDataURL.SetIsVoid(true);
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

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT f.id, f.url, length(f.data), f.expiration "
    "FROM moz_places h "
    "JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE h.url = :page_url "
    "LIMIT 1"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aPageURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasResult;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    nsCAutoString url;
    rv = stmt->GetUTF8String(1, url);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_NewURI(_retval, url);
  }
  return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsFaviconService::GetFaviconURLForPage(nsIURI *aPageURI,
                                       nsIFaviconDataCallback* aCallback)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG(aCallback);

  nsresult rv = AsyncGetFaviconURLForPage::start(aPageURI, aCallback);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::GetFaviconDataForPage(nsIURI* aPageURI,
                                        nsIFaviconDataCallback* aCallback)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG(aCallback);

  nsresult rv = AsyncGetFaviconDataForPage::start(aPageURI, aCallback);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::GetFaviconImageForPage(nsIURI* aPageURI, nsIURI** _retval)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT f.id, f.url, length(f.data), f.expiration "
    "FROM moz_places h "
    "JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE h.url = :page_url "
    "LIMIT 1"
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aPageURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasResult;
  nsCOMPtr<nsIURI> faviconURI;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    PRInt32 dataLen;
    rv = stmt->GetInt32(2, &dataLen);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dataLen > 0) {
      
      nsCAutoString favIconUri;
      rv = stmt->GetUTF8String(1, favIconUri);
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
nsFaviconService::IsFailedFavicon(nsIURI* aFaviconURI, bool* _retval)
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

  aNewMimeType.AssignLiteral(DEFAULT_MIME_TYPE);

  
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
nsFaviconService::GetFaviconDataAsync(nsIURI* aFaviconURI,
                                      mozIStorageStatementCallback *aCallback)
{
  NS_ASSERTION(aCallback, "Doesn't make sense to call this without a callback");
  nsCOMPtr<mozIStorageAsyncStatement> stmt = mDB->GetAsyncStatement(
    "SELECT f.data, f.mime_type FROM moz_favicons f WHERE url = :icon_url"
  );
  NS_ENSURE_STATE(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"), aFaviconURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStoragePendingStatement> pendingStatement;
  return stmt->ExecuteAsync(aCallback, getter_AddRefs(pendingStatement));
}




ExpireFaviconsStatementCallbackNotifier::ExpireFaviconsStatementCallbackNotifier(
  bool* aFaviconsExpirationRunning)
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
    mozilla::services::GetObserverService();
  if (observerService) {
    (void)observerService->NotifyObservers(nsnull,
                                           NS_PLACES_FAVICONS_EXPIRED_TOPIC_ID,
                                           nsnull);
  }

  return NS_OK;
}
