





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
#include "nsError.h"
#include "nsIContentSecurityPolicy.h"
#include "nsContentUtils.h"
#include "jswrapper.h"

#include "nsPrincipal.h"

#include "mozilla/Preferences.h"
#include "mozilla/HashFunctions.h"

#include "nsIAppsService.h"
#include "mozIApplication.h"

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


int32_t nsBasePrincipal::sCapabilitiesOrdinal = 0;
const char nsBasePrincipal::sInvalid[] = "Invalid";

NS_IMETHODIMP_(nsrefcnt)
nsBasePrincipal::AddRef()
{
  NS_PRECONDITION(int32_t(refcount) >= 0, "illegal refcnt");
  
  nsrefcnt count = PR_ATOMIC_INCREMENT(&refcount);
  NS_LOG_ADDREF(this, count, "nsBasePrincipal", sizeof(*this));
  return count;
}

NS_IMETHODIMP_(nsrefcnt)
nsBasePrincipal::Release()
{
  NS_PRECONDITION(0 != refcount, "dup release");
  nsrefcnt count = PR_ATOMIC_DECREMENT(&refcount);
  NS_LOG_RELEASE(this, count, "nsBasePrincipal");
  if (count == 0) {
    delete this;
  }

  return count;
}

nsBasePrincipal::nsBasePrincipal()
  : mCapabilities(nullptr),
    mSecurityPolicy(nullptr),
    mTrusted(false)
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

nsBasePrincipal::~nsBasePrincipal(void)
{
  SetSecurityPolicy(nullptr); 
  delete mCapabilities;
}

NS_IMETHODIMP
nsBasePrincipal::GetSecurityPolicy(void** aSecurityPolicy)
{
  if (mSecurityPolicy && mSecurityPolicy->IsInvalid()) 
    SetSecurityPolicy(nullptr);
  
  *aSecurityPolicy = (void *) mSecurityPolicy;
  return NS_OK;
}

NS_IMETHODIMP
nsBasePrincipal::SetSecurityPolicy(void* aSecurityPolicy)
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
nsBasePrincipal::CertificateEquals(nsIPrincipal *aOther)
{
  bool otherHasCert;
  aOther->GetHasCertificate(&otherHasCert);
  if (otherHasCert != (mCert != nullptr)) {
    
    return false;
  }

  if (!mCert)
    return true;

  nsAutoCString str;
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
nsBasePrincipal::CanEnableCapability(const char *capability, int16_t *result)
{
  
  if (mCapabilities) {
    nsCStringKey invalidKey(sInvalid);
    if (mCapabilities->Exists(&invalidKey)) {
      *result = nsIPrincipal::ENABLE_DENIED;

      return NS_OK;
    }
  }

  if (!mCert && !mTrusted) {
    
    
    
    
    
    nsCOMPtr<nsIURI> codebase;
    GetURI(getter_AddRefs(codebase));
    if (!gCodeBasePrincipalSupport && codebase) {
      bool mightEnable = false;     
      nsresult rv = codebase->SchemeIs("file", &mightEnable);
      if (NS_FAILED(rv) || !mightEnable) {
        rv = codebase->SchemeIs("resource", &mightEnable);
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
    int32_t len = space ? space - start : strlen(start);
    nsAutoCString capString(start, len);
    nsCStringKey key(capString);
    int16_t value =
      mCapabilities ? (int16_t)NS_PTR_TO_INT32(mCapabilities->Get(&key)) : 0;
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

nsresult
nsBasePrincipal::SetCanEnableCapability(const char *capability,
                                        int16_t canEnable)
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
    nsAutoCString capString(start, len);
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
nsBasePrincipal::IsCapabilityEnabled(const char *capability, void *annotation,
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
    nsAutoCString capString(start, len);
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
nsBasePrincipal::EnableCapability(const char *capability, void **annotation)
{
  return SetCapability(capability, annotation, AnnotationEnabled);
}

nsresult
nsBasePrincipal::SetCapability(const char *capability, void **annotation,
                               AnnotationValue value)
{
  if (*annotation == nullptr) {
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
    nsAutoCString capString(start, len);
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
nsBasePrincipal::GetHasCertificate(bool* aResult)
{
  *aResult = (mCert != nullptr);

  return NS_OK;
}

nsresult
nsBasePrincipal::SetCertificate(const nsACString& aFingerprint,
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
nsBasePrincipal::GetFingerprint(nsACString& aFingerprint)
{
  NS_ENSURE_STATE(mCert);

  aFingerprint = mCert->fingerprint;

  return NS_OK;
}

NS_IMETHODIMP
nsBasePrincipal::GetPrettyName(nsACString& aName)
{
  NS_ENSURE_STATE(mCert);

  aName = mCert->prettyName;

  return NS_OK;
}

NS_IMETHODIMP
nsBasePrincipal::GetSubjectName(nsACString& aName)
{
  NS_ENSURE_STATE(mCert);

  aName = mCert->subjectName;

  return NS_OK;
}

NS_IMETHODIMP
nsBasePrincipal::GetCertificate(nsISupports** aCertificate)
{
  if (mCert) {
    NS_IF_ADDREF(*aCertificate = mCert->cert);
  }
  else {
    *aCertificate = nullptr;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBasePrincipal::GetCsp(nsIContentSecurityPolicy** aCsp)
{
  NS_IF_ADDREF(*aCsp = mCSP);
  return NS_OK;
}

NS_IMETHODIMP
nsBasePrincipal::SetCsp(nsIContentSecurityPolicy* aCsp)
{
  
  
  if (mCSP)
    return NS_ERROR_ALREADY_INITIALIZED;

  mCSP = aCsp;
  return NS_OK;
}

nsresult
nsBasePrincipal::EnsureCertData(const nsACString& aSubjectName,
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
  int16_t value = (int16_t)NS_PTR_TO_INT32(aData);
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
nsBasePrincipal::GetPreferences(char** aPrefName, char** aID,
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

  *aPrefName = nullptr;
  *aID = nullptr;
  *aSubjectName = nullptr;
  *aGrantedList = nullptr;
  *aDeniedList = nullptr;
  *aIsTrusted = mTrusted;

  char *prefName = nullptr;
  char *id = nullptr;
  char *subjectName = nullptr;
  char *granted = nullptr;
  char *denied = nullptr;

  
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

  
  nsAutoCString grantedListStr, deniedListStr;
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

  uint32_t value;
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

#ifdef DEBUG
void nsPrincipal::dumpImpl()
{
  nsAutoCString str;
  GetScriptLocation(str);
  fprintf(stderr, "nsPrincipal (%p) = %s\n", static_cast<void*>(this), str.get());
}
#endif 

NS_IMPL_CLASSINFO(nsPrincipal, NULL, nsIClassInfo::MAIN_THREAD_ONLY,
                  NS_PRINCIPAL_CID)
NS_IMPL_QUERY_INTERFACE2_CI(nsPrincipal,
                            nsIPrincipal,
                            nsISerializable)
NS_IMPL_CI_INTERFACE_GETTER2(nsPrincipal,
                             nsIPrincipal,
                             nsISerializable)
NS_IMPL_ADDREF_INHERITED(nsPrincipal, nsBasePrincipal)
NS_IMPL_RELEASE_INHERITED(nsPrincipal, nsBasePrincipal)

nsPrincipal::nsPrincipal()
  : mAppId(nsIScriptSecurityManager::UNKNOWN_APP_ID)
  , mInMozBrowser(false)
  , mCodebaseImmutable(false)
  , mDomainImmutable(false)
  , mInitialized(false)
{ }

nsPrincipal::~nsPrincipal()
{ }

nsresult
nsPrincipal::Init(const nsACString& aCertFingerprint,
                  const nsACString& aSubjectName,
                  const nsACString& aPrettyName,
                  nsISupports* aCert,
                  nsIURI *aCodebase,
                  uint32_t aAppId,
                  bool aInMozBrowser)
{
  NS_ENSURE_STATE(!mInitialized);
  NS_ENSURE_ARG(!aCertFingerprint.IsEmpty() || aCodebase); 

  mInitialized = true;

  mCodebase = NS_TryToMakeImmutable(aCodebase);
  mCodebaseImmutable = URIIsImmutable(mCodebase);

  mAppId = aAppId;
  mInMozBrowser = aInMozBrowser;

  if (aCertFingerprint.IsEmpty())
    return NS_OK;

  return SetCertificate(aCertFingerprint, aSubjectName, aPrettyName, aCert);
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

 nsresult
nsPrincipal::GetOriginForURI(nsIURI* aURI, char **aOrigin)
{
  if (!aURI) {
    return NS_ERROR_FAILURE;
  }

  *aOrigin = nullptr;

  nsCOMPtr<nsIURI> origin = NS_GetInnermostURI(aURI);
  if (!origin) {
    return NS_ERROR_FAILURE;
  }

  nsAutoCString hostPort;

  
  
  
  
  bool isChrome;
  nsresult rv = origin->SchemeIs("chrome", &isChrome);
  if (NS_SUCCEEDED(rv) && !isChrome) {
    rv = origin->GetAsciiHost(hostPort);
    
    
    if (hostPort.IsEmpty()) {
      rv = NS_ERROR_FAILURE;
    }
  }

  int32_t port;
  if (NS_SUCCEEDED(rv) && !isChrome) {
    rv = origin->GetPort(&port);
  }

  if (NS_SUCCEEDED(rv) && !isChrome) {
    if (port != -1) {
      hostPort.AppendLiteral(":");
      hostPort.AppendInt(port, 10);
    }

    nsAutoCString scheme;
    rv = origin->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);

    *aOrigin = ToNewCString(scheme + NS_LITERAL_CSTRING("://") + hostPort);
  }
  else {
    
    
    nsAutoCString spec;
    
    
    rv = origin->GetAsciiSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    *aOrigin = ToNewCString(spec);
  }

  return *aOrigin ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsPrincipal::GetOrigin(char **aOrigin)
{
  return GetOriginForURI(mCodebase, aOrigin);
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

NS_IMETHODIMP
nsPrincipal::GetURI(nsIURI** aURI)
{
  if (mCodebaseImmutable) {
    NS_ADDREF(*aURI = mCodebase);
    return NS_OK;
  }

  if (!mCodebase) {
    *aURI = nullptr;
    return NS_OK;
  }

  return NS_EnsureSafeToReturn(mCodebase, aURI);
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
nsPrincipal::CheckMayLoad(nsIURI* aURI, bool aReport, bool aAllowIfInheritsPrincipal)
{
   if (aAllowIfInheritsPrincipal) {
    
    
    if (nsPrincipal::IsPrincipalInherited(aURI)) {
      return NS_OK;
    }
  }

  if (!nsScriptSecurityManager::SecurityCompareURIs(mCodebase, aURI)) {
    if (nsScriptSecurityManager::GetStrictFileOriginPolicy() &&
        URIIsLocalFile(aURI)) {
      nsCOMPtr<nsIFileURL> fileURL(do_QueryInterface(aURI));

      if (!URIIsLocalFile(mCodebase)) {
        
        
        
        
        

        if (aReport) {
          nsScriptSecurityManager::ReportError(
            nullptr, NS_LITERAL_STRING("CheckSameOriginError"), mCodebase, aURI);
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
            nullptr, NS_LITERAL_STRING("CheckSameOriginError"), mCodebase, aURI);
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
        nullptr, NS_LITERAL_STRING("CheckSameOriginError"), mCodebase, aURI);
    }
    
    return NS_ERROR_DOM_BAD_URI;
  }

  return NS_OK;
}

void
nsPrincipal::SetURI(nsIURI* aURI)
{
  mCodebase = NS_TryToMakeImmutable(aURI);
  mCodebaseImmutable = URIIsImmutable(mCodebase);
}

NS_IMETHODIMP
nsPrincipal::GetHashValue(uint32_t* aValue)
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
    *aDomain = nullptr;
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
  
  
  SetSecurityPolicy(nullptr);

  
  
  JSContext *cx = nsContentUtils::GetSafeJSContext();
  NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);
  JSPrincipals *principals = nsJSPrincipals::get(static_cast<nsIPrincipal*>(this));
  bool success = js::RecomputeWrappers(cx, js::ContentCompartmentsOnly(),
                                       js::CompartmentsWithPrincipals(principals));
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  success = js::RecomputeWrappers(cx, js::CompartmentsWithPrincipals(principals),
                                  js::ContentCompartmentsOnly());
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

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
                                bool aTrusted,
                                uint32_t aAppId,
                                bool aInMozBrowser)
{
  NS_PRECONDITION(!mCapabilities || mCapabilities->Count() == 0,
                  "mCapabilities was already initialized?");
  NS_PRECONDITION(mAnnotations.Length() == 0,
                  "mAnnotations was already initialized?");
  NS_PRECONDITION(!mInitialized, "We were already initialized?");

  mInitialized = true;

  mAppId = aAppId;
  mInMozBrowser = aInMozBrowser;

  nsresult rv;
  if (aIsCert) {
    rv = SetCertificate(aToken, aSubjectName, aPrettyName, aCert);
    
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  else {
    rv = NS_NewURI(getter_AddRefs(mCodebase), aToken, nullptr);
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
    int n = atoi(ordinalBegin);
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

NS_IMETHODIMP
nsPrincipal::GetExtendedOrigin(nsACString& aExtendedOrigin)
{
  MOZ_ASSERT(mAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID);

  mozilla::GetExtendedOrigin(mCodebase, mAppId, mInMozBrowser, aExtendedOrigin);
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetAppStatus(uint16_t* aAppStatus)
{
  MOZ_ASSERT(mAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID);

  *aAppStatus = GetAppStatus();
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetAppId(uint32_t* aAppId)
{
  if (mAppId == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    MOZ_ASSERT(false);
    *aAppId = nsIScriptSecurityManager::NO_APP_ID;
    return NS_OK;
  }

  *aAppId = mAppId;
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetIsInBrowserElement(bool* aIsInBrowserElement)
{
  *aIsInBrowserElement = mInMozBrowser;
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetUnknownAppId(bool* aUnknownAppId)
{
  *aUnknownAppId = mAppId == nsIScriptSecurityManager::UNKNOWN_APP_ID;
  return NS_OK;
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
    int n = atoi(ordinalBegin);
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

  nsCOMPtr<nsIURI> domain;
  rv = NS_ReadOptionalObject(aStream, true, getter_AddRefs(domain));
  if (NS_FAILED(rv)) {
    return rv;
  }

  uint32_t appId;
  rv = aStream->Read32(&appId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool inMozBrowser;
  rv = aStream->ReadBoolean(&inMozBrowser);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = Init(fingerprint, subjectName, prettyName, cert, codebase, appId, inMozBrowser);
  NS_ENSURE_SUCCESS(rv, rv);

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
  uint32_t value = NS_PTR_TO_INT32(aData);

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

  rv = aStream->WriteBoolean(mCert != nullptr);
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

  aStream->Write32(mAppId);
  aStream->WriteBoolean(mInMozBrowser);

  rv = aStream->Write8(mTrusted);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  

  return NS_OK;
}

uint16_t
nsPrincipal::GetAppStatus()
{
  MOZ_ASSERT(mAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID);

  
  
  if (mAppId == nsIScriptSecurityManager::NO_APP_ID ||
      mAppId == nsIScriptSecurityManager::UNKNOWN_APP_ID || mInMozBrowser) {
    return nsIPrincipal::APP_STATUS_NOT_INSTALLED;
  }

  nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(appsService, nsIPrincipal::APP_STATUS_NOT_INSTALLED);

  nsCOMPtr<mozIDOMApplication> domApp;
  appsService->GetAppByLocalId(mAppId, getter_AddRefs(domApp));
  nsCOMPtr<mozIApplication> app = do_QueryInterface(domApp);
  NS_ENSURE_TRUE(app, nsIPrincipal::APP_STATUS_NOT_INSTALLED);

  uint16_t status = nsIPrincipal::APP_STATUS_INSTALLED;
  NS_ENSURE_SUCCESS(app->GetAppStatus(&status),
                    nsIPrincipal::APP_STATUS_NOT_INSTALLED);

  nsAutoCString origin;
  NS_ENSURE_SUCCESS(GetOrigin(getter_Copies(origin)),
                    nsIPrincipal::APP_STATUS_NOT_INSTALLED);
  nsString appOrigin;
  NS_ENSURE_SUCCESS(app->GetOrigin(appOrigin),
                    nsIPrincipal::APP_STATUS_NOT_INSTALLED);

  
  
  nsCOMPtr<nsIURI> appURI;
  NS_ENSURE_SUCCESS(NS_NewURI(getter_AddRefs(appURI), appOrigin),
                    nsIPrincipal::APP_STATUS_NOT_INSTALLED);

  nsAutoCString appOriginPunned;
  NS_ENSURE_SUCCESS(GetOriginForURI(appURI, getter_Copies(appOriginPunned)),
                    nsIPrincipal::APP_STATUS_NOT_INSTALLED);

  if (!appOriginPunned.Equals(origin)) {
    return nsIPrincipal::APP_STATUS_NOT_INSTALLED;
  }

  return status;
}



static const char EXPANDED_PRINCIPAL_SPEC[] = "[Expanded Principal]";

NS_IMPL_CLASSINFO(nsExpandedPrincipal, NULL, nsIClassInfo::MAIN_THREAD_ONLY,
                  NS_EXPANDEDPRINCIPAL_CID)
NS_IMPL_QUERY_INTERFACE2_CI(nsExpandedPrincipal,
                            nsIPrincipal,
                            nsIExpandedPrincipal)
NS_IMPL_CI_INTERFACE_GETTER2(nsExpandedPrincipal,
                             nsIPrincipal,
                             nsIExpandedPrincipal)
NS_IMPL_ADDREF_INHERITED(nsExpandedPrincipal, nsBasePrincipal)
NS_IMPL_RELEASE_INHERITED(nsExpandedPrincipal, nsBasePrincipal)

nsExpandedPrincipal::nsExpandedPrincipal(nsTArray<nsCOMPtr <nsIPrincipal> > &aWhiteList)
{
  mPrincipals.AppendElements(aWhiteList);
}

nsExpandedPrincipal::~nsExpandedPrincipal()
{ }

NS_IMETHODIMP 
nsExpandedPrincipal::GetDomain(nsIURI** aDomain)
{
  *aDomain = nullptr;
  return NS_OK;
}

NS_IMETHODIMP 
nsExpandedPrincipal::SetDomain(nsIURI* aDomain)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsExpandedPrincipal::GetOrigin(char** aOrigin)
{
  *aOrigin = ToNewCString(NS_LITERAL_CSTRING(EXPANDED_PRINCIPAL_SPEC));
  return *aOrigin ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

typedef nsresult (NS_STDCALL nsIPrincipal::*nsIPrincipalMemFn)(nsIPrincipal* aOther,
                                                               bool* aResult);
#define CALL_MEMBER_FUNCTION(THIS,MEM_FN)  ((THIS)->*(MEM_FN))





static nsresult 
Equals(nsExpandedPrincipal* aThis, nsIPrincipalMemFn aFn, nsIPrincipal* aOther,
       bool* aResult)
{
  
  
  *aResult = false;
  
  nsresult rv = CALL_MEMBER_FUNCTION(aThis, aFn)(aOther, aResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!*aResult)
    return NS_OK;

  
  rv = CALL_MEMBER_FUNCTION(aOther, aFn)(aThis, aResult);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::Equals(nsIPrincipal* aOther, bool* aResult)
{
  return ::Equals(this, &nsIPrincipal::Subsumes, aOther, aResult);
}

NS_IMETHODIMP
nsExpandedPrincipal::EqualsIgnoringDomain(nsIPrincipal* aOther, bool* aResult)
{
  return ::Equals(this, &nsIPrincipal::SubsumesIgnoringDomain, aOther, aResult);
}




static nsresult 
Subsumes(nsExpandedPrincipal* aThis, nsIPrincipalMemFn aFn, nsIPrincipal* aOther, 
         bool* aResult)
{
  nsresult rv;
  nsCOMPtr<nsIExpandedPrincipal> expanded = do_QueryInterface(aOther);  
  if (expanded) {
    
    
    nsTArray< nsCOMPtr<nsIPrincipal> >* otherList;
    expanded->GetWhiteList(&otherList);
    for (uint32_t i = 0; i < otherList->Length(); ++i){
      rv = CALL_MEMBER_FUNCTION(aThis, aFn)((*otherList)[i], aResult);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!*aResult) {
        
        return NS_OK;    
      }
    }
  } else {
    
    nsTArray< nsCOMPtr<nsIPrincipal> >* list;
    aThis->GetWhiteList(&list);
    for (uint32_t i = 0; i < list->Length(); ++i){
      rv = CALL_MEMBER_FUNCTION((*list)[i], aFn)(aOther, aResult);
      NS_ENSURE_SUCCESS(rv, rv);
      if (*aResult) {
        
        return NS_OK;
      }
    }
  }
  return NS_OK;
}

#undef CALL_MEMBER_FUNCTION

NS_IMETHODIMP
nsExpandedPrincipal::Subsumes(nsIPrincipal* aOther, bool* aResult)
{
  return ::Subsumes(this, &nsIPrincipal::Subsumes, aOther, aResult);
}

NS_IMETHODIMP
nsExpandedPrincipal::SubsumesIgnoringDomain(nsIPrincipal* aOther, bool* aResult)
{
  return ::Subsumes(this, &nsIPrincipal::SubsumesIgnoringDomain, aOther, aResult);
}

NS_IMETHODIMP
nsExpandedPrincipal::CheckMayLoad(nsIURI* uri, bool aReport, bool aAllowIfInheritsPrincipal)
{
  nsresult rv;
  for (uint32_t i = 0; i < mPrincipals.Length(); ++i){
    rv = mPrincipals[i]->CheckMayLoad(uri, aReport, aAllowIfInheritsPrincipal);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_DOM_BAD_URI;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetHashValue(uint32_t* result)
{
  MOZ_NOT_REACHED("extended principal should never be used as key in a hash map");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetURI(nsIURI** aURI)
{
  *aURI = nullptr;
  return NS_OK;
}

NS_IMETHODIMP 
nsExpandedPrincipal::GetWhiteList(nsTArray<nsCOMPtr<nsIPrincipal> >** aWhiteList)
{
  *aWhiteList = &mPrincipals;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetExtendedOrigin(nsACString& aExtendedOrigin)
{
  return GetOrigin(getter_Copies(aExtendedOrigin));
}

NS_IMETHODIMP
nsExpandedPrincipal::GetAppStatus(uint16_t* aAppStatus)
{
  *aAppStatus = nsIPrincipal::APP_STATUS_NOT_INSTALLED;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetAppId(uint32_t* aAppId)
{
  *aAppId = nsIScriptSecurityManager::NO_APP_ID;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetIsInBrowserElement(bool* aIsInBrowserElement)
{
  *aIsInBrowserElement = false;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetUnknownAppId(bool* aUnknownAppId)
{
  *aUnknownAppId = false;
  return NS_OK;
}

void
nsExpandedPrincipal::GetScriptLocation(nsACString& aStr)
{
  if (mCert) {
    aStr.Assign(mCert->fingerprint);
  } else {
    
    aStr.Assign(EXPANDED_PRINCIPAL_SPEC);
  }
}

#ifdef DEBUG
void nsExpandedPrincipal::dumpImpl()
{
  fprintf(stderr, "nsExpandedPrincipal (%p)\n", static_cast<void*>(this));
}
#endif 





NS_IMETHODIMP
nsExpandedPrincipal::Read(nsIObjectInputStream* aStream)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsExpandedPrincipal::Write(nsIObjectOutputStream* aStream)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
