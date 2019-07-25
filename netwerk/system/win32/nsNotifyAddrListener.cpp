





































#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <ole2.h>
#include <netcon.h>
#include <objbase.h>
#include <iprtrmib.h>
#include "prmem.h"
#include "plstr.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsNotifyAddrListener.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/Services.h"
#include "nsCRT.h"

#include <iptypes.h>
#include <iphlpapi.h>

typedef void (WINAPI *NcFreeNetconPropertiesFunc)(NETCON_PROPERTIES*);

static HMODULE sNetshell;
static NcFreeNetconPropertiesFunc sNcFreeNetconProperties;

static void InitNetshellLibrary(void)
{
    if (!sNetshell) {
        sNetshell = LoadLibraryW(L"Netshell.dll");
        if (sNetshell) {
            sNcFreeNetconProperties = (NcFreeNetconPropertiesFunc)
                GetProcAddress(sNetshell, "NcFreeNetconProperties");
        }
    }
}

static void FreeDynamicLibraries(void)
{
    if (sNetshell) {
        sNcFreeNetconProperties = nsnull;
        FreeLibrary(sNetshell);
        sNetshell = nsnull;
    }
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsNotifyAddrListener,
                              nsINetworkLinkService,
                              nsIRunnable,
                              nsIObserver)

nsNotifyAddrListener::nsNotifyAddrListener()
    : mLinkUp(true)  
    , mStatusKnown(false)
    , mCheckAttempted(false)
    , mShutdownEvent(nsnull)
{
}

nsNotifyAddrListener::~nsNotifyAddrListener()
{
    NS_ASSERTION(!mThread, "nsNotifyAddrListener thread shutdown failed");
    FreeDynamicLibraries();
}

NS_IMETHODIMP
nsNotifyAddrListener::GetIsLinkUp(bool *aIsUp)
{
    if (!mCheckAttempted && !mStatusKnown) {
        mCheckAttempted = true;
        CheckLinkStatus();
    }

    *aIsUp = mLinkUp;
    return NS_OK;
}

NS_IMETHODIMP
nsNotifyAddrListener::GetLinkStatusKnown(bool *aIsUp)
{
    *aIsUp = mStatusKnown;
    return NS_OK;
}

NS_IMETHODIMP
nsNotifyAddrListener::GetLinkType(PRUint32 *aLinkType)
{
  NS_ENSURE_ARG_POINTER(aLinkType);

  
  *aLinkType = nsINetworkLinkService::LINK_TYPE_UNKNOWN;
  return NS_OK;
}

NS_IMETHODIMP
nsNotifyAddrListener::Run()
{
    HANDLE ev = CreateEvent(nsnull, FALSE, FALSE, nsnull);
    NS_ENSURE_TRUE(ev, NS_ERROR_OUT_OF_MEMORY);

    HANDLE handles[2] = { ev, mShutdownEvent };
    OVERLAPPED overlapped = { 0 };
    bool shuttingDown = false;

    overlapped.hEvent = ev;
    while (!shuttingDown) {
        HANDLE h;
        DWORD ret = NotifyAddrChange(&h, &overlapped);

        if (ret == ERROR_IO_PENDING) {
            ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            if (ret == WAIT_OBJECT_0) {
                CheckLinkStatus();
            } else {
                shuttingDown = true;
            }
        } else {
            shuttingDown = true;
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
    if (!strcmp("xpcom-shutdown-threads", topic))
        Shutdown();

    return NS_OK;
}

nsresult
nsNotifyAddrListener::Init(void)
{
    
    
    
    
    
    

    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
    if (!observerService)
        return NS_ERROR_FAILURE;

    nsresult rv = observerService->AddObserver(this, "xpcom-shutdown-threads",
                                               false);
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
        mozilla::services::GetObserverService();
    if (observerService)
        observerService->RemoveObserver(this, "xpcom-shutdown-threads");

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
        mozilla::services::GetObserverService();
    if (observerService)
        observerService->NotifyObservers(
                mService, NS_NETWORK_LINK_TOPIC,
                NS_ConvertASCIItoUTF16(mEventID).get());
    return NS_OK;
}

BOOL
nsNotifyAddrListener::CheckIsGateway(PIP_ADAPTER_ADDRESSES aAdapter)
{
    if (!aAdapter->FirstUnicastAddress)
        return FALSE;

    LPSOCKADDR aAddress = aAdapter->FirstUnicastAddress->Address.lpSockaddr;
    if (!aAddress)
        return FALSE;

    PSOCKADDR_IN in_addr = (PSOCKADDR_IN)aAddress;
    bool isGateway = (aAddress->sa_family == AF_INET &&
        in_addr->sin_addr.S_un.S_un_b.s_b1 == 192 &&
        in_addr->sin_addr.S_un.S_un_b.s_b2 == 168 &&
        in_addr->sin_addr.S_un.S_un_b.s_b3 == 0 &&
        in_addr->sin_addr.S_un.S_un_b.s_b4 == 1);

    if (isGateway)
      isGateway = CheckICSStatus(aAdapter->FriendlyName);

    return isGateway;
}

BOOL
nsNotifyAddrListener::CheckICSStatus(PWCHAR aAdapterName)
{
    InitNetshellLibrary();

    
    
    
    BOOL isICSGatewayAdapter = FALSE;

    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return FALSE;

    nsRefPtr<INetSharingManager> netSharingManager;
    hr = CoCreateInstance(
                CLSID_NetSharingManager,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_INetSharingManager,
                getter_AddRefs(netSharingManager));

    nsRefPtr<INetSharingPrivateConnectionCollection> privateCollection;
    if (SUCCEEDED(hr)) {
        hr = netSharingManager->get_EnumPrivateConnections(
                    ICSSC_DEFAULT,
                    getter_AddRefs(privateCollection));
    }

    nsRefPtr<IEnumNetSharingPrivateConnection> privateEnum;
    if (SUCCEEDED(hr)) {
        nsRefPtr<IUnknown> privateEnumUnknown;
        hr = privateCollection->get__NewEnum(getter_AddRefs(privateEnumUnknown));
        if (SUCCEEDED(hr)) {
            hr = privateEnumUnknown->QueryInterface(
                        IID_IEnumNetSharingPrivateConnection,
                        getter_AddRefs(privateEnum));
        }
    }

    if (SUCCEEDED(hr)) {
        ULONG fetched;
        VARIANT connectionVariant;
        while (!isICSGatewayAdapter &&
               SUCCEEDED(hr = privateEnum->Next(1, &connectionVariant,
               &fetched)) &&
               fetched) {
            if (connectionVariant.vt != VT_UNKNOWN) {
                
                
                
                
                
                NS_ERROR("Variant of unexpected type, expecting VT_UNKNOWN, we probably leak it!");
                continue;
            }

            nsRefPtr<INetConnection> connection;
            hr = connectionVariant.punkVal->QueryInterface(
                        IID_INetConnection,
                        getter_AddRefs(connection));
            connectionVariant.punkVal->Release();

            NETCON_PROPERTIES *properties;
            if (SUCCEEDED(hr))
                hr = connection->GetProperties(&properties);

            if (SUCCEEDED(hr)) {
                if (!wcscmp(properties->pszwName, aAdapterName))
                    isICSGatewayAdapter = TRUE;

                if (sNcFreeNetconProperties)
                    sNcFreeNetconProperties(properties);
            }
        }
    }

    CoUninitialize();

    return isICSGatewayAdapter;
}

DWORD
nsNotifyAddrListener::CheckAdaptersAddresses(void)
{
    static const DWORD flags =
        GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_ANYCAST |
        GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    ULONG len = 16384;

    PIP_ADAPTER_ADDRESSES addresses = (PIP_ADAPTER_ADDRESSES) malloc(len);
    if (!addresses)
        return ERROR_OUTOFMEMORY;

    DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &len);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        free(addresses);
        addresses = (PIP_ADAPTER_ADDRESSES) malloc(len);
        if (!addresses)
            return ERROR_BUFFER_OVERFLOW;
        ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &len);
    }

    if (ret == ERROR_SUCCESS) {
        PIP_ADAPTER_ADDRESSES ptr;
        BOOL linkUp = FALSE;

        for (ptr = addresses; !linkUp && ptr; ptr = ptr->Next) {
            if (ptr->OperStatus == IfOperStatusUp &&
                    ptr->IfType != IF_TYPE_SOFTWARE_LOOPBACK &&
                    !CheckIsGateway(ptr))
                linkUp = TRUE;
        }
        mLinkUp = linkUp;
        mStatusKnown = TRUE;
    }
    free(addresses);

    return ret;
}






void
nsNotifyAddrListener::CheckLinkStatus(void)
{
    DWORD ret;
    const char *event;

    ret = CheckAdaptersAddresses();
    if (ret != ERROR_SUCCESS)
        mLinkUp = true; 

    if (mStatusKnown)
        event = mLinkUp ? NS_NETWORK_LINK_DATA_UP : NS_NETWORK_LINK_DATA_DOWN;
    else
        event = NS_NETWORK_LINK_DATA_UNKNOWN;
    SendEventToUI(event);
}
