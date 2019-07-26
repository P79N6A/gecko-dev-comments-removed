




#include "nsUserInfo.h"

#include "mozilla/Util.h" 
#include "nsString.h"
#include "windows.h"
#include "nsCRT.h"
#include "nsXPIDLString.h"

#define SECURITY_WIN32
#include "lm.h"
#include "security.h"

nsUserInfo::nsUserInfo()
{
}

nsUserInfo::~nsUserInfo()
{
}

NS_IMPL_ISUPPORTS1(nsUserInfo, nsIUserInfo)

NS_IMETHODIMP
nsUserInfo::GetUsername(char **aUsername)
{
  NS_ENSURE_ARG_POINTER(aUsername);
  *aUsername = nullptr;

  
  PRUnichar username[UNLEN +1];
  DWORD size = mozilla::ArrayLength(username);
  if (!GetUserNameW(username, &size))
    return NS_ERROR_FAILURE;

  *aUsername = ToNewUTF8String(nsDependentString(username));
  return (*aUsername) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsUserInfo::GetFullname(PRUnichar **aFullname)
{
  NS_ENSURE_ARG_POINTER(aFullname);
  *aFullname = nullptr;

  PRUnichar fullName[512];
  DWORD size = mozilla::ArrayLength(fullName);

  if (GetUserNameExW(NameDisplay, fullName, &size)) {
    *aFullname = ToNewUnicode(nsDependentString(fullName));
  } else {
    DWORD getUsernameError = GetLastError();

    
    
    PRUnichar username[UNLEN + 1];
    size = mozilla::ArrayLength(username);
    if (!GetUserNameW(username, &size)) {
      
      return getUsernameError == ERROR_NONE_MAPPED ?
             NS_ERROR_NOT_AVAILABLE : NS_ERROR_FAILURE;
    }

    const DWORD level = 2;
    LPBYTE info;
    
    
    NET_API_STATUS status = NetUserGetInfo(nullptr, username, level, &info);
    if (status != NERR_Success) {
      
      
      return getUsernameError == ERROR_NONE_MAPPED ?
             NS_ERROR_NOT_AVAILABLE : NS_ERROR_FAILURE;
    }

    nsDependentString fullName =
      nsDependentString(reinterpret_cast<USER_INFO_2 *>(info)->usri2_full_name);

    
    if (fullName.Length() == 0) {
      NetApiBufferFree(info);
      return NS_ERROR_NOT_AVAILABLE;
    }

    *aFullname = ToNewUnicode(fullName);
    NetApiBufferFree(info);
  }

  return (*aFullname) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsUserInfo::GetDomain(char **aDomain)
{
  NS_ENSURE_ARG_POINTER(aDomain);
  *aDomain = nullptr;

  const DWORD level = 100;
  LPBYTE info;
  NET_API_STATUS status = NetWkstaGetInfo(NULL, level, &info);
  if (status == NERR_Success) {
    *aDomain =
      ToNewUTF8String(nsDependentString(reinterpret_cast<WKSTA_INFO_100 *>(info)->
                                        wki100_langroup));
    NetApiBufferFree(info);
  }

  return (*aDomain) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsUserInfo::GetEmailAddress(char **aEmailAddress)
{
  NS_ENSURE_ARG_POINTER(aEmailAddress);
  *aEmailAddress = nullptr;

  
  PRUnichar emailAddress[255];
  DWORD size = mozilla::ArrayLength(emailAddress);

  if (!GetUserNameExW(NameUserPrincipal, emailAddress, &size)) {
    DWORD getUsernameError = GetLastError();
    return getUsernameError == ERROR_NONE_MAPPED ?
           NS_ERROR_NOT_AVAILABLE : NS_ERROR_FAILURE;
  }

  *aEmailAddress = ToNewUTF8String(nsDependentString(emailAddress));
  return (*aEmailAddress) ? NS_OK : NS_ERROR_FAILURE;
}
