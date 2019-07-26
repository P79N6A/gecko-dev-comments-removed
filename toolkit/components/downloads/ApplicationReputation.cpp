





#include "ApplicationReputation.h"
#include "csd.pb.h"

#include "nsIApplicationReputation.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIIOService.h"
#include "nsIPrefService.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStreamListener.h"
#include "nsIStringStream.h"
#include "nsIUploadChannel2.h"
#include "nsIURI.h"
#include "nsIUrlClassifierDBService.h"

#include "mozilla/Preferences.h"
#include "mozilla/Services.h"

#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsNetCID.h"
#include "nsReadableUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsXPCOMStrings.h"

using mozilla::Preferences;




#define PREF_SB_APP_REP_URL "browser.safebrowsing.appRepURL"
#define PREF_SB_MALWARE_ENABLED "browser.safebrowsing.malware.enabled"
#define PREF_GENERAL_LOCALE "general.useragent.locale"
#define PREF_DOWNLOAD_BLOCK_TABLE "urlclassifier.download_block_table"
#define PREF_DOWNLOAD_ALLOW_TABLE "urlclassifier.download_allow_table"






class PendingLookup MOZ_FINAL :
  public nsIStreamListener,
  public nsIUrlClassifierCallback {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIURLCLASSIFIERCALLBACK
  PendingLookup(nsIApplicationReputationQuery* aQuery,
                nsIApplicationReputationCallback* aCallback);
  ~PendingLookup();

private:
  nsCOMPtr<nsIApplicationReputationQuery> mQuery;
  nsCOMPtr<nsIApplicationReputationCallback> mCallback;
  




  nsCString mResponse;
  



  nsresult OnComplete(bool shouldBlock, nsresult rv);
  



  nsresult OnStopRequestInternal(nsIRequest *aRequest,
                                 nsISupports *aContext,
                                 nsresult aResult,
                                 bool* aShouldBlock);
  



  nsresult SendRemoteQuery();
};

NS_IMPL_ISUPPORTS3(PendingLookup,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIUrlClassifierCallback)

PendingLookup::PendingLookup(nsIApplicationReputationQuery* aQuery,
                             nsIApplicationReputationCallback* aCallback) :
  mQuery(aQuery),
  mCallback(aCallback) {
}

PendingLookup::~PendingLookup() {
}

nsresult
PendingLookup::OnComplete(bool shouldBlock, nsresult rv) {
  nsresult res = mCallback->OnComplete(shouldBlock, rv);
  return res;
}



NS_IMETHODIMP
PendingLookup::HandleEvent(const nsACString& tables) {
  
  
  
  nsCString allow_list;
  Preferences::GetCString(PREF_DOWNLOAD_ALLOW_TABLE, &allow_list);
  if (FindInReadable(tables, allow_list)) {
    return OnComplete(false, NS_OK);
  }

  nsCString block_list;
  Preferences::GetCString(PREF_DOWNLOAD_BLOCK_TABLE, &block_list);
  if (FindInReadable(tables, block_list)) {
    return OnComplete(true, NS_OK);
  }

  nsresult rv = SendRemoteQuery();
  if (NS_FAILED(rv)) {
    return OnComplete(false, rv);
  }
  return NS_OK;
}

nsresult
PendingLookup::SendRemoteQuery() {
  
  
  safe_browsing::ClientDownloadRequest req;
  nsCOMPtr<nsIURI> uri;
  nsresult rv;
  rv = mQuery->GetSourceURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString spec;
  rv = uri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  req.set_url(spec.get());

  uint32_t fileSize;
  rv = mQuery->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);
  req.set_length(fileSize);
  
  req.set_user_initiated(false);

  nsCString locale;
  NS_ENSURE_SUCCESS(Preferences::GetCString(PREF_GENERAL_LOCALE, &locale),
                    NS_ERROR_NOT_AVAILABLE);
  req.set_locale(locale.get());
  nsCString sha256Hash;
  rv = mQuery->GetSha256Hash(sha256Hash);
  NS_ENSURE_SUCCESS(rv, rv);
  req.mutable_digests()->set_sha256(sha256Hash.Data());
  nsString fileName;
  rv = mQuery->GetSuggestedFileName(fileName);
  NS_ENSURE_SUCCESS(rv, rv);
  req.set_file_basename(NS_ConvertUTF16toUTF8(fileName).get());

  
  
  
  std::string serialized;
  if (!req.SerializeToString(&serialized)) {
    return NS_ERROR_UNEXPECTED;
  }

  
  nsCOMPtr<nsIStringInputStream> sstream =
    do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sstream->SetData(serialized.c_str(), serialized.length());
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIIOService> ios = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIChannel> channel;
  nsCString serviceUrl;
  NS_ENSURE_SUCCESS(Preferences::GetCString(PREF_SB_APP_REP_URL, &serviceUrl),
                    NS_ERROR_NOT_AVAILABLE);
  rv = ios->NewChannel(serviceUrl, nullptr, nullptr, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  const nsCString userAgent = NS_LITERAL_CSTRING("CsdTesting/Mozilla");
  rv = httpChannel->SetRequestHeader(
    NS_LITERAL_CSTRING("User-Agent"), userAgent, false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIUploadChannel2> uploadChannel = do_QueryInterface(channel, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = uploadChannel->ExplicitSetUploadStream(sstream,
    NS_LITERAL_CSTRING("application/octet-stream"), serialized.size(),
    NS_LITERAL_CSTRING("POST"), false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = channel->AsyncOpen(this, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}



static NS_METHOD
AppendSegmentToString(nsIInputStream* inputStream,
                      void *closure,
                      const char *rawSegment,
                      uint32_t toOffset,
                      uint32_t count,
                      uint32_t *writeCount) {
  nsAutoCString* decodedData = static_cast<nsAutoCString*>(closure);
  decodedData->Append(rawSegment, count);
  *writeCount = count;
  return NS_OK;
}

NS_IMETHODIMP
PendingLookup::OnDataAvailable(nsIRequest *aRequest,
                               nsISupports *aContext,
                               nsIInputStream *aStream,
                               uint64_t offset,
                               uint32_t count) {
  uint32_t read;
  return aStream->ReadSegments(AppendSegmentToString, &mResponse, count, &read);
}

NS_IMETHODIMP
PendingLookup::OnStartRequest(nsIRequest *aRequest,
                              nsISupports *aContext) {
  return NS_OK;
}

NS_IMETHODIMP
PendingLookup::OnStopRequest(nsIRequest *aRequest,
                             nsISupports *aContext,
                             nsresult aResult) {
  NS_ENSURE_STATE(mCallback);

  bool shouldBlock = false;
  nsresult rv = OnStopRequestInternal(aRequest, aContext, aResult,
                                      &shouldBlock);
  OnComplete(shouldBlock, rv);
  return rv;
}

nsresult
PendingLookup::OnStopRequestInternal(nsIRequest *aRequest,
                                     nsISupports *aContext,
                                     nsresult aResult,
                                     bool* aShouldBlock) {
  *aShouldBlock = false;
  nsresult rv;
  nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(aRequest, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t status = 0;
  rv = channel->GetResponseStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if (status != 200) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  std::string buf(mResponse.Data(), mResponse.Length());
  safe_browsing::ClientDownloadResponse response;
  if (!response.ParseFromString(buf)) {
    NS_WARNING("Could not parse protocol buffer");
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }

  
  
  if (response.verdict() == safe_browsing::ClientDownloadResponse::DANGEROUS) {
    *aShouldBlock = true;
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(ApplicationReputationService,
                   nsIApplicationReputationService)

ApplicationReputationService*
  ApplicationReputationService::gApplicationReputationService = nullptr;

ApplicationReputationService*
ApplicationReputationService::GetSingleton()
{
  if (gApplicationReputationService) {
    NS_ADDREF(gApplicationReputationService);
    return gApplicationReputationService;
  }

  
  gApplicationReputationService = new ApplicationReputationService();
  if (gApplicationReputationService) {
    NS_ADDREF(gApplicationReputationService);
  }

  return gApplicationReputationService;
}

ApplicationReputationService::ApplicationReputationService() :
  mDBService(nullptr),
  mSecurityManager(nullptr) {
}

ApplicationReputationService::~ApplicationReputationService() {
}

NS_IMETHODIMP
ApplicationReputationService::QueryReputation(
    nsIApplicationReputationQuery* aQuery,
    nsIApplicationReputationCallback* aCallback) {
  NS_ENSURE_ARG_POINTER(aQuery);
  NS_ENSURE_ARG_POINTER(aCallback);

  nsresult rv = QueryReputationInternal(aQuery, aCallback);
  if (NS_FAILED(rv)) {
    aCallback->OnComplete(false, rv);
  }
  return NS_OK;
}

nsresult ApplicationReputationService::QueryReputationInternal(
  nsIApplicationReputationQuery* aQuery,
  nsIApplicationReputationCallback* aCallback) {
  
  nsresult rv;
  if (!mDBService) {
    mDBService = do_GetService(NS_URLCLASSIFIERDBSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (!mSecurityManager) {
    mSecurityManager = do_GetService("@mozilla.org/scriptsecuritymanager;1",
                                     &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  if (!Preferences::GetBool(PREF_SB_MALWARE_ENABLED, false)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsCString serviceUrl;
  NS_ENSURE_SUCCESS(Preferences::GetCString(PREF_SB_APP_REP_URL, &serviceUrl),
                    NS_ERROR_NOT_AVAILABLE);
  if (serviceUrl.EqualsLiteral("")) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsRefPtr<PendingLookup> lookup(new PendingLookup(aQuery, aCallback));
  NS_ENSURE_STATE(lookup);

  nsCOMPtr<nsIURI> uri;
  rv = aQuery->GetSourceURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_ENSURE_STATE(uri);
  nsCOMPtr<nsIPrincipal> principal;
  
  
  
  
  rv = mSecurityManager->GetNoAppCodebasePrincipal(uri,
                                                   getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  return mDBService->Lookup(principal, lookup);
}
