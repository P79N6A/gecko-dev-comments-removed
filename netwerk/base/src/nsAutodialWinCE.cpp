




































#include <objbase.h>
#include <connmgr.h>

#include "nsAutodialWinCE.h"

#include "nsCOMPtr.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"



static const GUID ras_DestNetInternet =
        { 0x436ef144, 0xb4fb, 0x4863, { 0xa0, 0x41, 0x8f, 0x90, 0x5a, 0x62, 0xc5, 0x72 } };

nsRASAutodial::nsRASAutodial()
{
}

nsRASAutodial::~nsRASAutodial()
{
}

nsresult
nsRASAutodial::Init()
{
  return NS_OK;
}

nsresult nsRASAutodial::DialDefault(const PRUnichar* )
{
  HANDLE connectionHandle;

  
  CONNMGR_CONNECTIONINFO conn_info;
  memset(&conn_info, 0, sizeof(conn_info));
  
  conn_info.cbSize      = sizeof(conn_info);
  conn_info.dwParams    = CONNMGR_PARAM_GUIDDESTNET;
  conn_info.dwPriority  = CONNMGR_PRIORITY_USERINTERACTIVE;
  conn_info.guidDestNet = ras_DestNetInternet;
  conn_info.bExclusive  = FALSE;
  conn_info.bDisabled   = FALSE;
  
  DWORD status;
  HRESULT result = ConnMgrEstablishConnectionSync(&conn_info, 
						  &connectionHandle, 
						  60000,
						  &status);
  if (result != S_OK)
    return NS_ERROR_FAILURE;

  PRInt32 defaultCacheTime = 1;    
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    PRInt32 t;
    if (NS_SUCCEEDED(prefs->GetIntPref("network.autodial.cacheTime", &t)))
	defaultCacheTime = t;
  }

  ConnMgrReleaseConnection(connectionHandle, defaultCacheTime);
  
  if (status != CONNMGR_STATUS_CONNECTED)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

PRBool
nsRASAutodial::ShouldDialOnNetworkError()
{
  return PR_TRUE;
}
