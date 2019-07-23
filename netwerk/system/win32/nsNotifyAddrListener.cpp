





































#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winsock2.h>
#include <iprtrmib.h>
#include <time.h>
#include "prmem.h"
#include "plstr.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsNotifyAddrListener.h"
#include "nsString.h"

#include <iptypes.h>
#include <iphlpapi.h>

typedef DWORD (WINAPI *GetAdaptersAddressesFunc)(ULONG, DWORD, PVOID,
                                                 PIP_ADAPTER_ADDRESSES,
                                                 PULONG);
typedef DWORD (WINAPI *GetAdaptersInfoFunc)(PIP_ADAPTER_INFO, PULONG);
typedef DWORD (WINAPI *GetIfEntryFunc)(PMIB_IFROW);
typedef DWORD (WINAPI *GetIpAddrTableFunc)(PMIB_IPADDRTABLE, PULONG, BOOL);
typedef DWORD (WINAPI *NotifyAddrChangeFunc)(PHANDLE, LPOVERLAPPED);

static HMODULE sIPHelper;
static GetAdaptersAddressesFunc sGetAdaptersAddresses;
static GetAdaptersInfoFunc sGetAdaptersInfo;
static GetIfEntryFunc sGetIfEntry;
static GetIpAddrTableFunc sGetIpAddrTable;
static NotifyAddrChangeFunc sNotifyAddrChange;

static void InitIPHelperLibrary(void)
{
    if (sIPHelper)
        return;

    sIPHelper = LoadLibraryA("iphlpapi.dll");
    if (!sIPHelper)
        return;

    sGetAdaptersAddresses = (GetAdaptersAddressesFunc)
        GetProcAddress(sIPHelper, "GetAdaptersAddresses");
    sGetAdaptersInfo = (GetAdaptersInfoFunc)
        GetProcAddress(sIPHelper, "GetAdaptersInfo");
    sGetIfEntry = (GetIfEntryFunc)
        GetProcAddress(sIPHelper, "GetIfEntry");
    sGetIpAddrTable = (GetIpAddrTableFunc)
        GetProcAddress(sIPHelper, "GetIpAddrTable");
    sNotifyAddrChange = (NotifyAddrChangeFunc)
        GetProcAddress(sIPHelper, "NotifyAddrChange");
}

static void FreeIPHelperLibrary(void)
{
    if (!sIPHelper)
        return;

    sGetAdaptersAddresses = nsnull;
    sGetAdaptersInfo = nsnull;
    sGetIfEntry = nsnull;
    sGetIpAddrTable = nsnull;
    sNotifyAddrChange = nsnull;

    FreeLibrary(sIPHelper);
    sIPHelper = nsnull;
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsNotifyAddrListener,
                              nsINetworkLinkService,
                              nsIRunnable,
                              nsIObserver)

nsNotifyAddrListener::nsNotifyAddrListener()
    : mLinkUp(PR_TRUE)  
    , mStatusKnown(PR_FALSE)
    , mShutdownEvent(nsnull)
{
    mOSVerInfo.dwOSVersionInfoSize = sizeof(mOSVerInfo);
    GetVersionEx(&mOSVerInfo);
    InitIPHelperLibrary();
}

nsNotifyAddrListener::~nsNotifyAddrListener()
{
    NS_ASSERTION(!mThread, "nsNotifyAddrListener thread shutdown failed");
    FreeIPHelperLibrary();
}

NS_IMETHODIMP
nsNotifyAddrListener::GetIsLinkUp(PRBool *aIsUp)
{
    *aIsUp = mLinkUp;
    return NS_OK;
}

NS_IMETHODIMP
nsNotifyAddrListener::GetLinkStatusKnown(PRBool *aIsUp)
{
    *aIsUp = mStatusKnown;
    return NS_OK;
}

NS_IMETHODIMP
nsNotifyAddrListener::Run()
{
    HANDLE ev = CreateEvent(nsnull, FALSE, FALSE, nsnull);
    NS_ENSURE_TRUE(ev, NS_ERROR_OUT_OF_MEMORY);

    HANDLE handles[2] = { ev, mShutdownEvent };
    OVERLAPPED overlapped = { 0 };
    PRBool shuttingDown = PR_FALSE;

    overlapped.hEvent = ev;
    while (!shuttingDown) {
        HANDLE h;
        DWORD ret = sNotifyAddrChange(&h, &overlapped);

        if (ret == ERROR_IO_PENDING) {
            ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            if (ret == WAIT_OBJECT_0) {
                CheckLinkStatus();
            } else {
                shuttingDown = PR_TRUE;
            }
        } else {
            shuttingDown = PR_TRUE;
        }
    }
    CloseHandle(ev);

    return NS_OK;
}

NS_IMETHODIMP
nsNotifyAddrListener::Observe(nsISupports *subject,
                              const char *topic,
                              const PRUnichar *data)
{
    if (!strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID, topic))
        Shutdown();

    return NS_OK;
}

nsresult
nsNotifyAddrListener::Init(void)
{
    
    
    
    
    
    

    
    if (mOSVerInfo.dwPlatformId != VER_PLATFORM_WIN32_NT ||
        mOSVerInfo.dwMajorVersion < 5)
        return NS_OK;

    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                      PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    mShutdownEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    NS_ENSURE_TRUE(mShutdownEvent, NS_ERROR_OUT_OF_MEMORY);

    rv = NS_NewThread(getter_AddRefs(mThread), this);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
nsNotifyAddrListener::Shutdown(void)
{
    
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1");
    if (observerService)
        observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);

    if (!mShutdownEvent)
        return NS_OK;

    SetEvent(mShutdownEvent);

    nsresult rv = mThread->Shutdown();

    
    
    
    mThread = nsnull;

    CloseHandle(mShutdownEvent);
    mShutdownEvent = NULL;

    return rv;
}




nsresult
nsNotifyAddrListener::SendEventToUI(const char *aEventID)
{
    if (!aEventID)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    nsCOMPtr<nsIRunnable> event = new ChangeEvent(this, aEventID);
    if (NS_FAILED(rv = NS_DispatchToMainThread(event)))
        NS_WARNING("Failed to dispatch ChangeEvent");
    return rv;
}

NS_IMETHODIMP
nsNotifyAddrListener::ChangeEvent::Run()
{
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1");
    if (observerService)
        observerService->NotifyObservers(
                mService, NS_NETWORK_LINK_TOPIC,
                NS_ConvertASCIItoUTF16(mEventID).get());
    return NS_OK;
}

DWORD
nsNotifyAddrListener::GetOperationalStatus(DWORD aAdapterIndex)
{
    DWORD status = MIB_IF_OPER_STATUS_CONNECTED;

    
    
    if (mOSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        
        
        if (sGetIfEntry) {
            MIB_IFROW ifRow;

            ifRow.dwIndex = aAdapterIndex;
            if (sGetIfEntry(&ifRow) == ERROR_SUCCESS)
                status = ifRow.dwOperStatus;
        }
    }
    return status;
}







DWORD
nsNotifyAddrListener::CheckIPAddrTable(void)
{
    if (!sGetIpAddrTable)
        return ERROR_CALL_NOT_IMPLEMENTED;

    ULONG size = 0;
    DWORD ret = sGetIpAddrTable(nsnull, &size, FALSE);
    if (ret == ERROR_INSUFFICIENT_BUFFER && size > 0) {
        PMIB_IPADDRTABLE table = (PMIB_IPADDRTABLE) malloc(size);
        if (!table)
            return ERROR_OUTOFMEMORY;

        ret = sGetIpAddrTable(table, &size, FALSE);
        if (ret == ERROR_SUCCESS) {
            PRBool linkUp = PR_FALSE;

            for (DWORD i = 0; !linkUp && i < table->dwNumEntries; i++) {
                if (GetOperationalStatus(table->table[i].dwIndex) >=
                        MIB_IF_OPER_STATUS_CONNECTED &&
                        table->table[i].dwAddr != 0)
                    linkUp = PR_TRUE;
            }
            mLinkUp = linkUp;
        }
        free(table);
    }
    return ret;
}












DWORD
nsNotifyAddrListener::CheckAdaptersInfo(void)
{
    if (!sGetAdaptersInfo)
        return ERROR_NOT_SUPPORTED;

    ULONG adaptersLen = 0;

    DWORD ret = sGetAdaptersInfo(0, &adaptersLen);
    if (ret == ERROR_BUFFER_OVERFLOW && adaptersLen > 0) {
        PIP_ADAPTER_INFO adapters = (PIP_ADAPTER_INFO) malloc(adaptersLen);
        if (sGetAdaptersInfo(adapters, &adaptersLen) == ERROR_SUCCESS) {
            PRBool linkUp = PR_FALSE;
            PIP_ADAPTER_INFO ptr;

            for (ptr = adapters; ptr && !linkUp; ptr = ptr->Next) {
                if (GetOperationalStatus(ptr->Index) >=
                        MIB_IF_OPER_STATUS_CONNECTED) {
                    if (ptr->DhcpEnabled) {
                        if (PL_strcmp(ptr->DhcpServer.IpAddress.String,
                                      "255.255.255.255")) {
                            
                            
                            linkUp = PR_TRUE;
                        }
                    }
                    else {
                        PIP_ADDR_STRING ipAddr;
                        for (ipAddr = &ptr->IpAddressList; ipAddr && !linkUp;
                             ipAddr = ipAddr->Next)
                            if (PL_strcmp(ipAddr->IpAddress.String, "0.0.0.0"))
                                linkUp = PR_TRUE;
                    }
                }
            }
            mLinkUp = linkUp;
            mStatusKnown = PR_TRUE;
            free(adapters);
        }
    }
    return ret;
}

DWORD
nsNotifyAddrListener::CheckAdaptersAddresses(void)
{
    static const DWORD flags =
        GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_ANYCAST |
        GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    if (!sGetAdaptersAddresses)
        return ERROR_NOT_SUPPORTED;

    ULONG len = 0;

    DWORD ret = sGetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &len);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        PIP_ADAPTER_ADDRESSES addresses = (PIP_ADAPTER_ADDRESSES) malloc(len);
        if (addresses) {
            ret = sGetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &len);
            if (ret == ERROR_SUCCESS) {
                PIP_ADAPTER_ADDRESSES ptr;
                BOOL linkUp = FALSE;

                for (ptr = addresses; !linkUp && ptr; ptr = ptr->Next) {
                    if (ptr->OperStatus == IfOperStatusUp &&
                            ptr->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
                        linkUp = TRUE;
                }
                mLinkUp = linkUp;
                mStatusKnown = TRUE;
            }
            free(addresses);
        }
    }
    return ret;
}






void
nsNotifyAddrListener::CheckLinkStatus(void)
{
    DWORD ret;
    const char *event;

    ret = CheckAdaptersAddresses();
    if (ret == ERROR_NOT_SUPPORTED)
        ret = CheckAdaptersInfo();
    if (ret == ERROR_NOT_SUPPORTED)
        ret = CheckIPAddrTable();
    if (ret != ERROR_SUCCESS)
        mLinkUp = PR_TRUE; 

    if (mStatusKnown)
        event = mLinkUp ? NS_NETWORK_LINK_DATA_UP : NS_NETWORK_LINK_DATA_DOWN;
    else
        event = NS_NETWORK_LINK_DATA_UNKNOWN;
    SendEventToUI(event);
}
