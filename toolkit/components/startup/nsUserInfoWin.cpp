




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
    *aUsername = nullptr;

    PRUnichar username[256];
    DWORD size = 256;

    if (!GetUserNameW(username, &size))
        return NS_ERROR_FAILURE;

    *aUsername = ToNewUTF8String(nsDependentString(username));

    return (*aUsername) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsUserInfo::GetFullname(PRUnichar **aFullname)
{
    *aFullname = nullptr;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsUserInfo::GetDomain(char * *aDomain)
{ 
    *aDomain = nullptr;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsUserInfo::GetEmailAddress(char * *aEmailAddress)
{
    *aEmailAddress = nullptr;
    return NS_ERROR_NOT_IMPLEMENTED;
}
