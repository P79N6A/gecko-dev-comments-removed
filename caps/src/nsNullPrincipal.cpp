











































#include "nsNullPrincipal.h"
#include "nsMemory.h"
#include "nsIUUIDGenerator.h"
#include "nsID.h"
#include "prmem.h" 
#include "nsNetUtil.h"
#include "nsIClassInfoImpl.h"
#include "nsNetCID.h"

static NS_DEFINE_CID(kSimpleURICID, NS_SIMPLEURI_CID);

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
  nsrefcnt count = PR_AtomicIncrement((PRInt32 *)&mJSPrincipals.refcount);
  NS_LOG_ADDREF(this, count, "nsNullPrincipal", sizeof(*this));
  return count;
}

NS_IMETHODIMP_(nsrefcnt)
nsNullPrincipal::Release()
{
  NS_PRECONDITION(0 != mJSPrincipals.refcount, "dup release");
  nsrefcnt count = PR_AtomicDecrement((PRInt32 *)&mJSPrincipals.refcount);
  NS_LOG_RELEASE(this, count, "nsNullPrincipal");
  if (count == 0) {
    NS_DELETEXPCOM(this);
  }

  return count;
}

nsNullPrincipal::nsNullPrincipal()
{
}

nsNullPrincipal::~nsNullPrincipal()
{
}

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

  char* chars = id.ToString();
  NS_ENSURE_TRUE(chars, NS_ERROR_OUT_OF_MEMORY);

  nsCAutoString str(NS_NULLPRINCIPAL_SCHEME ":");
  PRUint32 prefixLen = str.Length();
  PRUint32 suffixLen = strlen(chars);

  str.Append(chars);

  PR_Free(chars);
  
  if (str.Length() != prefixLen + suffixLen) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  mURI = do_CreateInstance(kSimpleURICID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mURI->SetSpec(str);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_TryToSetImmutable(mURI);

  return mJSPrincipals.Init(this, str.get());
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

