







































#include "nscore.h"
#include "nsScriptSecurityManager.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "plstr.h"
#include "nsCRT.h"
#include "nsIURI.h"
#include "nsIFileURL.h"
#include "nsIProtocolHandler.h"
#include "nsNetUtil.h"
#include "nsJSPrincipals.h"
#include "nsVoidArray.h"
#include "nsHashtable.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIClassInfoImpl.h"
#include "nsDOMError.h"
#include "nsIContentSecurityPolicy.h"

#include "nsPrincipal.h"

#include "mozilla/Preferences.h"
#include "mozilla/HashFunctions.h"

using namespace mozilla;

static bool gCodeBasePrincipalSupport = false;
static bool gIsObservingCodeBasePrincipalSupport = false;

static bool URIIsImmutable(nsIURI* aURI)
{
  nsCOMPtr<nsIMutable> mutableObj(do_QueryInterface(aURI));
  bool isMutable;
  return
    mutableObj &&
    NS_SUCCEEDED(mutableObj->GetMutable(&isMutable)) &&
    !isMutable;                               
}


PRInt32 nsPrincipal::sCapabilitiesOrdinal = 0;
const char nsPrincipal::sInvalid[] = "Invalid";


NS_IMPL_CLASSINFO(nsPrincipal, NULL, nsIClassInfo::MAIN_THREAD_ONLY,
                  NS_PRINCIPAL_CID)
NS_IMPL_QUERY_INTERFACE2_CI(nsPrincipal,
                            nsIPrincipal,
                            nsISerializable)
NS_IMPL_CI_INTERFACE_GETTER2(nsPrincipal,
                             nsIPrincipal,
                             nsISerializable)

NS_IMETHODIMP_(nsrefcnt)
nsPrincipal::AddRef()
{
  NS_PRECONDITION(PRInt32(refcount) >= 0, "illegal refcnt");
  
  nsrefcnt count = PR_ATOMIC_INCREMENT(&refcount);
  NS_LOG_ADDREF(this, count, "nsPrincipal", sizeof(*this));
  return count;
}

NS_IMETHODIMP_(nsrefcnt)
nsPrincipal::Release()
{
  NS_PRECONDITION(0 != refcount, "dup release");
  nsrefcnt count = PR_ATOMIC_DECREMENT(&refcount);
  NS_LOG_RELEASE(this, count, "nsPrincipal");
  if (count == 0) {
    delete this;
  }

  return count;
}

nsPrincipal::nsPrincipal()
  : mCapabilities(nsnull),
    mSecurityPolicy(nsnull),
    mTrusted(false),
    mInitialized(false),
    mCodebaseImmutable(false),
    mDomainImmutable(false)
{
  if (!gIsObservingCodeBasePrincipalSupport) {
    nsresult rv =
      Preferences::AddBoolVarCache(&gCodeBasePrincipalSupport,
                                   "signed.applets.codebase_principal_support",
                                   false);
    gIsObservingCodeBasePrincipalSupport = NS_SUCCEEDED(rv);
    NS_WARN_IF_FALSE(gIsObservingCodeBasePrincipalSupport,
                     "Installing gCodeBasePrincipalSupport failed!");
  }
}

nsresult
nsPrincipal::Init(const nsACString& aCertFingerprint,
                  const nsACString& aSubjectName,
                  const nsACString& aPrettyName,
                  nsISupports* aCert,
                  nsIURI *aCodebase)
{
  NS_ENSURE_STATE(!mInitialized);
  NS_ENSURE_ARG(!aCertFingerprint.IsEmpty() || aCodebase); 

  mInitialized = true;

  mCodebase = NS_TryToMakeImmutable(aCodebase);
  mCodebaseImmutable = URIIsImmutable(mCodebase);

  if (aCertFingerprint.IsEmpty())
    return NS_OK;

  return SetCertificate(aCertFingerprint, aSubjectName, aPrettyName, aCert);
}

nsPrincipal::~nsPrincipal(void)
{
  SetSecurityPolicy(nsnull); 
  delete mCapabilities;
}

void
nsPrincipal::GetScriptLocation(nsACString &aStr)
{
  if (mCert) {
    aStr.Assign(mCert->fingerprint);
  } else {
    mCodebase->GetSpec(aStr);
  }
}

#ifdef DEBUG
void nsPrincipal::dumpImpl()
{
  nsCAutoString str;
  GetScriptLocation(str);
  fprintf(stderr, "nsPrincipal (%p) = %s\n", this, str.get());
}
#endif 

NS_IMETHODIMP
nsPrincipal::GetOrigin(char **aOrigin)
{
  *aOrigin = nsnull;

  nsCOMPtr<nsIURI> origin;
  if (mCodebase) {
    origin = NS_GetInnermostURI(mCodebase);
  }
  
  if (!origin) {
    NS_ASSERTION(mCert, "No Domain or Codebase for a non-cert principal");
    return NS_ERROR_FAILURE;
  }

  nsCAutoString hostPort;

  
  
  
  
  bool isChrome;
  nsresult rv = origin->SchemeIs("chrome", &isChrome);
  if (NS_SUCCEEDED(rv) && !isChrome) {
    rv = origin->GetAsciiHost(hostPort);
    
    
    if (hostPort.IsEmpty())
      rv = NS_ERROR_FAILURE;
  }

  PRInt32 port;
  if (NS_SUCCEEDED(rv) && !isChrome) {
    rv = origin->GetPort(&port);
  }

  if (NS_SUCCEEDED(rv) && !isChrome) {
    if (port != -1) {
      hostPort.AppendLiteral(":");
      hostPort.AppendInt(port, 10);
    }

    nsCAutoString scheme;
    rv = origin->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);
    *aOrigin = ToNewCString(scheme + NS_LITERAL_CSTRING("://") + hostPort);
  }
  else {
    
    
    nsCAutoString spec;
    
    
    rv = origin->GetAsciiSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
    *aOrigin = ToNewCString(spec);
  }

  return *aOrigin ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsPrincipal::GetSecurityPolicy(void** aSecurityPolicy)
{
  if (mSecurityPolicy && mSecurityPolicy->IsInvalid()) 
    SetSecurityPolicy(nsnull);
  
  *aSecurityPolicy = (void *) mSecurityPolicy;
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::SetSecurityPolicy(void* aSecurityPolicy)
{
  DomainPolicy *newPolicy = reinterpret_cast<DomainPolicy *>(aSecurityPolicy);
  if (newPolicy)
    newPolicy->Hold();
 
  if (mSecurityPolicy)
    mSecurityPolicy->Drop();
  
  mSecurityPolicy = newPolicy;
  return NS_OK;
}

bool
nsPrincipal::CertificateEquals(nsIPrincipal *aOther)
{
  bool otherHasCert;
  aOther->GetHasCertificate(&otherHasCert);
  if (otherHasCert != (mCert != nsnull)) {
    
    return false;
  }

  if (!mCert)
    return true;

  nsCAutoString str;
  aOther->GetFingerprint(str);
  if (!str.Equals(mCert->fingerprint))
    return false;

  
  
  
  if (!mCert->subjectName.IsEmpty()) {
    
    aOther->GetSubjectName(str);
    return str.Equals(mCert->subjectName) || str.IsEmpty();
  }

  return true;
}

NS_IMETHODIMP
nsPrincipal::Equals(nsIPrincipal *aOther, bool *aResult)
{
  if (!aOther) {
    NS_WARNING("Need a principal to compare this to!");
    *aResult = false;
    return NS_OK;
  }

  if (this != aOther) {
    if (!CertificateEquals(aOther)) {
      *aResult = false;
      return NS_OK;
    }

    if (mCert) {
      
      
      
      nsCOMPtr<nsIURI> otherURI;
      nsresult rv = aOther->GetURI(getter_AddRefs(otherURI));
      if (NS_FAILED(rv)) {
        *aResult = false;
        return rv;
      }

      if (!otherURI || !mCodebase) {
        *aResult = true;
        return NS_OK;
      }

      
    }

    
    *aResult =
      NS_SUCCEEDED(nsScriptSecurityManager::CheckSameOriginPrincipal(this,
                                                                     aOther));
    return NS_OK;
  }

  *aResult = true;
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::EqualsIgnoringDomain(nsIPrincipal *aOther, bool *aResult)
{
  if (this == aOther) {
    *aResult = true;
    return NS_OK;
  }

  *aResult = false;
  if (!CertificateEquals(aOther)) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> otherURI;
  nsresult rv = aOther->GetURI(getter_AddRefs(otherURI));
  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_ASSERTION(mCodebase,
               "shouldn't be calling this on principals from preferences");

  
  *aResult = nsScriptSecurityManager::SecurityCompareURIs(mCodebase,
                                                          otherURI);
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::Subsumes(nsIPrincipal *aOther, bool *aResult)
{
  return Equals(aOther, aResult);
}

NS_IMETHODIMP
nsPrincipal::SubsumesIgnoringDomain(nsIPrincipal *aOther, bool *aResult)
{
  return EqualsIgnoringDomain(aOther, aResult);
}

static bool
URIIsLocalFile(nsIURI *aURI)
{
  bool isFile;
  nsCOMPtr<nsINetUtil> util = do_GetNetUtil();

  return util && NS_SUCCEEDED(util->ProtocolHasFlags(aURI,
                                nsIProtocolHandler::URI_IS_LOCAL_FILE,
                                &isFile)) &&
         isFile;
}

NS_IMETHODIMP
nsPrincipal::CheckMayLoad(nsIURI* aURI, bool aReport)
{
  if (!nsScriptSecurityManager::SecurityCompareURIs(mCodebase, aURI)) {
    if (nsScriptSecurityManager::GetStrictFileOriginPolicy() &&
        URIIsLocalFile(aURI)) {
      nsCOMPtr<nsIFileURL> fileURL(do_QueryInterface(aURI));

      if (!URIIsLocalFile(mCodebase)) {
        
        
        
        
        

        if (aReport) {
          nsScriptSecurityManager::ReportError(
            nsnull, NS_LITERAL_STRING("CheckSameOriginError"), mCodebase, aURI);
        }

        return NS_ERROR_DOM_BAD_URI;
      }

      
      
      
      nsCOMPtr<nsIFileURL> codebaseFileURL(do_QueryInterface(mCodebase));
      nsCOMPtr<nsIFile> targetFile;
      nsCOMPtr<nsIFile> codebaseFile;
      bool targetIsDir;

      
      

      if (!codebaseFileURL || !fileURL ||
          NS_FAILED(fileURL->GetFile(getter_AddRefs(targetFile))) ||
          NS_FAILED(codebaseFileURL->GetFile(getter_AddRefs(codebaseFile))) ||
          !targetFile || !codebaseFile ||
          NS_FAILED(targetFile->Normalize()) ||
          NS_FAILED(codebaseFile->Normalize()) ||
          NS_FAILED(targetFile->IsDirectory(&targetIsDir)) ||
          targetIsDir) {
        if (aReport) {
          nsScriptSecurityManager::ReportError(
            nsnull, NS_LITERAL_STRING("CheckSameOriginError"), mCodebase, aURI);
        }

        return NS_ERROR_DOM_BAD_URI;
      }

      
      
      
      
      
      bool codebaseIsDir;
      bool contained = false;
      nsresult rv = codebaseFile->IsDirectory(&codebaseIsDir);
      if (NS_SUCCEEDED(rv) && codebaseIsDir) {
        rv = codebaseFile->Contains(targetFile, true, &contained);
      }
      else {
        nsCOMPtr<nsIFile> codebaseParent;
        rv = codebaseFile->GetParent(getter_AddRefs(codebaseParent));
        if (NS_SUCCEEDED(rv) && codebaseParent) {
          rv = codebaseParent->Contains(targetFile, true, &contained);
        }
      }

      if (NS_SUCCEEDED(rv) && contained) {
        return NS_OK;
      }
    }

    if (aReport) {
      nsScriptSecurityManager::ReportError(
        nsnull, NS_LITERAL_STRING("CheckSameOriginError"), mCodebase, aURI);
    }
    
    return NS_ERROR_DOM_BAD_URI;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::CanEnableCapability(const char *capability, PRInt16 *result)
{
  
  if (mCapabilities) {
    nsCStringKey invalidKey(sInvalid);
    if (mCapabilities->Exists(&invalidKey)) {
      *result = nsIPrincipal::ENABLE_DENIED;

      return NS_OK;
    }
  }

  if (!mCert && !mTrusted) {
    NS_ASSERTION(mInitialized, "Trying to enable a capability on an "
                               "uninitialized principal");

    
    
    
    
    

    if (!gCodeBasePrincipalSupport) {
      bool mightEnable = false;
      nsresult rv = mCodebase->SchemeIs("file", &mightEnable);
      if (NS_FAILED(rv) || !mightEnable) {
        rv = mCodebase->SchemeIs("resource", &mightEnable);
        if (NS_FAILED(rv) || !mightEnable) {
          *result = nsIPrincipal::ENABLE_DENIED;

          return NS_OK;
        }
      }
    }
  }

  const char *start = capability;
  *result = nsIPrincipal::ENABLE_GRANTED;
  for(;;) {
    const char *space = PL_strchr(start, ' ');
    PRInt32 len = space ? space - start : strlen(start);
    nsCAutoString capString(start, len);
    nsCStringKey key(capString);
    PRInt16 value =
      mCapabilities ? (PRInt16)NS_PTR_TO_INT32(mCapabilities->Get(&key)) : 0;
    if (value == 0 || value == nsIPrincipal::ENABLE_UNKNOWN) {
      
      
      value = nsIPrincipal::ENABLE_WITH_USER_PERMISSION;
    }

    if (value < *result) {
      *result = value;
    }

    if (!space) {
      break;
    }

    start = space + 1;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::SetCanEnableCapability(const char *capability,
                                    PRInt16 canEnable)
{
  
  if (!mCapabilities) {
    mCapabilities = new nsHashtable(7);  
    NS_ENSURE_TRUE(mCapabilities, NS_ERROR_OUT_OF_MEMORY);
  }

  nsCStringKey invalidKey(sInvalid);
  if (mCapabilities->Exists(&invalidKey)) {
    return NS_OK;
  }

  if (PL_strcmp(capability, sInvalid) == 0) {
    mCapabilities->Reset();
  }

  const char *start = capability;
  for(;;) {
    const char *space = PL_strchr(start, ' ');
    int len = space ? space - start : strlen(start);
    nsCAutoString capString(start, len);
    nsCStringKey key(capString);
    mCapabilities->Put(&key, NS_INT32_TO_PTR(canEnable));
    if (!space) {
      break;
    }

    start = space + 1;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::IsCapabilityEnabled(const char *capability, void *annotation,
                                 bool *result)
{
  *result = false;
  nsHashtable *ht = (nsHashtable *) annotation;
  if (!ht) {
    return NS_OK;
  }
  const char *start = capability;
  for(;;) {
    const char *space = PL_strchr(start, ' ');
    int len = space ? space - start : strlen(start);
    nsCAutoString capString(start, len);
    nsCStringKey key(capString);
    *result = (ht->Get(&key) == (void *) AnnotationEnabled);
    if (!*result) {
      
      return NS_OK;
    }

    if (!space) {
      return NS_OK;
    }

    start = space + 1;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::EnableCapability(const char *capability, void **annotation)
{
  return SetCapability(capability, annotation, AnnotationEnabled);
}

NS_IMETHODIMP
nsPrincipal::DisableCapability(const char *capability, void **annotation)
{
  return SetCapability(capability, annotation, AnnotationDisabled);
}

NS_IMETHODIMP
nsPrincipal::RevertCapability(const char *capability, void **annotation)
{
  if (*annotation) {
    nsHashtable *ht = (nsHashtable *) *annotation;
    const char *start = capability;
    for(;;) {
      const char *space = PL_strchr(start, ' ');
      int len = space ? space - start : strlen(start);
      nsCAutoString capString(start, len);
      nsCStringKey key(capString);
      ht->Remove(&key);
      if (!space) {
        return NS_OK;
      }

      start = space + 1;
    }
  }
  return NS_OK;
}

nsresult
nsPrincipal::SetCapability(const char *capability, void **annotation,
                           AnnotationValue value)
{
  if (*annotation == nsnull) {
    nsHashtable* ht = new nsHashtable(5);

    if (!ht) {
       return NS_ERROR_OUT_OF_MEMORY;
     }

    
    
    if (!mAnnotations.AppendElement(ht)) {
      delete ht;
      return NS_ERROR_OUT_OF_MEMORY;
    }

    *annotation = ht;
  }

  const char *start = capability;
  for(;;) {
    const char *space = PL_strchr(start, ' ');
    int len = space ? space - start : strlen(start);
    nsCAutoString capString(start, len);
    nsCStringKey key(capString);
    nsHashtable *ht = static_cast<nsHashtable *>(*annotation);
    ht->Put(&key, (void *) value);
    if (!space) {
      break;
    }

    start = space + 1;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetHasCertificate(bool* aResult)
{
  *aResult = (mCert != nsnull);

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetURI(nsIURI** aURI)
{
  if (mCodebaseImmutable) {
    NS_ADDREF(*aURI = mCodebase);
    return NS_OK;
  }

  if (!mCodebase) {
    *aURI = nsnull;
    return NS_OK;
  }

  return NS_EnsureSafeToReturn(mCodebase, aURI);
}

void
nsPrincipal::SetURI(nsIURI* aURI)
{
  mCodebase = NS_TryToMakeImmutable(aURI);
  mCodebaseImmutable = URIIsImmutable(mCodebase);
}


nsresult
nsPrincipal::SetCertificate(const nsACString& aFingerprint,
                            const nsACString& aSubjectName,
                            const nsACString& aPrettyName,
                            nsISupports* aCert)
{
  NS_ENSURE_STATE(!mCert);

  if (aFingerprint.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }

  mCert = new Certificate(aFingerprint, aSubjectName, aPrettyName, aCert);
  if (!mCert) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetFingerprint(nsACString& aFingerprint)
{
  NS_ENSURE_STATE(mCert);

  aFingerprint = mCert->fingerprint;

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetPrettyName(nsACString& aName)
{
  NS_ENSURE_STATE(mCert);

  aName = mCert->prettyName;

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetSubjectName(nsACString& aName)
{
  NS_ENSURE_STATE(mCert);

  aName = mCert->subjectName;

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetCertificate(nsISupports** aCertificate)
{
  if (mCert) {
    NS_IF_ADDREF(*aCertificate = mCert->cert);
  }
  else {
    *aCertificate = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetCsp(nsIContentSecurityPolicy** aCsp)
{
  NS_IF_ADDREF(*aCsp = mCSP);
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::SetCsp(nsIContentSecurityPolicy* aCsp)
{
  
  
  if (mCSP)
    return NS_ERROR_ALREADY_INITIALIZED;

  mCSP = aCsp;
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetHashValue(PRUint32* aValue)
{
  NS_PRECONDITION(mCert || mCodebase, "Need a cert or codebase");

  
  if (mCert) {
    *aValue = HashString(mCert->fingerprint);
  }
  else {
    *aValue = nsScriptSecurityManager::HashPrincipalByOrigin(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetDomain(nsIURI** aDomain)
{
  if (!mDomain) {
    *aDomain = nsnull;
    return NS_OK;
  }

  if (mDomainImmutable) {
    NS_ADDREF(*aDomain = mDomain);
    return NS_OK;
  }

  return NS_EnsureSafeToReturn(mDomain, aDomain);
}

NS_IMETHODIMP
nsPrincipal::SetDomain(nsIURI* aDomain)
{
  mDomain = NS_TryToMakeImmutable(aDomain);
  mDomainImmutable = URIIsImmutable(mDomain);
  
  
  SetSecurityPolicy(nsnull);

  return NS_OK;
}

nsresult
nsPrincipal::InitFromPersistent(const char* aPrefName,
                                const nsCString& aToken,
                                const nsCString& aSubjectName,
                                const nsACString& aPrettyName,
                                const char* aGrantedList,
                                const char* aDeniedList,
                                nsISupports* aCert,
                                bool aIsCert,
                                bool aTrusted)
{
  NS_PRECONDITION(!mCapabilities || mCapabilities->Count() == 0,
                  "mCapabilities was already initialized?");
  NS_PRECONDITION(mAnnotations.Length() == 0,
                  "mAnnotations was already initialized?");
  NS_PRECONDITION(!mInitialized, "We were already initialized?");

  mInitialized = true;

  nsresult rv;
  if (aIsCert) {
    rv = SetCertificate(aToken, aSubjectName, aPrettyName, aCert);
    
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  else {
    rv = NS_NewURI(getter_AddRefs(mCodebase), aToken, nsnull);
    if (NS_FAILED(rv)) {
      NS_ERROR("Malformed URI in capability.principal preference.");
      return rv;
    }

    NS_TryToSetImmutable(mCodebase);
    mCodebaseImmutable = URIIsImmutable(mCodebase);

    mTrusted = aTrusted;
  }

  
  mPrefName = aPrefName;

  const char* ordinalBegin = PL_strpbrk(aPrefName, "1234567890");
  if (ordinalBegin) {
    PRIntn n = atoi(ordinalBegin);
    if (sCapabilitiesOrdinal <= n) {
      sCapabilitiesOrdinal = n + 1;
    }
  }

  
  rv = NS_OK;
  if (aGrantedList) {
    rv = SetCanEnableCapability(aGrantedList, nsIPrincipal::ENABLE_GRANTED);
  }

  if (NS_SUCCEEDED(rv) && aDeniedList) {
    rv = SetCanEnableCapability(aDeniedList, nsIPrincipal::ENABLE_DENIED);
  }

  return rv;
}

nsresult
nsPrincipal::EnsureCertData(const nsACString& aSubjectName,
                            const nsACString& aPrettyName,
                            nsISupports* aCert)
{
  NS_ENSURE_STATE(mCert);

  if (!mCert->subjectName.IsEmpty() &&
      !mCert->subjectName.Equals(aSubjectName)) {
    return NS_ERROR_INVALID_ARG;
  }

  mCert->subjectName = aSubjectName;
  mCert->prettyName = aPrettyName;
  mCert->cert = aCert;
  return NS_OK;
}

struct CapabilityList
{
  nsCString* granted;
  nsCString* denied;
};

static bool
AppendCapability(nsHashKey *aKey, void *aData, void *capListPtr)
{
  CapabilityList* capList = (CapabilityList*)capListPtr;
  PRInt16 value = (PRInt16)NS_PTR_TO_INT32(aData);
  nsCStringKey* key = (nsCStringKey *)aKey;
  if (value == nsIPrincipal::ENABLE_GRANTED) {
    capList->granted->Append(key->GetString(), key->GetStringLength());
    capList->granted->Append(' ');
  }
  else if (value == nsIPrincipal::ENABLE_DENIED) {
    capList->denied->Append(key->GetString(), key->GetStringLength());
    capList->denied->Append(' ');
  }

  return true;
}

NS_IMETHODIMP
nsPrincipal::GetPreferences(char** aPrefName, char** aID,
                            char** aSubjectName,
                            char** aGrantedList, char** aDeniedList,
                            bool* aIsTrusted)
{
  if (mPrefName.IsEmpty()) {
    if (mCert) {
      mPrefName.Assign("capability.principal.certificate.p");
    }
    else {
      mPrefName.Assign("capability.principal.codebase.p");
    }

    mPrefName.AppendInt(sCapabilitiesOrdinal++);
    mPrefName.Append(".id");
  }

  *aPrefName = nsnull;
  *aID = nsnull;
  *aSubjectName = nsnull;
  *aGrantedList = nsnull;
  *aDeniedList = nsnull;
  *aIsTrusted = mTrusted;

  char *prefName = nsnull;
  char *id = nsnull;
  char *subjectName = nsnull;
  char *granted = nsnull;
  char *denied = nsnull;

  
  prefName = ToNewCString(mPrefName);
  if (!prefName) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsresult rv = NS_OK;
  if (mCert) {
    id = ToNewCString(mCert->fingerprint);
    if (!id) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  else {
    rv = GetOrigin(&id);
  }

  if (NS_FAILED(rv)) {
    nsMemory::Free(prefName);
    return rv;
  }

  if (mCert) {
    subjectName = ToNewCString(mCert->subjectName);
  } else {
    subjectName = ToNewCString(EmptyCString());
  }

  if (!subjectName) {
    nsMemory::Free(prefName);
    nsMemory::Free(id);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsCAutoString grantedListStr, deniedListStr;
  if (mCapabilities) {
    CapabilityList capList = CapabilityList();
    capList.granted = &grantedListStr;
    capList.denied = &deniedListStr;
    mCapabilities->Enumerate(AppendCapability, (void*)&capList);
  }

  if (!grantedListStr.IsEmpty()) {
    grantedListStr.Truncate(grantedListStr.Length() - 1);
    granted = ToNewCString(grantedListStr);
    if (!granted) {
      nsMemory::Free(prefName);
      nsMemory::Free(id);
      nsMemory::Free(subjectName);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (!deniedListStr.IsEmpty()) {
    deniedListStr.Truncate(deniedListStr.Length() - 1);
    denied = ToNewCString(deniedListStr);
    if (!denied) {
      nsMemory::Free(prefName);
      nsMemory::Free(id);
      nsMemory::Free(subjectName);
      if (granted) {
        nsMemory::Free(granted);
      }
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  *aPrefName = prefName;
  *aID = id;
  *aSubjectName = subjectName;
  *aGrantedList = granted;
  *aDeniedList = denied;

  return NS_OK;
}

static nsresult
ReadAnnotationEntry(nsIObjectInputStream* aStream, nsHashKey** aKey,
                    void** aData)
{
  nsresult rv;
  nsCStringKey* key = new nsCStringKey(aStream, &rv);
  if (!key)
    return NS_ERROR_OUT_OF_MEMORY;

  if (NS_FAILED(rv)) {
    delete key;
    return rv;
  }

  PRUint32 value;
  rv = aStream->Read32(&value);
  if (NS_FAILED(rv)) {
    delete key;
    return rv;
  }

  *aKey = key;
  *aData = (void*) value;
  return NS_OK;
}

static void
FreeAnnotationEntry(nsIObjectInputStream* aStream, nsHashKey* aKey,
                    void* aData)
{
  delete aKey;
}

NS_IMETHODIMP
nsPrincipal::Read(nsIObjectInputStream* aStream)
{
  bool hasCapabilities;
  nsresult rv = aStream->ReadBoolean(&hasCapabilities);
  if (NS_SUCCEEDED(rv) && hasCapabilities) {
    mCapabilities = new nsHashtable(aStream, ReadAnnotationEntry,
                                    FreeAnnotationEntry, &rv);
    NS_ENSURE_TRUE(mCapabilities, NS_ERROR_OUT_OF_MEMORY);
  }

  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = NS_ReadOptionalCString(aStream, mPrefName);
  if (NS_FAILED(rv)) {
    return rv;
  }

  const char* ordinalBegin = PL_strpbrk(mPrefName.get(), "1234567890");
  if (ordinalBegin) {
    PRIntn n = atoi(ordinalBegin);
    if (sCapabilitiesOrdinal <= n) {
      sCapabilitiesOrdinal = n + 1;
    }
  }

  bool haveCert;
  rv = aStream->ReadBoolean(&haveCert);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCString fingerprint;
  nsCString subjectName;
  nsCString prettyName;
  nsCOMPtr<nsISupports> cert;
  if (haveCert) {
    rv = NS_ReadOptionalCString(aStream, fingerprint);
    if (NS_FAILED(rv)) {
      return rv;
    }

    rv = NS_ReadOptionalCString(aStream, subjectName);
    if (NS_FAILED(rv)) {
      return rv;
    }

    rv = NS_ReadOptionalCString(aStream, prettyName);
    if (NS_FAILED(rv)) {
      return rv;
    }

    rv = aStream->ReadObject(true, getter_AddRefs(cert));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  nsCOMPtr<nsIURI> codebase;
  rv = NS_ReadOptionalObject(aStream, true, getter_AddRefs(codebase));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Init(fingerprint, subjectName, prettyName, cert, codebase);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> domain;
  rv = NS_ReadOptionalObject(aStream, true, getter_AddRefs(domain));
  if (NS_FAILED(rv)) {
    return rv;
  }

  SetDomain(domain);

  rv = aStream->ReadBoolean(&mTrusted);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

static nsresult
WriteScalarValue(nsIObjectOutputStream* aStream, void* aData)
{
  PRUint32 value = NS_PTR_TO_INT32(aData);

  return aStream->Write32(value);
}

NS_IMETHODIMP
nsPrincipal::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_STATE(mCert || mCodebase);
  
  
  
  
  bool hasCapabilities = (mCapabilities && mCapabilities->Count() > 0);
  nsresult rv = aStream->WriteBoolean(hasCapabilities);
  if (NS_SUCCEEDED(rv) && hasCapabilities) {
    rv = mCapabilities->Write(aStream, WriteScalarValue);
  }

  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = NS_WriteOptionalStringZ(aStream, mPrefName.get());
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = aStream->WriteBoolean(mCert != nsnull);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (mCert) {
    NS_ENSURE_STATE(mCert->cert);
    
    rv = NS_WriteOptionalStringZ(aStream, mCert->fingerprint.get());
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    rv = NS_WriteOptionalStringZ(aStream, mCert->subjectName.get());
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    rv = NS_WriteOptionalStringZ(aStream, mCert->prettyName.get());
    if (NS_FAILED(rv)) {
      return rv;
    }

    rv = aStream->WriteCompoundObject(mCert->cert, NS_GET_IID(nsISupports),
                                      true);
    if (NS_FAILED(rv)) {
      return rv;
    }    
  }
  
  
  
  

  rv = NS_WriteOptionalCompoundObject(aStream, mCodebase, NS_GET_IID(nsIURI),
                                      true);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = NS_WriteOptionalCompoundObject(aStream, mDomain, NS_GET_IID(nsIURI),
                                      true);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = aStream->Write8(mTrusted);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  

  return NS_OK;
}
