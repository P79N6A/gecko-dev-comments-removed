






































#include "nsCNullSecurityContext.h"
#include "nsString.h"

NS_IMPL_ISUPPORTS1(nsCNullSecurityContext, nsISecurityContext)

NS_IMETHODIMP nsCNullSecurityContext::Implies(const char* target, const char* action, PRBool* bActionAllowed)
{    
    
    *bActionAllowed = PR_TRUE;
    return NS_OK;	
}
   
NS_IMETHODIMP nsCNullSecurityContext::GetOrigin(char* buf, int len)
{
    if (buf == NULL)
       return NS_ERROR_NULL_POINTER;
    
    const char origin[] = "file:///";
    PRInt32 originLen = (PRInt32) (sizeof(origin) - 1);
    if (len <= originLen) {
        return NS_ERROR_NULL_POINTER;
    }
    
    memcpy(buf, origin, originLen);
    return NS_OK;
}

NS_IMETHODIMP nsCNullSecurityContext::GetCertificateID(char* buf, int len)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}
