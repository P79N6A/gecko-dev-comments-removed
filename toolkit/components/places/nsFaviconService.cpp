













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
#include "mozilla/ArrayUtils.h"
#include "mozilla/Preferences.h"


#include "imgITools.h"
#include "imgIContainer.h"


#define OPTIMIZED_FAVICON_DIMENSION 16

#define MAX_FAVICON_CACHE_SIZE 256
#define FAVICON_CACHE_REDUCE_COUNT 64

#define MAX_UNASSOCIATED_FAVICONS 64




#define UNASSOCIATED_ICON_EXPIRY_INTERVAL 60000



#define DEFAULT_MIME_TYPE "image/png"

using namespace mozilla;
using namespace mozilla::places;





class ExpireFaviconsStatementCallbackNotifier : public AsyncStatementCallback
{
public:
  ExpireFaviconsStatementCallbackNotifier();
  NS_IMETHOD HandleCompletion(uint16_t aReason);
};


PLACES_FACTORY_SINGLETON_IMPLEMENTATION(nsFaviconService, gFaviconService)

NS_IMPL_CLASSINFO(nsFaviconService, nullptr, 0, NS_FAVICONSERVICE_CID)
NS_IMPL_ISUPPORTS_CI(
  nsFaviconService
, nsIFaviconService
, mozIAsyncFavicons
, nsITimerCallback
)

nsFaviconService::nsFaviconService()
  : mOptimizedIconDimension(OPTIMIZED_FAVICON_DIMENSION)
  , mFailedFaviconSerial(0)
  , mFailedFavicons(MAX_FAVICON_CACHE_SIZE)
  , mUnassociatedIcons(MAX_UNASSOCIATED_FAVICONS)
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
    gFaviconService = nullptr;
}


nsresult
nsFaviconService::Init()
{
  mDB = Database::GetDatabase();
  NS_ENSURE_STATE(mDB);

  mOptimizedIconDimension = Preferences::GetInt(
    "places.favicons.optimizeToDimension", OPTIMIZED_FAVICON_DIMENSION
  );

  mExpireUnassociatedIconsTimer = do_CreateInstance("@mozilla.org/timer;1");
  NS_ENSURE_STATE(mExpireUnassociatedIconsTimer);

  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::ExpireAllFavicons()
{
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
    new ExpireFaviconsStatementCallbackNotifier();
  nsresult rv = mDB->MainConn()->ExecuteAsync(
    stmts, ArrayLength(stmts), callback, getter_AddRefs(ps)
  );
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




static PLDHashOperator
ExpireNonrecentUnassociatedIconsEnumerator(
  UnassociatedIconHashKey* aIconKey,
  void* aNow)
{
  PRTime now = *(reinterpret_cast<PRTime*>(aNow));
  if (now - aIconKey->created >= UNASSOCIATED_ICON_EXPIRY_INTERVAL) {
    return PL_DHASH_REMOVE;
  }
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsFaviconService::Notify(nsITimer* timer)
{
  if (timer != mExpireUnassociatedIconsTimer.get()) {
    return NS_ERROR_INVALID_ARG;
  }

  PRTime now = PR_Now();
  mUnassociatedIcons.EnumerateEntries(
    ExpireNonrecentUnassociatedIconsEnumerator, &now);
  
  if (mUnassociatedIcons.Count() > 0) {
    mExpireUnassociatedIconsTimer->InitWithCallback(
      this, UNASSOCIATED_ICON_EXPIRY_INTERVAL, nsITimer::TYPE_ONE_SHOT);
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
  nsAutoCString faviconSpec;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  if (history && NS_SUCCEEDED(aFaviconURI->GetSpec(faviconSpec))) {
    history->SendPageChangedNotification(aPageURI,
                                         nsINavHistoryObserver::ATTRIBUTE_FAVICON,
                                         NS_ConvertUTF8toUTF16(faviconSpec),
                                         aGUID);
  }
}

NS_IMETHODIMP
nsFaviconService::SetAndFetchFaviconForPage(nsIURI* aPageURI,
                                            nsIURI* aFaviconURI,
                                            bool aForceReload,
                                            uint32_t aFaviconLoadType,
                                            nsIFaviconDataCallback* aCallback)
{
  NS_ENSURE_ARG(aPageURI);
  NS_ENSURE_ARG(aFaviconURI);

  
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
    aFaviconLoadType, aCallback
  );
  NS_ENSURE_SUCCESS(rv, rv);

  
  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::ReplaceFaviconData(nsIURI* aFaviconURI,
                                    const uint8_t* aData,
                                    uint32_t aDataLen,
                                    const nsACString& aMimeType,
                                    PRTime aExpiration)
{
  NS_ENSURE_ARG(aFaviconURI);
  NS_ENSURE_ARG(aData);
  NS_ENSURE_TRUE(aDataLen > 0, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(aMimeType.Length() > 0, NS_ERROR_INVALID_ARG);
  if (aExpiration == 0) {
    aExpiration = PR_Now() + MAX_FAVICON_EXPIRATION;
  }

  UnassociatedIconHashKey* iconKey = mUnassociatedIcons.PutEntry(aFaviconURI);
  if (!iconKey) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  iconKey->created = PR_Now();

  
  
  int32_t unassociatedCount = mUnassociatedIcons.Count();
  if (unassociatedCount == 1) {
    mExpireUnassociatedIconsTimer->Cancel();
    mExpireUnassociatedIconsTimer->InitWithCallback(
      this, UNASSOCIATED_ICON_EXPIRY_INTERVAL, nsITimer::TYPE_ONE_SHOT);
  }

  IconData* iconData = &(iconKey->iconData);
  iconData->expiration = aExpiration;
  iconData->status = ICON_STATUS_CACHED;
  iconData->fetchMode = FETCH_NEVER;
  nsresult rv = aFaviconURI->GetSpec(iconData->spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if (aDataLen > MAX_ICON_FILESIZE(mOptimizedIconDimension)) {
    rv = OptimizeFaviconImage(aData, aDataLen, aMimeType, iconData->data, iconData->mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (iconData->data.Length() > MAX_FAVICON_SIZE) {
      
      
      mUnassociatedIcons.RemoveEntry(aFaviconURI);
      return NS_ERROR_FAILURE;
    }
  } else {
    iconData->mimeType.Assign(aMimeType);
    iconData->data.Assign(TO_CHARBUFFER(aData), aDataLen);
  }

  
  
  
  rv = AsyncReplaceFaviconData::start(iconData);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsFaviconService::ReplaceFaviconDataFromDataURL(nsIURI* aFaviconURI,
                                               const nsAString& aDataURL,
                                               PRTime aExpiration)
{
  NS_ENSURE_ARG(aFaviconURI);
  NS_ENSURE_TRUE(aDataURL.Length() > 0, NS_ERROR_INVALID_ARG);
  if (aExpiration == 0) {
    aExpiration = PR_Now() + MAX_FAVICON_EXPIRATION;
  }

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

  uint64_t available64;
  rv = stream->Available(&available64);
  NS_ENSURE_SUCCESS(rv, rv);
  if (available64 == 0 || available64 > UINT32_MAX / sizeof(uint8_t))
    return NS_ERROR_FILE_TOO_BIG;
  uint32_t available = (uint32_t)available64;

  
  uint8_t* buffer = static_cast<uint8_t*>
                               (nsMemory::Alloc(sizeof(uint8_t) * available));
  if (!buffer)
    return NS_ERROR_OUT_OF_MEMORY;
  uint32_t numRead;
  rv = stream->Read(TO_CHARBUFFER(buffer), available, &numRead);
  if (NS_FAILED(rv) || numRead != available) {
    nsMemory::Free(buffer);
    return rv;
  }

  nsAutoCString mimeType;
  rv = channel->GetContentType(mimeType);
  if (NS_FAILED(rv)) {
    nsMemory::Free(buffer);
    return rv;
  }

  
  rv = ReplaceFaviconData(aFaviconURI, buffer, available, mimeType, aExpiration);
  nsMemory::Free(buffer);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
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

nsresult
nsFaviconService::GetFaviconLinkForIcon(nsIURI* aFaviconURI,
                                        nsIURI** aOutputURI)
{
  NS_ENSURE_ARG(aFaviconURI);
  NS_ENSURE_ARG_POINTER(aOutputURI);

  nsAutoCString spec;
  if (aFaviconURI) {
    nsresult rv = aFaviconURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return GetFaviconLinkForIconString(spec, aOutputURI);
}


static PLDHashOperator
ExpireFailedFaviconsCallback(nsCStringHashKey::KeyType aKey,
                             uint32_t& aData,
                             void* userArg)
{
  uint32_t* threshold = reinterpret_cast<uint32_t*>(userArg);
  if (aData < *threshold)
    return PL_DHASH_REMOVE;
  return PL_DHASH_NEXT;
}


NS_IMETHODIMP
nsFaviconService::AddFailedFavicon(nsIURI* aFaviconURI)
{
  NS_ENSURE_ARG(aFaviconURI);

  nsAutoCString spec;
  nsresult rv = aFaviconURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  mFailedFavicons.Put(spec, mFailedFaviconSerial);
  mFailedFaviconSerial ++;

  if (mFailedFavicons.Count() > MAX_FAVICON_CACHE_SIZE) {
    
    
    uint32_t threshold = mFailedFaviconSerial -
                         MAX_FAVICON_CACHE_SIZE + FAVICON_CACHE_REDUCE_COUNT;
    mFailedFavicons.Enumerate(ExpireFailedFaviconsCallback, &threshold);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsFaviconService::RemoveFailedFavicon(nsIURI* aFaviconURI)
{
  NS_ENSURE_ARG(aFaviconURI);

  nsAutoCString spec;
  nsresult rv = aFaviconURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mFailedFavicons.Remove(spec);
  return NS_OK;
}


NS_IMETHODIMP
nsFaviconService::IsFailedFavicon(nsIURI* aFaviconURI, bool* _retval)
{
  NS_ENSURE_ARG(aFaviconURI);
  nsAutoCString spec;
  nsresult rv = aFaviconURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t serial;
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

  nsAutoCString annoUri;
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
nsFaviconService::OptimizeFaviconImage(const uint8_t* aData, uint32_t aDataLen,
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
                                  EmptyString(),
                                  getter_AddRefs(iconStream));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = NS_ConsumeStream(iconStream, UINT32_MAX, aNewData);
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

  
  

  nsAutoCString faviconURI;
  aFaviconURI->GetSpecIgnoringRef(faviconURI);
  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("icon_url"), faviconURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStoragePendingStatement> pendingStatement;
  return stmt->ExecuteAsync(aCallback, getter_AddRefs(pendingStatement));
}




ExpireFaviconsStatementCallbackNotifier::ExpireFaviconsStatementCallbackNotifier()
{
}


NS_IMETHODIMP
ExpireFaviconsStatementCallbackNotifier::HandleCompletion(uint16_t aReason)
{
  
  if (aReason != mozIStorageStatementCallback::REASON_FINISHED)
    return NS_OK;

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    (void)observerService->NotifyObservers(nullptr,
                                           NS_PLACES_FAVICONS_EXPIRED_TOPIC_ID,
                                           nullptr);
  }

  return NS_OK;
}
