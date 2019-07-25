











































#include "nsNullPrincipal.h"
#include "nsNullPrincipalURI.h"
#include "nsMemory.h"
#include "nsIUUIDGenerator.h"
#include "nsID.h"
#include "nsNetUtil.h"
#include "nsIClassInfoImpl.h"
#include "nsNetCID.h"
#include "nsDOMError.h"
#include "nsScriptSecurityManager.h"

NS_IMPL_CLASSINFO(nsNullPrincipal, NULL, nsIClassInfo::MAIN_THREAD_ONLY,
                  NS_NULLPRINCIPAL_CID)
NS_IMPL_QUERY_INTERFACE2_CI(nsNullPrincipal,
                            nsIPrincipal,
                            nsISerializable)
NS_IMPL_CI_INTERFACE_GETTER2(nsNullPrincipal,
                             nsIPrincipal,
                             nsISerializable)

NS_IMETHODIMP_(nsrefcnt) 
nsNullPrincipal::AddRef()
{
  NS_PRECONDITION(PRInt32(mJSPrincipals.refcount) >= 0, "illegal refcnt");
  nsrefcnt count = PR_ATOMIC_INCREMENT(&mJSPrincipals.refcount);
  NS_LOG_ADDREF(this, count, "nsNullPrincipal", sizeof(*this));
  return count;
}

NS_IMETHODIMP_(nsrefcnt)
nsNullPrincipal::Release()
{
  NS_PRECONDITION(0 != mJSPrincipals.refcount, "dup release");
  nsrefcnt count = PR_ATOMIC_DECREMENT(&mJSPrincipals.refcount);
  NS_LOG_RELEASE(this, count, "nsNullPrincipal");
  if (count == 0) {
    delete this;
  }

  return count;
}

nsNullPrincipal::nsNullPrincipal()
{
}

nsNullPrincipal::~nsNullPrincipal()
{
}

#define NS_NULLPRINCIPAL_PREFIX NS_NULLPRINCIPAL_SCHEME ":"

nsresult
nsNullPrincipal::Init()
{
  
  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidgen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsID id;
  rv = uuidgen->GenerateUUIDInPlace(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  char chars[NSID_LENGTH];
  id.ToProvidedString(chars);

  PRUint32 suffixLen = NSID_LENGTH - 1;
  PRUint32 prefixLen = NS_ARRAY_LENGTH(NS_NULLPRINCIPAL_PREFIX) - 1;

  
  
  nsCString str;
  str.SetCapacity(prefixLen + suffixLen);

  str.Append(NS_NULLPRINCIPAL_PREFIX);
  str.Append(chars);

  if (str.Length() != prefixLen + suffixLen) {
    NS_WARNING("Out of memory allocating null-principal URI");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mURI = new nsNullPrincipalURI(str);
  NS_ENSURE_TRUE(mURI, NS_ERROR_OUT_OF_MEMORY);

  return mJSPrincipals.Init(this, str);
}





NS_IMETHODIMP
nsNullPrincipal::GetPreferences(char** aPrefName, char** aID,
                                char** aSubjectName,
                                char** aGrantedList, char** aDeniedList,
                                PRBool* aIsTrusted)
{
  
  *aPrefName = nsnull;
  *aID = nsnull;
  *aSubjectName = nsnull;
  *aGrantedList = nsnull;
  *aDeniedList = nsnull;
  *aIsTrusted = PR_FALSE;

  return NS_ERROR_FAILURE; 
}

NS_IMETHODIMP
nsNullPrincipal::Equals(nsIPrincipal *aOther, PRBool *aResult)
{
  
  
  *aResult = (aOther == this);
  return NS_OK;
}

NS_IMETHODIMP
nsNullPrincipal::EqualsIgnoringDomain(nsIPrincipal *aOther, PRBool *aResult)
{
  return Equals(aOther, aResult);
}

NS_IMETHODIMP
nsNullPrincipal::GetHashValue(PRUint32 *aResult)
{
  *aResult = (NS_PTR_TO_INT32(this) >> 2);
  return NS_OK;
}

NS_IMETHODIMP
nsNullPrincipal::GetJSPrincipals(JSContext *cx, JSPrincipals **aJsprin)
{
  NS_PRECONDITION(mJSPrincipals.nsIPrincipalPtr,
                  "mJSPrincipals is uninitalized!");

  JSPRINCIPALS_HOLD(cx, &mJSPrincipals);
  *aJsprin = &mJSPrincipals;
  return NS_OK;
}

NS_IMETHODIMP
nsNullPrincipal::GetSecurityPolicy(void** aSecurityPolicy)
{
  
  
  *aSecurityPolicy = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsNullPrincipal::SetSecurityPolicy(void* aSecurityPolicy)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP 
nsNullPrincipal::CanEnableCapability(const char *aCapability, 
                                     PRInt16 *aResult)
{
  
  *aResult = nsIPrincipal::ENABLE_DENIED;
  return NS_OK;
}

NS_IMETHODIMP 
nsNullPrincipal::SetCanEnableCapability(const char *aCapability, 
                                        PRInt16 aCanEnable)
{
  return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP 
nsNullPrincipal::IsCapabilityEnabled(const char *aCapability, 
                                     void *aAnnotation, 
                                     PRBool *aResult)
{
  
  *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP 
nsNullPrincipal::EnableCapability(const char *aCapability, void **aAnnotation)
{
  NS_NOTREACHED("Didn't I say it?  NO CAPABILITIES!");
  *aAnnotation = nsnull;
  return NS_OK;
}

NS_IMETHODIMP 
nsNullPrincipal::RevertCapability(const char *aCapability, void **aAnnotation)
{
    *aAnnotation = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsNullPrincipal::DisableCapability(const char *aCapability, void **aAnnotation)
{
  
  *aAnnotation = nsnull;
  return NS_OK;
}

NS_IMETHODIMP 
nsNullPrincipal::GetURI(nsIURI** aURI)
{
  return NS_EnsureSafeToReturn(mURI, aURI);
}

NS_IMETHODIMP
nsNullPrincipal::GetCsp(nsIContentSecurityPolicy** aCsp)
{
  
  *aCsp = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsNullPrincipal::SetCsp(nsIContentSecurityPolicy* aCsp)
{
  
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsNullPrincipal::GetDomain(nsIURI** aDomain)
{
  return NS_EnsureSafeToReturn(mURI, aDomain);
}

NS_IMETHODIMP
nsNullPrincipal::SetDomain(nsIURI* aDomain)
{
  
  
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP 
nsNullPrincipal::GetOrigin(char** aOrigin)
{
  *aOrigin = nsnull;
  
  nsCAutoString str;
  nsresult rv = mURI->GetSpec(str);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOrigin = ToNewCString(str);
  NS_ENSURE_TRUE(*aOrigin, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP 
nsNullPrincipal::GetHasCertificate(PRBool* aResult)
{
  *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP 
nsNullPrincipal::GetFingerprint(nsACString& aID)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP 
nsNullPrincipal::GetPrettyName(nsACString& aName)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsNullPrincipal::Subsumes(nsIPrincipal *aOther, PRBool *aResult)
{
  
  
  
  *aResult = (aOther == this);
  return NS_OK;
}

NS_IMETHODIMP
nsNullPrincipal::CheckMayLoad(nsIURI* aURI, PRBool aReport)
{
  if (aReport) {
    nsScriptSecurityManager::ReportError(
      nsnull, NS_LITERAL_STRING("CheckSameOriginError"), mURI, aURI);
  }

  return NS_ERROR_DOM_BAD_URI;
}

NS_IMETHODIMP 
nsNullPrincipal::GetSubjectName(nsACString& aName)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsNullPrincipal::GetCertificate(nsISupports** aCertificate)
{
    *aCertificate = nsnull;
    return NS_OK;
}




NS_IMETHODIMP
nsNullPrincipal::Read(nsIObjectInputStream* aStream)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsNullPrincipal::Write(nsIObjectOutputStream* aStream)
{
  
  
  return NS_OK;
}

