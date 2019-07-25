






































#include "nsClientAuthRemember.h"

#include "nsIX509Cert.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsIObserverService.h"
#include "nsNetUtil.h"
#include "nsISupportsPrimitives.h"
#include "nsPromiseFlatString.h"
#include "nsProxiedService.h"
#include "nsStringBuffer.h"
#include "nspr.h"
#include "pk11pub.h"
#include "certdb.h"
#include "sechash.h"
#include "ssl.h" 

#include "nsNSSCleaner.h"

using namespace mozilla;

NSSCleanupAutoPtrClass(CERTCertificate, CERT_DestroyCertificate)

NS_IMPL_THREADSAFE_ISUPPORTS2(nsClientAuthRememberService, 
                              nsIObserver,
                              nsISupportsWeakReference)

nsClientAuthRememberService::nsClientAuthRememberService()
  : monitor("nsClientAuthRememberService.monitor")
{
}

nsClientAuthRememberService::~nsClientAuthRememberService()
{
  RemoveAllFromMemory();
}

nsresult
nsClientAuthRememberService::Init()
{
  if (!mSettingsTable.Init())
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIProxyObjectManager> proxyman(do_GetService(NS_XPCOMPROXY_CONTRACTID));
  if (!proxyman)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1"));
  nsCOMPtr<nsIObserverService> proxiedObserver;

  NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                       NS_GET_IID(nsIObserverService),
                       observerService,
                       NS_PROXY_SYNC,
                       getter_AddRefs(proxiedObserver));

  if (proxiedObserver) {
    proxiedObserver->AddObserver(this, "profile-before-change", PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsClientAuthRememberService::Observe(nsISupports     *aSubject,
                               const char      *aTopic,
                               const PRUnichar *aData)
{
  
  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    
    

    ReentrantMonitorAutoEnter lock(monitor);
    RemoveAllFromMemory();
  }

  return NS_OK;
}

void nsClientAuthRememberService::ClearRememberedDecisions()
{
  ReentrantMonitorAutoEnter lock(monitor);
  RemoveAllFromMemory();
}

void
nsClientAuthRememberService::RemoveAllFromMemory()
{
  mSettingsTable.Clear();
}

static nsresult
GetCertFingerprintByOidTag(CERTCertificate* nsscert,
                           SECOidTag aOidTag, 
                           nsCString &fp)
{
  unsigned int hash_len = HASH_ResultLenByOidTag(aOidTag);
  nsRefPtr<nsStringBuffer> fingerprint = nsStringBuffer::Alloc(hash_len);
  if (!fingerprint)
    return NS_ERROR_OUT_OF_MEMORY;

  PK11_HashBuf(aOidTag, (unsigned char*)fingerprint->Data(), 
               nsscert->derCert.data, nsscert->derCert.len);

  SECItem fpItem;
  fpItem.data = (unsigned char*)fingerprint->Data();
  fpItem.len = hash_len;

  fp.Adopt(CERT_Hexify(&fpItem, 1));
  return NS_OK;
}

nsresult
nsClientAuthRememberService::RememberDecision(const nsACString & aHostName, 
                                              CERTCertificate *aServerCert, CERTCertificate *aClientCert)
{
  
  NS_ENSURE_ARG_POINTER(aServerCert);
  if (aHostName.IsEmpty())
    return NS_ERROR_INVALID_ARG;

  nsCAutoString fpStr;
  nsresult rv = GetCertFingerprintByOidTag(aServerCert, SEC_OID_SHA256, fpStr);
  if (NS_FAILED(rv))
    return rv;

  {
    ReentrantMonitorAutoEnter lock(monitor);
    if (aClientCert) {
      nsNSSCertificate pipCert(aClientCert);
      char *dbkey = NULL;
      rv = pipCert.GetDbKey(&dbkey);
      if (NS_SUCCEEDED(rv) && dbkey) {
        AddEntryToList(aHostName, fpStr, 
                       nsDependentCString(dbkey));
      }
      if (dbkey) {
        PORT_Free(dbkey);
      }
    }
    else {
      nsCString empty;
      AddEntryToList(aHostName, fpStr, empty);
    }
  }

  return NS_OK;
}

nsresult
nsClientAuthRememberService::HasRememberedDecision(const nsACString & aHostName, 
                                                   CERTCertificate *aCert, 
                                                   nsACString & aCertDBKey,
                                                   PRBool *_retval)
{
  if (aHostName.IsEmpty())
    return NS_ERROR_INVALID_ARG;

  NS_ENSURE_ARG_POINTER(aCert);
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;

  nsresult rv;
  nsCAutoString fpStr;
  rv = GetCertFingerprintByOidTag(aCert, SEC_OID_SHA256, fpStr);
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString hostCert;
  GetHostWithCert(aHostName, fpStr, hostCert);
  nsClientAuthRemember settings;

  {
    ReentrantMonitorAutoEnter lock(monitor);
    nsClientAuthRememberEntry *entry = mSettingsTable.GetEntry(hostCert.get());
    if (!entry)
      return NS_OK;
    settings = entry->mSettings; 
  }

  aCertDBKey = settings.mDBKey;
  *_retval = PR_TRUE;
  return NS_OK;
}

nsresult
nsClientAuthRememberService::AddEntryToList(const nsACString &aHostName, 
                                      const nsACString &fingerprint,
                                      const nsACString &db_key)

{
  nsCAutoString hostCert;
  GetHostWithCert(aHostName, fingerprint, hostCert);

  {
    ReentrantMonitorAutoEnter lock(monitor);
    nsClientAuthRememberEntry *entry = mSettingsTable.PutEntry(hostCert.get());

    if (!entry) {
      NS_ERROR("can't insert a null entry!");
      return NS_ERROR_OUT_OF_MEMORY;
    }

    entry->mHostWithCert = hostCert;

    nsClientAuthRemember &settings = entry->mSettings;
    settings.mAsciiHost = aHostName;
    settings.mFingerprint = fingerprint;
    settings.mDBKey = db_key;
  }

  return NS_OK;
}

void
nsClientAuthRememberService::GetHostWithCert(const nsACString & aHostName, 
                                             const nsACString & fingerprint, 
                                             nsACString& _retval)
{
  nsCAutoString hostCert(aHostName);
  hostCert.AppendLiteral(":");
  hostCert.Append(fingerprint);
  
  _retval.Assign(hostCert);
}
