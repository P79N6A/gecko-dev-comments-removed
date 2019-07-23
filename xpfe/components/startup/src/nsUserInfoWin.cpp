






































#include "nsUserInfo.h"
#include "nsString.h"
#include "windows.h"
#include "nsCRT.h"
#include "nsXPIDLString.h"

nsUserInfo::nsUserInfo()
{
}

nsUserInfo::~nsUserInfo()
{
}

NS_IMPL_ISUPPORTS1(nsUserInfo,nsIUserInfo)

NS_IMETHODIMP
nsUserInfo::GetUsername(char **aUsername)
{
    *aUsername = nsnull;

    TCHAR username[256];
    DWORD size = 256;

    if (!GetUserName(username, &size))
        return NS_ERROR_FAILURE;
    
    *aUsername = nsCRT::strdup(username);
    
    if (*aUsername) return NS_OK;

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsUserInfo::GetFullname(PRUnichar **aFullname)
{
    *aFullname = nsnull;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsUserInfo::GetDomain(char * *aDomain)
{ 
    *aDomain = nsnull;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsUserInfo::GetEmailAddress(char * *aEmailAddress)
{
    *aEmailAddress = nsnull;
    return NS_ERROR_NOT_IMPLEMENTED;
}
