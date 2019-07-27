






#include "PackagedAppService.h"
#include "nsICacheStorage.h"
#include "LoadContextInfo.h"
#include "nsICacheStorageService.h"
#include "nsIResponseHeadProvider.h"
#include "nsIMultiPartChannel.h"
#include "../../cache2/CacheFileUtils.h"
#include "nsStreamUtils.h"

namespace mozilla {
namespace net {

static PackagedAppService *gPackagedAppService = nullptr;

NS_IMPL_ISUPPORTS(PackagedAppService, nsIPackagedAppService)

NS_IMPL_ISUPPORTS(PackagedAppService::CacheEntryWriter, nsIStreamListener)

 nsresult
PackagedAppService::CacheEntryWriter::Create(nsIURI *aURI,
                                             nsICacheStorage *aStorage,
                                             CacheEntryWriter **aResult)
{
  nsRefPtr<CacheEntryWriter> writer = new CacheEntryWriter();
  nsresult rv = aStorage->OpenTruncate(aURI, EmptyCString(),
                                       getter_AddRefs(writer->mEntry));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = writer->mEntry->ForceValidFor(PR_UINT32_MAX);
  if (NS_FAILED(rv)) {
    return rv;
  }

  writer.forget(aResult);
  return NS_OK;
}

nsresult
PackagedAppService::CacheEntryWriter::CopySecurityInfo(nsIChannel *aChannel)
{
  if (!aChannel) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsISupports> securityInfo;
  aChannel->GetSecurityInfo(getter_AddRefs(securityInfo));
  if (securityInfo) {
    mEntry->SetSecurityInfo(securityInfo);
  }

  return NS_OK;
}

NS_METHOD
PackagedAppService::CacheEntryWriter::ConsumeData(nsIInputStream *aStream,
                                                  void *aClosure,
                                                  const char *aFromRawSegment,
                                                  uint32_t aToOffset,
                                                  uint32_t aCount,
                                                  uint32_t *aWriteCount)
{
  MOZ_ASSERT(aClosure, "The closure must not be null");
  CacheEntryWriter *self = static_cast<CacheEntryWriter*>(aClosure);
  MOZ_ASSERT(self->mOutputStream, "The stream should not be null");
  return self->mOutputStream->Write(aFromRawSegment, aCount, aWriteCount);
}

NS_IMETHODIMP
PackagedAppService::CacheEntryWriter::OnStartRequest(nsIRequest *aRequest,
                                                     nsISupports *aContext)
{
  nsCOMPtr<nsIResponseHeadProvider> provider(do_QueryInterface(aRequest));
  if (!provider) {
    return NS_ERROR_INVALID_ARG;
  }
  nsHttpResponseHead *responseHead = provider->GetResponseHead();
  if (!responseHead) {
    return NS_ERROR_FAILURE;
  }

  mEntry->SetPredictedDataSize(responseHead->TotalEntitySize());

  nsAutoCString head;
  responseHead->Flatten(head, true);
  nsresult rv = mEntry->SetMetaDataElement("response-head", head.get());
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mEntry->SetMetaDataElement("request-method", "GET");
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIMultiPartChannel> multiPartChannel = do_QueryInterface(aRequest);
  if (!multiPartChannel) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIChannel> baseChannel;
  multiPartChannel->GetBaseChannel(getter_AddRefs(baseChannel));

  rv = CopySecurityInfo(baseChannel);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = mEntry->OpenOutputStream(0, getter_AddRefs(mOutputStream));
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
PackagedAppService::CacheEntryWriter::OnStopRequest(nsIRequest *aRequest,
                                                    nsISupports *aContext,
                                                    nsresult aStatusCode)
{
  if (mOutputStream) {
    mOutputStream->Close();
    mOutputStream = nullptr;
  }

  return NS_OK;
}

NS_IMETHODIMP
PackagedAppService::CacheEntryWriter::OnDataAvailable(nsIRequest *aRequest,
                                                      nsISupports *aContext,
                                                      nsIInputStream *aInputStream,
                                                      uint64_t aOffset,
                                                      uint32_t aCount)
{
  if (!aInputStream) {
    return NS_ERROR_INVALID_ARG;
  }
  
  uint32_t n;
  return aInputStream->ReadSegments(ConsumeData, this, aCount, &n);
}


NS_IMPL_ISUPPORTS(PackagedAppService::PackagedAppDownloader, nsIStreamListener)

nsresult
PackagedAppService::PackagedAppDownloader::Init(nsILoadContextInfo* aInfo,
                                                const nsCString& aKey)
{
  nsresult rv;
  nsCOMPtr<nsICacheStorageService> cacheStorageService =
    do_GetService("@mozilla.org/netwerk/cache-storage-service;1", &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = cacheStorageService->DiskCacheStorage(aInfo, false,
                                             getter_AddRefs(mCacheStorage));
  if (NS_FAILED(rv)) {
    return rv;
  }

  mPackageKey = aKey;
  return NS_OK;
}

NS_IMETHODIMP
PackagedAppService::PackagedAppDownloader::OnStartRequest(nsIRequest *aRequest,
                                                          nsISupports *aContext)
{
  
  
  mWriter = nullptr;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetSubresourceURI(aRequest, getter_AddRefs(uri));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_OK;
  }

  rv = CacheEntryWriter::Create(uri, mCacheStorage, getter_AddRefs(mWriter));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_OK;
  }

  MOZ_ASSERT(mWriter);
  rv = mWriter->OnStartRequest(aRequest, aContext);
  NS_WARN_IF(NS_FAILED(rv));
  return NS_OK;
}

nsresult
PackagedAppService::PackagedAppDownloader::GetSubresourceURI(nsIRequest * aRequest,
                                                             nsIURI ** aResult)
{
  nsresult rv;
  nsCOMPtr<nsIResponseHeadProvider> provider(do_QueryInterface(aRequest));
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));

  if (NS_WARN_IF(!provider || !chan)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsHttpResponseHead *responseHead = provider->GetResponseHead();
  if (NS_WARN_IF(!responseHead)) {
    return NS_ERROR_FAILURE;
  }
  nsAutoCString contentLocation;
  rv = responseHead->GetHeader(nsHttp::ResolveAtom("Content-Location"), contentLocation);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIURI> uri;
  rv = chan->GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsAutoCString path;
  rv = uri->GetPath(path);
  if (NS_FAILED(rv)) {
    return rv;
  }

  path += PACKAGED_APP_TOKEN;

  
  if (StringBeginsWith(contentLocation, NS_LITERAL_CSTRING("/"))) {
    contentLocation = Substring(contentLocation, 1);
  }

  path += contentLocation;

  nsCOMPtr<nsIURI> partURI;
  rv = uri->CloneIgnoringRef(getter_AddRefs(partURI));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = partURI->SetPath(path);
  if (NS_FAILED(rv)) {
    return rv;
  }

  partURI.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
PackagedAppService::PackagedAppDownloader::OnStopRequest(nsIRequest *aRequest,
                                                         nsISupports *aContext,
                                                         nsresult aStatusCode)
{
  nsCOMPtr<nsIMultiPartChannel> multiChannel(do_QueryInterface(aRequest));
  nsresult rv;


  
  
  
  if (multiChannel && mWriter) {
    mWriter->OnStopRequest(aRequest, aContext, aStatusCode);

    nsCOMPtr<nsIURI> uri;
    rv = GetSubresourceURI(aRequest, getter_AddRefs(uri));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return NS_OK;
    }

    nsCOMPtr<nsICacheEntry> entry;
    mWriter->mEntry.swap(entry);

    
    mWriter = nullptr;
    CallCallbacks(uri, entry, aStatusCode);
  }

  bool lastPart = false;
  if (multiChannel) {
    rv = multiChannel->GetIsLastPart(&lastPart);
    if (NS_SUCCEEDED(rv) && !lastPart) {
      
      return NS_OK;
    }
  }

  
  
  if (NS_SUCCEEDED(aStatusCode) && lastPart) {
    aStatusCode = NS_ERROR_FILE_NOT_FOUND;
  }

  nsRefPtr<PackagedAppDownloader> kungFuDeathGrip(this);
  
  if (gPackagedAppService) {
    gPackagedAppService->NotifyPackageDownloaded(mPackageKey);
  }
  ClearCallbacks(aStatusCode);
  return NS_OK;
}

NS_IMETHODIMP
PackagedAppService::PackagedAppDownloader::OnDataAvailable(nsIRequest *aRequest,
                                                           nsISupports *aContext,
                                                           nsIInputStream *aInputStream,
                                                           uint64_t aOffset,
                                                           uint32_t aCount)
{
  if (!mWriter) {
    uint32_t n;
    return aInputStream->ReadSegments(NS_DiscardSegment, nullptr, aCount, &n);
  }
  return mWriter->OnDataAvailable(aRequest, aContext, aInputStream, aOffset,
                                  aCount);
}

nsresult
PackagedAppService::PackagedAppDownloader::AddCallback(nsIURI *aURI,
                                                       nsICacheEntryOpenCallback *aCallback)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread(), "mCallbacks hashtable is not thread safe");
  nsAutoCString spec;
  aURI->GetAsciiSpec(spec);

  
  nsCOMArray<nsICacheEntryOpenCallback>* array = mCallbacks.Get(spec);
  if (array) {
    
    array->AppendObject(aCallback);
  } else {
    
    
    nsCOMArray<nsICacheEntryOpenCallback>* newArray =
      new nsCOMArray<nsICacheEntryOpenCallback>();
    newArray->AppendObject(aCallback);
    mCallbacks.Put(spec, newArray);
  }
  return NS_OK;
}

nsresult
PackagedAppService::PackagedAppDownloader::CallCallbacks(nsIURI *aURI,
                                                         nsICacheEntry *aEntry,
                                                         nsresult aResult)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread(), "mCallbacks hashtable is not thread safe");
  
  nsCOMPtr<nsICacheEntry> handle(aEntry);

  nsAutoCString spec;
  aURI->GetSpec(spec);

  nsCOMArray<nsICacheEntryOpenCallback>* array = mCallbacks.Get(spec);
  if (array) {
    
    for (uint32_t i = 0; i < array->Length(); ++i) {
      nsCOMPtr<nsICacheEntryOpenCallback> callback(array->ObjectAt(i));
      
      mCacheStorage->AsyncOpenURI(aURI, EmptyCString(),
                                  nsICacheStorage::OPEN_READONLY, callback);
    }
    
    array->Clear();
    mCallbacks.Remove(spec);
    aEntry->ForceValidFor(0);
  }
  return NS_OK;
}

PLDHashOperator
PackagedAppService::PackagedAppDownloader::ClearCallbacksEnumerator(const nsACString& key,
  nsAutoPtr<nsCOMArray<nsICacheEntryOpenCallback> >& callbackArray,
  void* arg)
{
  MOZ_ASSERT(arg, "The void* parameter should be a pointer to nsresult");
  nsresult *result = static_cast<nsresult*>(arg);
  for (uint32_t i = 0; i < callbackArray->Length(); ++i) {
    nsCOMPtr<nsICacheEntryOpenCallback> callback = callbackArray->ObjectAt(i);
    callback->OnCacheEntryAvailable(nullptr, false, nullptr, *result);
  }
  
  return PL_DHASH_REMOVE;
}

nsresult
PackagedAppService::PackagedAppDownloader::ClearCallbacks(nsresult aResult)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread(), "mCallbacks hashtable is not thread safe");
  mCallbacks.Enumerate(ClearCallbacksEnumerator, &aResult);
  return NS_OK;
}

NS_IMPL_ISUPPORTS(PackagedAppService::CacheEntryChecker, nsICacheEntryOpenCallback)

NS_IMETHODIMP
PackagedAppService::CacheEntryChecker::OnCacheEntryCheck(nsICacheEntry *aEntry,
                                                         nsIApplicationCache *aApplicationCache,
                                                         uint32_t *_retval)
{
  return mCallback->OnCacheEntryCheck(aEntry, aApplicationCache, _retval);
}

NS_IMETHODIMP
PackagedAppService::CacheEntryChecker::OnCacheEntryAvailable(nsICacheEntry *aEntry,
                                                             bool aNew,
                                                             nsIApplicationCache *aApplicationCache,
                                                             nsresult aResult)
{
  if (aResult == NS_ERROR_CACHE_KEY_NOT_FOUND) {
    MOZ_ASSERT(!aEntry, "No entry");
    
    
    gPackagedAppService->OpenNewPackageInternal(mURI, mCallback,
                                                mLoadContextInfo);
  } else {
    
    
    mCallback->OnCacheEntryAvailable(aEntry, aNew, aApplicationCache, aResult);
    
  }
  return NS_OK;
}

PackagedAppService::PackagedAppService()
{
  gPackagedAppService = this;
}

PackagedAppService::~PackagedAppService()
{
  gPackagedAppService = nullptr;
}

NS_IMETHODIMP
PackagedAppService::RequestURI(nsIURI *aURI,
                               nsILoadContextInfo *aInfo,
                               nsICacheEntryOpenCallback *aCallback)
{
  
  if (!aURI || !aCallback || !aInfo) {
    return NS_ERROR_INVALID_ARG;
  }

  nsAutoCString path;
  aURI->GetPath(path);
  int32_t pos = path.Find(PACKAGED_APP_TOKEN);
  if (pos == kNotFound) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;
  nsCOMPtr<nsICacheStorageService> cacheStorageService =
    do_GetService("@mozilla.org/netwerk/cache-storage-service;1", &rv);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsICacheStorage> cacheStorage;

  rv = cacheStorageService->DiskCacheStorage(aInfo, false,
                                             getter_AddRefs(cacheStorage));

  nsRefPtr<CacheEntryChecker> checker = new CacheEntryChecker(aURI, aCallback, aInfo);
  return cacheStorage->AsyncOpenURI(aURI, EmptyCString(),
                                    nsICacheStorage::OPEN_READONLY, checker);
}

nsresult
PackagedAppService::NotifyPackageDownloaded(nsCString aKey)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread(), "mDownloadingPackages hashtable is not thread safe");
  mDownloadingPackages.Remove(aKey);
  return NS_OK;
}

nsresult
PackagedAppService::OpenNewPackageInternal(nsIURI *aURI,
                                           nsICacheEntryOpenCallback *aCallback,
                                           nsILoadContextInfo *aInfo)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread(), "mDownloadingPackages hashtable is not thread safe");

  nsAutoCString path;
  nsresult rv = aURI->GetPath(path);
  if (NS_FAILED(rv)) {
    return rv;
  }

  int32_t pos = path.Find(PACKAGED_APP_TOKEN);
  MOZ_ASSERT(pos != kNotFound,
             "This should never be called if the token is missing");

  nsCOMPtr<nsIURI> packageURI;
  rv = aURI->CloneIgnoringRef(getter_AddRefs(packageURI));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = packageURI->SetPath(Substring(path, 0, pos));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsAutoCString key;
  CacheFileUtils::AppendKeyPrefix(aInfo, key);

  {
    nsAutoCString spec;
    packageURI->GetAsciiSpec(spec);
    key += ":";
    key += spec;
  }

  nsRefPtr<PackagedAppDownloader> downloader;
  if (mDownloadingPackages.Get(key, getter_AddRefs(downloader))) {
    
    
    
    

    downloader->AddCallback(aURI, aCallback);
    return NS_OK;
  }

  
  
  uint32_t extra_flags = 0;
  if (aInfo->IsAnonymous()) {
    extra_flags = nsIRequest::LOAD_ANONYMOUS;
  }

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(
    getter_AddRefs(channel), packageURI, nsContentUtils::GetSystemPrincipal(),
    nsILoadInfo::SEC_NORMAL, nsIContentPolicy::TYPE_OTHER, nullptr, nullptr,
    nsIRequest::LOAD_NORMAL | extra_flags);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsICachingChannel> cacheChan(do_QueryInterface(channel));
  if (cacheChan) {
    
    
    
    cacheChan->SetCacheOnlyMetadata(true);
  }

  downloader = new PackagedAppDownloader();
  rv = downloader->Init(aInfo, key);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  downloader->AddCallback(aURI, aCallback);

  nsCOMPtr<nsIStreamConverterService> streamconv =
    do_GetService("@mozilla.org/streamConverters;1", &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIStreamListener> mimeConverter;
  rv = streamconv->AsyncConvertData("multipart/mixed", "*/*", downloader, nullptr,
                                    getter_AddRefs(mimeConverter));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  mDownloadingPackages.Put(key, downloader);

  return channel->AsyncOpen(mimeConverter, nullptr);
}

} 
} 
