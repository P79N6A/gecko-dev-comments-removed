




































#include "nsUserInfo.h"
#include "nsString.h"
#include "nsReadableUtils.h"

#include "nsIServiceManager.h"
#include "nsIInternetConfigService.h"

nsUserInfo::nsUserInfo()
{
}

nsUserInfo::~nsUserInfo()
{
}

NS_IMPL_ISUPPORTS1(nsUserInfo,nsIUserInfo)

NS_IMETHODIMP
nsUserInfo::GetFullname(PRUnichar **aFullname)
{
  nsresult result = NS_ERROR_FAILURE;
  nsCOMPtr<nsIInternetConfigService> icService (do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID));
  if (icService)
  {
	  nsCAutoString cName;
    result = icService->GetString(nsIInternetConfigService::eICString_RealName, cName);
    if ( NS_SUCCEEDED ( result ) )
    {
  	  nsString fullName;
      *aFullname = ToNewUnicode(cName);
    }
  }
  return result;

}

NS_IMETHODIMP 
nsUserInfo::GetEmailAddress(char * *aEmailAddress)
{
  nsresult result = NS_ERROR_FAILURE;
  nsCOMPtr<nsIInternetConfigService> icService (do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID));
  if (icService)
  {
    nsCAutoString tempString;
    result = icService->GetString(nsIInternetConfigService::eICString_Email, tempString);
    if (NS_SUCCEEDED(result))
      *aEmailAddress = ToNewCString(tempString);  

  }
  return result;
}

NS_IMETHODIMP 
nsUserInfo::GetUsername(char * *aUsername)
{
  *aUsername = nsnull;
  
  nsCAutoString   tempString;
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIInternetConfigService> icService (do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID));
  if (icService)
    rv = icService->GetString(nsIInternetConfigService::eICString_Email, tempString);

	if ( NS_FAILED( rv ) ) return rv;
	
  const char*     atString = "@";
  PRInt32         atOffset = tempString.Find(atString);
  if (atOffset != kNotFound)
    tempString.Truncate(atOffset);
      
  *aUsername = ToNewCString(tempString);  
  return NS_OK;
}

NS_IMETHODIMP 
nsUserInfo::GetDomain(char * *aDomain)
{
  *aDomain = nsnull;
  nsCAutoString   tempString;
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIInternetConfigService> icService (do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID));
  if (icService)
    rv = icService->GetString(nsIInternetConfigService::eICString_Email, tempString);
	if ( NS_FAILED( rv ) ) return rv;
  const char*     atString = "@";
  PRInt32         atOffset = tempString.Find(atString);
  if (atOffset != kNotFound)
  {
    nsCAutoString domainString;
    tempString.Right(domainString, tempString.Length() - (atOffset + 1));
    *aDomain = ToNewCString(domainString);
    return NS_OK;
  }

  
  return NS_ERROR_FAILURE;
}

#pragma mark -






