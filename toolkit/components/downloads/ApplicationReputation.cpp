







#include "ApplicationReputation.h"
#include "csd.pb.h"

#include "nsIArray.h"
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
#include "nsIX509Cert.h"
#include "nsIX509CertDB.h"
#include "nsIX509CertList.h"

#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/LoadContext.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsNetCID.h"
#include "nsReadableUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "nsXPCOMStrings.h"

#include "nsIContentPolicy.h"
#include "nsILoadInfo.h"
#include "nsContentUtils.h"

using mozilla::Preferences;
using mozilla::TimeStamp;
using mozilla::Telemetry::Accumulate;
using safe_browsing::ClientDownloadRequest;
using safe_browsing::ClientDownloadRequest_CertificateChain;
using safe_browsing::ClientDownloadRequest_Resource;
using safe_browsing::ClientDownloadRequest_SignatureInfo;


#define PREF_SB_APP_REP_URL "browser.safebrowsing.appRepURL"
#define PREF_SB_MALWARE_ENABLED "browser.safebrowsing.malware.enabled"
#define PREF_SB_DOWNLOADS_ENABLED "browser.safebrowsing.downloads.enabled"
#define PREF_SB_DOWNLOADS_REMOTE_ENABLED "browser.safebrowsing.downloads.remote.enabled"
#define PREF_GENERAL_LOCALE "general.useragent.locale"
#define PREF_DOWNLOAD_BLOCK_TABLE "urlclassifier.downloadBlockTable"
#define PREF_DOWNLOAD_ALLOW_TABLE "urlclassifier.downloadAllowTable"


#if defined(PR_LOGGING)
PRLogModuleInfo *ApplicationReputationService::prlog = nullptr;
#define LOG(args) PR_LOG(ApplicationReputationService::prlog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(ApplicationReputationService::prlog, 4)
#else
#define LOG(args)
#define LOG_ENABLED() (false)
#endif

class PendingDBLookup;





class PendingLookup final : public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  
  PendingLookup(nsIApplicationReputationQuery* aQuery,
                nsIApplicationReputationCallback* aCallback);

  
  
  
  
  
  nsresult StartLookup();

private:
  ~PendingLookup();

  friend class PendingDBLookup;

  
  
  enum SERVER_RESPONSE_TYPES {
    SERVER_RESPONSE_VALID = 0,
    SERVER_RESPONSE_FAILED = 1,
    SERVER_RESPONSE_INVALID = 2,
  };

  
  uint32_t mBlocklistCount;
  uint32_t mAllowlistCount;

  
  nsCOMPtr<nsIApplicationReputationQuery> mQuery;

  
  nsCOMPtr<nsIApplicationReputationCallback> mCallback;

  
  
  nsTArray<nsCString> mAllowlistSpecs;
  
  nsTArray<nsCString> mAnylistSpecs;

  
  TimeStamp mStartTime;

  
  
  
  
  ClientDownloadRequest mRequest;

  
  
  
  nsCString mResponse;

  
  bool IsBinaryFile();

  
  ClientDownloadRequest::DownloadType GetDownloadType(const nsAString& aFilename);

  
  
  nsresult OnComplete(bool shouldBlock, nsresult rv);

  
  
  nsresult OnStopRequestInternal(nsIRequest *aRequest,
                                 nsISupports *aContext,
                                 nsresult aResult,
                                 bool* aShouldBlock);

  
  
  nsresult GetStrippedSpec(nsIURI* aUri, nsACString& spec);

  
  nsCString EscapeCertificateAttribute(const nsACString& aAttribute);

  
  nsCString EscapeFingerprint(const nsACString& aAttribute);

  
  
  nsresult GenerateWhitelistStringsForPair(
    nsIX509Cert* certificate, nsIX509Cert* issuer);

  
  
  nsresult GenerateWhitelistStringsForChain(
    const ClientDownloadRequest_CertificateChain& aChain);

  
  
  
  
  
  nsresult GenerateWhitelistStrings();

  
  
  nsresult ParseCertificates(nsIArray* aSigArray);

  
  nsresult AddRedirects(nsIArray* aRedirects);

  
  
  nsresult DoLookupInternal();

  
  
  
  
  nsresult LookupNext();

  
  
  nsresult SendRemoteQuery();

  
  nsresult SendRemoteQueryInternal();
};



class PendingDBLookup final : public nsIUrlClassifierCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERCALLBACK

  
  explicit PendingDBLookup(PendingLookup* aPendingLookup);

  
  
  
  nsresult LookupSpec(const nsACString& aSpec, bool aAllowlistOnly);

private:
  ~PendingDBLookup();

  
  
  enum LIST_TYPES {
    ALLOW_LIST = 0,
    BLOCK_LIST = 1,
    NO_LIST = 2,
  };

  nsCString mSpec;
  bool mAllowlistOnly;
  nsRefPtr<PendingLookup> mPendingLookup;
  nsresult LookupSpecInternal(const nsACString& aSpec);
};

NS_IMPL_ISUPPORTS(PendingDBLookup,
                  nsIUrlClassifierCallback)

PendingDBLookup::PendingDBLookup(PendingLookup* aPendingLookup) :
  mAllowlistOnly(false),
  mPendingLookup(aPendingLookup)
{
  LOG(("Created pending DB lookup [this = %p]", this));
}

PendingDBLookup::~PendingDBLookup()
{
  LOG(("Destroying pending DB lookup [this = %p]", this));
  mPendingLookup = nullptr;
}

nsresult
PendingDBLookup::LookupSpec(const nsACString& aSpec,
                            bool aAllowlistOnly)
{
  LOG(("Checking principal %s [this=%p]", aSpec.Data(), this));
  mSpec = aSpec;
  mAllowlistOnly = aAllowlistOnly;
  nsresult rv = LookupSpecInternal(aSpec);
  if (NS_FAILED(rv)) {
    LOG(("Error in LookupSpecInternal"));
    return mPendingLookup->OnComplete(false, NS_OK);
  }
  
  
  return rv;
}

nsresult
PendingDBLookup::LookupSpecInternal(const nsACString& aSpec)
{
  nsresult rv;

  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsIIOService> ios = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  rv = ios->NewURI(aSpec, nullptr, nullptr, getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrincipal> principal;
  nsCOMPtr<nsIScriptSecurityManager> secMan =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = secMan->GetNoAppCodebasePrincipal(uri, getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  LOG(("Checking DB service for principal %s [this = %p]", mSpec.get(), this));
  nsCOMPtr<nsIUrlClassifierDBService> dbService =
    do_GetService(NS_URLCLASSIFIERDBSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString tables;
  nsAutoCString allowlist;
  Preferences::GetCString(PREF_DOWNLOAD_ALLOW_TABLE, &allowlist);
  if (!allowlist.IsEmpty()) {
    tables.Append(allowlist);
  }
  nsAutoCString blocklist;
  Preferences::GetCString(PREF_DOWNLOAD_BLOCK_TABLE, &blocklist);
  if (!mAllowlistOnly && !blocklist.IsEmpty()) {
    tables.Append(',');
    tables.Append(blocklist);
  }
  return dbService->Lookup(principal, tables, this);
}

NS_IMETHODIMP
PendingDBLookup::HandleEvent(const nsACString& tables)
{
  
  
  
  
  nsAutoCString blockList;
  Preferences::GetCString(PREF_DOWNLOAD_BLOCK_TABLE, &blockList);
  if (!mAllowlistOnly && FindInReadable(blockList, tables)) {
    mPendingLookup->mBlocklistCount++;
    Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_LOCAL, BLOCK_LIST);
    LOG(("Found principal %s on blocklist [this = %p]", mSpec.get(), this));
    return mPendingLookup->OnComplete(true, NS_OK);
  }

  nsAutoCString allowList;
  Preferences::GetCString(PREF_DOWNLOAD_ALLOW_TABLE, &allowList);
  if (FindInReadable(allowList, tables)) {
    mPendingLookup->mAllowlistCount++;
    Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_LOCAL, ALLOW_LIST);
    LOG(("Found principal %s on allowlist [this = %p]", mSpec.get(), this));
    
  } else {
    LOG(("Didn't find principal %s on any list [this = %p]", mSpec.get(),
         this));
    Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_LOCAL, NO_LIST);
  }
  return mPendingLookup->LookupNext();
}

NS_IMPL_ISUPPORTS(PendingLookup,
                  nsIStreamListener,
                  nsIRequestObserver)

PendingLookup::PendingLookup(nsIApplicationReputationQuery* aQuery,
                             nsIApplicationReputationCallback* aCallback) :
  mBlocklistCount(0),
  mAllowlistCount(0),
  mQuery(aQuery),
  mCallback(aCallback)
{
  LOG(("Created pending lookup [this = %p]", this));
}

PendingLookup::~PendingLookup()
{
  LOG(("Destroying pending lookup [this = %p]", this));
}

bool
PendingLookup::IsBinaryFile()
{
  nsString fileName;
  nsresult rv = mQuery->GetSuggestedFileName(fileName);
  if (NS_FAILED(rv)) {
    LOG(("No suggested filename [this = %p]", this));
    return false;
  }
  LOG(("Suggested filename: %s [this = %p]",
       NS_ConvertUTF16toUTF8(fileName).get(), this));
  return
    
    
    StringEndsWith(fileName, NS_LITERAL_STRING(".apk")) ||
    
    StringEndsWith(fileName, NS_LITERAL_STRING(".bas")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".bat")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".cab")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".cmd")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".com")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".exe")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".hta")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".msi")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".pif")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".reg")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".scr")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".vb")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".vbs")) ||
    
    StringEndsWith(fileName, NS_LITERAL_STRING(".app")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".dmg")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".osx")) ||
    StringEndsWith(fileName, NS_LITERAL_STRING(".pkg")) ||
    
    StringEndsWith(fileName, NS_LITERAL_STRING(".zip"));
}

ClientDownloadRequest::DownloadType
PendingLookup::GetDownloadType(const nsAString& aFilename) {
  MOZ_ASSERT(IsBinaryFile());

  
  if (StringEndsWith(aFilename, NS_LITERAL_STRING(".zip"))) {
    return ClientDownloadRequest::ZIPPED_EXECUTABLE;
  } else if (StringEndsWith(aFilename, NS_LITERAL_STRING(".apk"))) {
    return ClientDownloadRequest::ANDROID_APK;
  } else if (StringEndsWith(aFilename, NS_LITERAL_STRING(".app")) ||
             StringEndsWith(aFilename, NS_LITERAL_STRING(".dmg")) ||
             StringEndsWith(aFilename, NS_LITERAL_STRING(".osx")) ||
             StringEndsWith(aFilename, NS_LITERAL_STRING(".pkg"))) {
    return ClientDownloadRequest::MAC_EXECUTABLE;
  }

  return ClientDownloadRequest::WIN_EXECUTABLE; 
}

nsresult
PendingLookup::LookupNext()
{
  
  
  
  if (mBlocklistCount > 0) {
    return OnComplete(true, NS_OK);
  }
  int index = mAnylistSpecs.Length() - 1;
  nsCString spec;
  if (index >= 0) {
    
    spec = mAnylistSpecs[index];
    mAnylistSpecs.RemoveElementAt(index);
    nsRefPtr<PendingDBLookup> lookup(new PendingDBLookup(this));
    return lookup->LookupSpec(spec, false);
  }
  
  if (mBlocklistCount > 0) {
    return OnComplete(true, NS_OK);
  }
  
  if (mAllowlistCount > 0) {
    return OnComplete(false, NS_OK);
  }
  
  index = mAllowlistSpecs.Length() - 1;
  if (index >= 0) {
    spec = mAllowlistSpecs[index];
    LOG(("PendingLookup::LookupNext: checking %s on allowlist", spec.get()));
    mAllowlistSpecs.RemoveElementAt(index);
    nsRefPtr<PendingDBLookup> lookup(new PendingDBLookup(this));
    return lookup->LookupSpec(spec, true);
  }
  
  
  if (!IsBinaryFile()) {
    LOG(("Not eligible for remote lookups [this=%x]", this));
    return OnComplete(false, NS_OK);
  }
  nsresult rv = SendRemoteQuery();
  if (NS_FAILED(rv)) {
    return OnComplete(false, rv);
  }
  return NS_OK;
}

nsCString
PendingLookup::EscapeCertificateAttribute(const nsACString& aAttribute)
{
  
  nsCString escaped;
  escaped.SetCapacity(aAttribute.Length());
  for (unsigned int i = 0; i < aAttribute.Length(); ++i) {
    if (aAttribute.Data()[i] == '%') {
      escaped.AppendLiteral("%25");
    } else if (aAttribute.Data()[i] == '/') {
      escaped.AppendLiteral("%2F");
    } else if (aAttribute.Data()[i] == ' ') {
      escaped.AppendLiteral("%20");
    } else {
      escaped.Append(aAttribute.Data()[i]);
    }
  }
  return escaped;
}

nsCString
PendingLookup::EscapeFingerprint(const nsACString& aFingerprint)
{
  
  nsCString escaped;
  escaped.SetCapacity(aFingerprint.Length());
  for (unsigned int i = 0; i < aFingerprint.Length(); ++i) {
    if (aFingerprint.Data()[i] != ':') {
      escaped.Append(aFingerprint.Data()[i]);
    }
  }
  return escaped;
}

nsresult
PendingLookup::GenerateWhitelistStringsForPair(
  nsIX509Cert* certificate,
  nsIX509Cert* issuer)
{
  
  
  
  
  
  
  nsCString whitelistString(
    "http://sb-ssl.google.com/safebrowsing/csd/certificate/");

  nsString fingerprint;
  nsresult rv = issuer->GetSha1Fingerprint(fingerprint);
  NS_ENSURE_SUCCESS(rv, rv);
  whitelistString.Append(
    EscapeFingerprint(NS_ConvertUTF16toUTF8(fingerprint)));

  nsString commonName;
  rv = certificate->GetCommonName(commonName);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!commonName.IsEmpty()) {
    whitelistString.AppendLiteral("/CN=");
    whitelistString.Append(
      EscapeCertificateAttribute(NS_ConvertUTF16toUTF8(commonName)));
  }

  nsString organization;
  rv = certificate->GetOrganization(organization);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!organization.IsEmpty()) {
    whitelistString.AppendLiteral("/O=");
    whitelistString.Append(
      EscapeCertificateAttribute(NS_ConvertUTF16toUTF8(organization)));
  }

  nsString organizationalUnit;
  rv = certificate->GetOrganizationalUnit(organizationalUnit);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!organizationalUnit.IsEmpty()) {
    whitelistString.AppendLiteral("/OU=");
    whitelistString.Append(
      EscapeCertificateAttribute(NS_ConvertUTF16toUTF8(organizationalUnit)));
  }
  LOG(("Whitelisting %s", whitelistString.get()));

  mAllowlistSpecs.AppendElement(whitelistString);
  return NS_OK;
}

nsresult
PendingLookup::GenerateWhitelistStringsForChain(
  const safe_browsing::ClientDownloadRequest_CertificateChain& aChain)
{
  
  
  if (aChain.element_size() < 2) {
    return NS_OK;
  }

  
  nsresult rv;
  nsCOMPtr<nsIX509CertDB> certDB = do_GetService(NS_X509CERTDB_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIX509Cert> signer;
  rv = certDB->ConstructX509(
    const_cast<char *>(aChain.element(0).certificate().data()),
    aChain.element(0).certificate().size(), getter_AddRefs(signer));
  NS_ENSURE_SUCCESS(rv, rv);

  for (int i = 1; i < aChain.element_size(); ++i) {
    
    nsCOMPtr<nsIX509Cert> issuer;
    rv = certDB->ConstructX509(
      const_cast<char *>(aChain.element(i).certificate().data()),
      aChain.element(i).certificate().size(), getter_AddRefs(issuer));
    NS_ENSURE_SUCCESS(rv, rv);

    nsresult rv = GenerateWhitelistStringsForPair(signer, issuer);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
PendingLookup::GenerateWhitelistStrings()
{
  for (int i = 0; i < mRequest.signature().certificate_chain_size(); ++i) {
    nsresult rv = GenerateWhitelistStringsForChain(
      mRequest.signature().certificate_chain(i));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
PendingLookup::AddRedirects(nsIArray* aRedirects)
{
  uint32_t length = 0;
  aRedirects->GetLength(&length);
  LOG(("ApplicationReputation: Got %u redirects", length));
  nsCOMPtr<nsISimpleEnumerator> iter;
  nsresult rv = aRedirects->Enumerate(getter_AddRefs(iter));
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMoreRedirects = false;
  rv = iter->HasMoreElements(&hasMoreRedirects);
  NS_ENSURE_SUCCESS(rv, rv);

  while (hasMoreRedirects) {
    nsCOMPtr<nsISupports> supports;
    rv = iter->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrincipal> principal = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> uri;
    rv = principal->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCString spec;
    rv = GetStrippedSpec(uri, spec);
    NS_ENSURE_SUCCESS(rv, rv);
    mAnylistSpecs.AppendElement(spec);
    LOG(("ApplicationReputation: Appending redirect %s\n", spec.get()));

    
    ClientDownloadRequest_Resource* resource = mRequest.add_resources();
    resource->set_url(spec.get());
    resource->set_type(ClientDownloadRequest::DOWNLOAD_REDIRECT);

    rv = iter->HasMoreElements(&hasMoreRedirects);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
PendingLookup::StartLookup()
{
  mStartTime = TimeStamp::Now();
  nsresult rv = DoLookupInternal();
  if (NS_FAILED(rv)) {
    return OnComplete(false, NS_OK);
  };
  return rv;
}

nsresult
PendingLookup::GetStrippedSpec(nsIURI* aUri, nsACString& escaped)
{
  
  
  nsresult rv;
  nsCOMPtr<nsIURL> url = do_QueryInterface(aUri, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = url->GetScheme(escaped);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString temp;
  rv = url->GetHostPort(temp);
  NS_ENSURE_SUCCESS(rv, rv);

  escaped.Append("://");
  escaped.Append(temp);

  rv = url->GetFilePath(temp);
  NS_ENSURE_SUCCESS(rv, rv);

  
  escaped.Append(temp);

  return NS_OK;
}

nsresult
PendingLookup::DoLookupInternal()
{
  
  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = mQuery->GetSourceURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString spec;
  rv = GetStrippedSpec(uri, spec);
  NS_ENSURE_SUCCESS(rv, rv);

  mAnylistSpecs.AppendElement(spec);

  ClientDownloadRequest_Resource* resource = mRequest.add_resources();
  resource->set_url(spec.get());
  resource->set_type(ClientDownloadRequest::DOWNLOAD_URL);

  nsCOMPtr<nsIURI> referrer = nullptr;
  rv = mQuery->GetReferrerURI(getter_AddRefs(referrer));
  if (referrer) {
    nsCString spec;
    rv = GetStrippedSpec(referrer, spec);
    NS_ENSURE_SUCCESS(rv, rv);
    mAnylistSpecs.AppendElement(spec);
    resource->set_referrer(spec.get());
  }
  nsCOMPtr<nsIArray> redirects;
  rv = mQuery->GetRedirects(getter_AddRefs(redirects));
  if (redirects) {
    AddRedirects(redirects);
  } else {
    LOG(("ApplicationReputation: Got no redirects [this=%p]", this));
  }

  
  
  nsCOMPtr<nsIArray> sigArray;
  rv = mQuery->GetSignatureInfo(getter_AddRefs(sigArray));
  NS_ENSURE_SUCCESS(rv, rv);

  if (sigArray) {
    rv = ParseCertificates(sigArray);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = GenerateWhitelistStrings();
  NS_ENSURE_SUCCESS(rv, rv);

  
  return LookupNext();
}

nsresult
PendingLookup::OnComplete(bool shouldBlock, nsresult rv)
{
  Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_SHOULD_BLOCK,
    shouldBlock);
#if defined(PR_LOGGING)
  double t = (TimeStamp::Now() - mStartTime).ToMilliseconds();
#endif
  if (shouldBlock) {
    LOG(("Application Reputation check failed, blocking bad binary in %f ms "
         "[this = %p]", t, this));
  } else {
    LOG(("Application Reputation check passed in %f ms [this = %p]", t, this));
  }
  nsresult res = mCallback->OnComplete(shouldBlock, rv);
  return res;
}

nsresult
PendingLookup::ParseCertificates(nsIArray* aSigArray)
{
  
  NS_ENSURE_ARG_POINTER(aSigArray);

  
  
  
  nsCOMPtr<nsISimpleEnumerator> chains;
  nsresult rv = aSigArray->Enumerate(getter_AddRefs(chains));
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMoreChains = false;
  rv = chains->HasMoreElements(&hasMoreChains);
  NS_ENSURE_SUCCESS(rv, rv);

  while (hasMoreChains) {
    nsCOMPtr<nsISupports> supports;
    rv = chains->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIX509CertList> certList = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    safe_browsing::ClientDownloadRequest_CertificateChain* certChain =
      mRequest.mutable_signature()->add_certificate_chain();
    nsCOMPtr<nsISimpleEnumerator> chainElt;
    rv = certList->GetEnumerator(getter_AddRefs(chainElt));
    NS_ENSURE_SUCCESS(rv, rv);

    
    bool hasMoreCerts = false;
    rv = chainElt->HasMoreElements(&hasMoreCerts);
    while (hasMoreCerts) {
      nsCOMPtr<nsISupports> supports;
      rv = chainElt->GetNext(getter_AddRefs(supports));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIX509Cert> cert = do_QueryInterface(supports, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      uint8_t* data = nullptr;
      uint32_t len = 0;
      rv = cert->GetRawDER(&len, &data);
      NS_ENSURE_SUCCESS(rv, rv);

      
      certChain->add_element()->set_certificate(data, len);
      free(data);

      rv = chainElt->HasMoreElements(&hasMoreCerts);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = chains->HasMoreElements(&hasMoreChains);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (mRequest.signature().certificate_chain_size() > 0) {
    mRequest.mutable_signature()->set_trusted(true);
  }
  return NS_OK;
}

nsresult
PendingLookup::SendRemoteQuery()
{
  nsresult rv = SendRemoteQueryInternal();
  if (NS_FAILED(rv)) {
    LOG(("Failed sending remote query for application reputation "
         "[this = %p]", this));
    return OnComplete(false, rv);
  }
  
  
  return rv;
}

nsresult
PendingLookup::SendRemoteQueryInternal()
{
  
  if (!Preferences::GetBool(PREF_SB_DOWNLOADS_REMOTE_ENABLED, false)) {
    LOG(("Remote lookups are disabled [this = %p]", this));
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  nsCString serviceUrl;
  NS_ENSURE_SUCCESS(Preferences::GetCString(PREF_SB_APP_REP_URL, &serviceUrl),
                    NS_ERROR_NOT_AVAILABLE);
  if (serviceUrl.EqualsLiteral("")) {
    LOG(("Remote lookup URL is empty [this = %p]", this));
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  nsCString table;
  NS_ENSURE_SUCCESS(Preferences::GetCString(PREF_DOWNLOAD_BLOCK_TABLE, &table),
                    NS_ERROR_NOT_AVAILABLE);
  if (table.EqualsLiteral("")) {
    LOG(("Blocklist is empty [this = %p]", this));
    return NS_ERROR_NOT_AVAILABLE;
  }
#ifdef XP_WIN
  
  NS_ENSURE_SUCCESS(Preferences::GetCString(PREF_DOWNLOAD_ALLOW_TABLE, &table),
                    NS_ERROR_NOT_AVAILABLE);
  if (table.EqualsLiteral("")) {
    LOG(("Allowlist is empty [this = %p]", this));
    return NS_ERROR_NOT_AVAILABLE;
  }
#endif

  LOG(("Sending remote query for application reputation [this = %p]",
       this));
  
  
  nsCOMPtr<nsIURI> uri;
  nsresult rv;
  rv = mQuery->GetSourceURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString spec;
  rv = GetStrippedSpec(uri, spec);
  NS_ENSURE_SUCCESS(rv, rv);
  mRequest.set_url(spec.get());

  uint32_t fileSize;
  rv = mQuery->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);
  mRequest.set_length(fileSize);
  
  
  mRequest.set_user_initiated(true);

  nsCString locale;
  NS_ENSURE_SUCCESS(Preferences::GetCString(PREF_GENERAL_LOCALE, &locale),
                    NS_ERROR_NOT_AVAILABLE);
  mRequest.set_locale(locale.get());
  nsCString sha256Hash;
  rv = mQuery->GetSha256Hash(sha256Hash);
  NS_ENSURE_SUCCESS(rv, rv);
  mRequest.mutable_digests()->set_sha256(sha256Hash.Data());
  nsString fileName;
  rv = mQuery->GetSuggestedFileName(fileName);
  NS_ENSURE_SUCCESS(rv, rv);
  mRequest.set_file_basename(NS_ConvertUTF16toUTF8(fileName).get());
  mRequest.set_download_type(GetDownloadType(fileName));

  if (mRequest.signature().trusted()) {
    LOG(("Got signed binary for remote application reputation check "
         "[this = %p]", this));
  } else {
    LOG(("Got unsigned binary for remote application reputation check "
         "[this = %p]", this));
  }

  
  
  
  std::string serialized;
  if (!mRequest.SerializeToString(&serialized)) {
    return NS_ERROR_UNEXPECTED;
  }
  LOG(("Serialized protocol buffer [this = %p]: (length=%d) %s", this,
       serialized.length(), serialized.c_str()));

  
  nsCOMPtr<nsIStringInputStream> sstream =
    do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sstream->SetData(serialized.c_str(), serialized.length());
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIChannel> channel;
  nsCOMPtr<nsIIOService> ios = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  rv = ios->NewChannel2(serviceUrl,
                        nullptr,
                        nullptr,
                        nullptr, 
                        nsContentUtils::GetSystemPrincipal(),
                        nullptr, 
                        nsILoadInfo::SEC_NORMAL,
                        nsIContentPolicy::TYPE_OTHER,
                        getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIUploadChannel2> uploadChannel = do_QueryInterface(channel, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = uploadChannel->ExplicitSetUploadStream(sstream,
    NS_LITERAL_CSTRING("application/octet-stream"), serialized.size(),
    NS_LITERAL_CSTRING("POST"), false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIInterfaceRequestor> loadContext =
    new mozilla::LoadContext(NECKO_SAFEBROWSING_APP_ID);
  rv = channel->SetNotificationCallbacks(loadContext);
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
  if (NS_FAILED(aResult)) {
    Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_SERVER,
      SERVER_RESPONSE_FAILED);
    return aResult;
  }

  *aShouldBlock = false;
  nsresult rv;
  nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(aRequest, &rv);
  if (NS_FAILED(rv)) {
    Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_SERVER,
      SERVER_RESPONSE_FAILED);
    return rv;
  }

  uint32_t status = 0;
  rv = channel->GetResponseStatus(&status);
  if (NS_FAILED(rv)) {
    Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_SERVER,
      SERVER_RESPONSE_FAILED);
    return rv;
  }

  if (status != 200) {
    Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_SERVER,
      SERVER_RESPONSE_FAILED);
    return NS_ERROR_NOT_AVAILABLE;
  }

  std::string buf(mResponse.Data(), mResponse.Length());
  safe_browsing::ClientDownloadResponse response;
  if (!response.ParseFromString(buf)) {
    LOG(("Invalid protocol buffer response [this = %p]: %s", this, buf.c_str()));
    Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_SERVER,
                                   SERVER_RESPONSE_INVALID);
    return NS_ERROR_CANNOT_CONVERT_DATA;
  }

  
  
  Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_SERVER,
    SERVER_RESPONSE_VALID);
  
  Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_SERVER_VERDICT,
    std::min<uint32_t>(response.verdict(), 7));
  switch(response.verdict()) {
    case safe_browsing::ClientDownloadResponse::DANGEROUS:
    case safe_browsing::ClientDownloadResponse::DANGEROUS_HOST:
      *aShouldBlock = true;
      break;
    default:
      break;
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS(ApplicationReputationService,
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

ApplicationReputationService::ApplicationReputationService()
{
#if defined(PR_LOGGING)
  if (!prlog) {
    prlog = PR_NewLogModule("ApplicationReputation");
  }
#endif
  LOG(("Application reputation service started up"));
}

ApplicationReputationService::~ApplicationReputationService() {
  LOG(("Application reputation service shutting down"));
}

NS_IMETHODIMP
ApplicationReputationService::QueryReputation(
    nsIApplicationReputationQuery* aQuery,
    nsIApplicationReputationCallback* aCallback) {
  LOG(("Starting application reputation check [query=%p]", aQuery));
  NS_ENSURE_ARG_POINTER(aQuery);
  NS_ENSURE_ARG_POINTER(aCallback);

  Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_COUNT, true);
  nsresult rv = QueryReputationInternal(aQuery, aCallback);
  if (NS_FAILED(rv)) {
    Accumulate(mozilla::Telemetry::APPLICATION_REPUTATION_SHOULD_BLOCK,
      false);
    aCallback->OnComplete(false, rv);
  }
  return NS_OK;
}

nsresult ApplicationReputationService::QueryReputationInternal(
  nsIApplicationReputationQuery* aQuery,
  nsIApplicationReputationCallback* aCallback) {
  nsresult rv;
  
  if (!Preferences::GetBool(PREF_SB_MALWARE_ENABLED, false)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!Preferences::GetBool(PREF_SB_DOWNLOADS_ENABLED, false)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIURI> uri;
  rv = aQuery->GetSourceURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_ENSURE_STATE(uri);

  
  nsRefPtr<PendingLookup> lookup(new PendingLookup(aQuery, aCallback));
  NS_ENSURE_STATE(lookup);

  return lookup->StartLookup();
}
