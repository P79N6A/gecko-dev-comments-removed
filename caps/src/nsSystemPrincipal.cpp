






































#include "nscore.h"
#include "nsSystemPrincipal.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsIClassInfoImpl.h"

NS_IMPL_CLASSINFO(nsSystemPrincipal, NULL,
                  nsIClassInfo::SINGLETON | nsIClassInfo::MAIN_THREAD_ONLY,
                  NS_SYSTEMPRINCIPAL_CID)
NS_IMPL_QUERY_INTERFACE2_CI(nsSystemPrincipal,
                            nsIPrincipal,
                            nsISerializable)
NS_IMPL_CI_INTERFACE_GETTER2(nsSystemPrincipal,
                             nsIPrincipal,
                             nsISerializable)

NS_IMETHODIMP_(nsrefcnt) 
nsSystemPrincipal::AddRef()
{
  NS_PRECONDITION(PRInt32(mJSPrincipals.refcount) >= 0, "illegal refcnt");
  nsrefcnt count = PR_ATOMIC_INCREMENT(&mJSPrincipals.refcount);
  NS_LOG_ADDREF(this, count, "nsSystemPrincipal", sizeof(*this));
  return count;
}

NS_IMETHODIMP_(nsrefcnt)
nsSystemPrincipal::Release()
{
  NS_PRECONDITION(0 != mJSPrincipals.refcount, "dup release");
  nsrefcnt count = PR_ATOMIC_DECREMENT(&mJSPrincipals.refcount);
  NS_LOG_RELEASE(this, count, "nsSystemPrincipal");
  if (count == 0) {
    delete this;
  }

  return count;
}






NS_IMETHODIMP
nsSystemPrincipal::GetPreferences(char** aPrefName, char** aID,
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
nsSystemPrincipal::Equals(nsIPrincipal *other, PRBool *result)
{
    *result = (other == this);
    return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::EqualsIgnoringDomain(nsIPrincipal *other, PRBool *result)
{
    return Equals(other, result);
}

NS_IMETHODIMP
nsSystemPrincipal::Subsumes(nsIPrincipal *other, PRBool *result)
{
    *result = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::CheckMayLoad(nsIURI* uri, PRBool aReport)
{
    return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::GetHashValue(PRUint32 *result)
{
    *result = NS_PTR_TO_INT32(this);
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::CanEnableCapability(const char *capability, 
                                       PRInt16 *result)
{
    
    *result = nsIPrincipal::ENABLE_GRANTED;
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::SetCanEnableCapability(const char *capability, 
                                          PRInt16 canEnable)
{
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP 
nsSystemPrincipal::IsCapabilityEnabled(const char *capability, 
                                       void *annotation, 
                                       PRBool *result)
{
    *result = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::EnableCapability(const char *capability, void **annotation)
{
    *annotation = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::RevertCapability(const char *capability, void **annotation)
{
    *annotation = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::DisableCapability(const char *capability, void **annotation) 
{
    
    
    *annotation = nsnull;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsSystemPrincipal::GetURI(nsIURI** aURI)
{
    *aURI = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::GetOrigin(char** aOrigin)
{
    *aOrigin = ToNewCString(NS_LITERAL_CSTRING("[System Principal]"));
    return *aOrigin ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
nsSystemPrincipal::GetFingerprint(nsACString& aID)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP 
nsSystemPrincipal::GetPrettyName(nsACString& aName)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP 
nsSystemPrincipal::GetSubjectName(nsACString& aName)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsSystemPrincipal::GetCertificate(nsISupports** aCertificate)
{
    *aCertificate = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsSystemPrincipal::GetHasCertificate(PRBool* aResult)
{
    *aResult = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::GetCsp(nsIContentSecurityPolicy** aCsp)
{
  *aCsp = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::SetCsp(nsIContentSecurityPolicy* aCsp)
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::GetDomain(nsIURI** aDomain)
{
    *aDomain = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::SetDomain(nsIURI* aDomain)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::GetSecurityPolicy(void** aSecurityPolicy)
{
    *aSecurityPolicy = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::SetSecurityPolicy(void* aSecurityPolicy)
{
    return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::GetJSPrincipals(JSContext *cx, JSPrincipals **jsprin)
{
    NS_PRECONDITION(mJSPrincipals.nsIPrincipalPtr, "mJSPrincipals is uninitialized!");

    JSPRINCIPALS_HOLD(cx, &mJSPrincipals);
    *jsprin = &mJSPrincipals;
    return NS_OK;
}






NS_IMETHODIMP
nsSystemPrincipal::Read(nsIObjectInputStream* aStream)
{
    
    return NS_OK;
}

NS_IMETHODIMP
nsSystemPrincipal::Write(nsIObjectOutputStream* aStream)
{
    
    return NS_OK;
}





nsSystemPrincipal::nsSystemPrincipal()
{
}

#define SYSTEM_PRINCIPAL_SPEC "[System Principal]"

nsresult
nsSystemPrincipal::Init()
{
    
    
    nsCString str(SYSTEM_PRINCIPAL_SPEC);
    if (!str.EqualsLiteral(SYSTEM_PRINCIPAL_SPEC)) {
        NS_WARNING("Out of memory initializing system principal");
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    return mJSPrincipals.Init(this, str);
}

nsSystemPrincipal::~nsSystemPrincipal(void)
{
}
