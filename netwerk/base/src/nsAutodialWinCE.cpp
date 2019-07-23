




































#include <objbase.h>

#ifdef WINCE_WINDOWS_MOBILE
#include <connmgr.h>
#endif

#include "nsAutodialWinCE.h"

#include "nsString.h"
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

nsresult nsAutodial::DialDefault(const PRUnichar* hostName)
{
#ifdef WINCE_WINDOWS_MOBILE
  HANDLE connectionHandle;

  
  nsString theURL;
  theURL.Append(L"http://");
  theURL.Append(hostName);
  
  GUID networkUID;
  ConnMgrMapURL(theURL.get(), &networkUID, 0);

  
  CONNMGR_CONNECTIONINFO conn_info;
  memset(&conn_info, 0, sizeof(conn_info));
  
  conn_info.cbSize      = sizeof(conn_info);
  conn_info.dwParams    = CONNMGR_PARAM_GUIDDESTNET;
  conn_info.dwPriority  = CONNMGR_PRIORITY_USERINTERACTIVE;
  conn_info.guidDestNet = networkUID;
  conn_info.bExclusive  = FALSE;
  conn_info.bDisabled   = FALSE;
  conn_info.dwFlags     = CONNMGR_FLAG_PROXY_HTTP | CONNMGR_FLAG_PROXY_WAP | 
                          CONNMGR_FLAG_PROXY_SOCKS4 | CONNMGR_FLAG_PROXY_SOCKS5 |
                          CONNMGR_FLAG_NO_ERROR_MSGS;

  DWORD status;
  HRESULT result = ConnMgrEstablishConnectionSync(&conn_info, 
                                                  &connectionHandle, 
                                                  60000,
                                                  &status);
  
  if (conn_info.guidDestNet != autodial_DestNetInternet &&
      (result != S_OK || status != CONNMGR_STATUS_CONNECTED)) {
    conn_info.guidDestNet = autodial_DestNetInternet;  
    result = ConnMgrEstablishConnectionSync(&conn_info, 
                                            &connectionHandle, 
                                            60000,
                                            &status);
  }
  
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
