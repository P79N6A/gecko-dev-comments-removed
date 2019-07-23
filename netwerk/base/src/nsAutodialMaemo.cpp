




































#include "nsAutodialMaemo.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"
#include "nsMaemoNetworkManager.h"


nsAutodial::nsAutodial()
{
}

nsAutodial::~nsAutodial()
{
}

nsresult
nsAutodial::Init()
{
  return NS_OK;
}

nsresult
nsAutodial::DialDefault(const PRUnichar* hostName)
{
  if (nsMaemoNetworkManager::OpenConnectionSync())
    return NS_OK;

  return NS_ERROR_FAILURE;
}

PRBool
nsAutodial::ShouldDialOnNetworkError()
{
  if (nsMaemoNetworkManager::IsConnected())
    return PR_FALSE;

  return PR_TRUE;
}
