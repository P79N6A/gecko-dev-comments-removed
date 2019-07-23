



































#include "nsNotifyAddrListener.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsIObserverService.h"

#include <objbase.h>
#ifdef WINCE_WINDOWS_MOBILE
#include <connmgr.h>
#endif


static const GUID nal_DestNetInternet =
        { 0x436ef144, 0xb4fb, 0x4863, { 0xa0, 0x41, 0x8f, 0x90, 0x5a, 0x62, 0xc5, 0x72 } };


NS_IMPL_THREADSAFE_ISUPPORTS1(nsNotifyAddrListener,
                              nsINetworkLinkService)

nsNotifyAddrListener::nsNotifyAddrListener()
: mConnectionHandle(NULL)
{
}

nsNotifyAddrListener::~nsNotifyAddrListener()
{
#ifdef WINCE_WINDOWS_MOBILE
  if (mConnectionHandle)
    ConnMgrReleaseConnection(mConnectionHandle, 0);
#endif
}

nsresult
nsNotifyAddrListener::Init(void)
{
#ifdef WINCE_WINDOWS_MOBILE
  CONNMGR_CONNECTIONINFO conn_info;
  memset(&conn_info, 0, sizeof(conn_info));
  
  conn_info.cbSize      = sizeof(conn_info);
  conn_info.dwParams    = CONNMGR_PARAM_GUIDDESTNET;
  conn_info.dwPriority  = CONNMGR_PRIORITY_LOWBKGND;
  conn_info.guidDestNet = nal_DestNetInternet;
  conn_info.bExclusive  = FALSE;
  conn_info.bDisabled   = FALSE;
  
  ConnMgrEstablishConnection(&conn_info, 
                             &mConnectionHandle);

#endif
  return NS_OK;
}

NS_IMETHODIMP
nsNotifyAddrListener::GetIsLinkUp(PRBool *aIsUp)
{
#ifdef WINCE_WINDOWS_MOBILE
  DWORD status;
  HRESULT result = ConnMgrConnectionStatus(mConnectionHandle, &status);
  if (FAILED(result)) {
    *aIsUp = PR_FALSE;
    return NS_ERROR_FAILURE;
  }
  *aIsUp = status == CONNMGR_STATUS_CONNECTED;
#else
  *aIsUp = PR_TRUE;
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsNotifyAddrListener::GetLinkStatusKnown(PRBool *aIsKnown)
{
#ifdef WINCE_WINDOWS_MOBILE
  DWORD status;
  HRESULT result = ConnMgrConnectionStatus(mConnectionHandle, &status);
  if (FAILED(result)) {
    *aIsKnown = PR_FALSE;
    return NS_ERROR_FAILURE;
  }
  *aIsKnown = status != CONNMGR_STATUS_UNKNOWN;
#else
  *aIsKnown = PR_TRUE;
#endif
  return NS_OK;
}
