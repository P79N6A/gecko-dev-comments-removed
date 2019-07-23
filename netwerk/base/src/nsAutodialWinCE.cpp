




































#include <objbase.h>

#ifdef WINCE_WINDOWS_MOBILE
#include <connmgr.h>
#endif

#include "nsAutodialWinCE.h"

#include "nsCOMPtr.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"


static const GUID autodial_DestNetInternet =
        { 0x436ef144, 0xb4fb, 0x4863, { 0xa0, 0x41, 0x8f, 0x90, 0x5a, 0x62, 0xc5, 0x72 } };

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

nsresult nsAutodial::DialDefault(const PRUnichar* )
{
#ifdef WINCE_WINDOWS_MOBILE
  HANDLE connectionHandle;

  
  CONNMGR_CONNECTIONINFO conn_info;
  memset(&conn_info, 0, sizeof(conn_info));
  
  conn_info.cbSize      = sizeof(conn_info);
  conn_info.dwParams    = CONNMGR_PARAM_GUIDDESTNET;
  conn_info.dwPriority  = CONNMGR_PRIORITY_USERINTERACTIVE;
  conn_info.guidDestNet = autodial_DestNetInternet;
  conn_info.bExclusive  = FALSE;
  conn_info.bDisabled   = FALSE;
  
  DWORD status;
  HRESULT result = ConnMgrEstablishConnectionSync(&conn_info, 
                                                  &connectionHandle, 
                                                  60000,
                                                  &status);

  if (result != S_OK || status != CONNMGR_STATUS_CONNECTED)
    return NS_ERROR_FAILURE;

  return NS_OK;
#else
  return NS_ERROR_FAILURE;
#endif
}

PRBool
nsAutodial::ShouldDialOnNetworkError()
{
#ifdef WINCE_WINDOWS_MOBILE
  return PR_TRUE;
#else
  return PR_FALSE;
#endif
}
