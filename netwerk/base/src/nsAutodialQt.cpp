




































#include "nsQtNetworkManager.h"
#include "nsAutodialQt.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"


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
  if (nsQtNetworkManager::get()->openConnection(QString::fromUtf16(hostName))) {
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

PRBool
nsAutodial::ShouldDialOnNetworkError()
{
  if (nsQtNetworkManager::get()->isOnline()) {
    return PR_FALSE;
  }

  return PR_TRUE;
}
