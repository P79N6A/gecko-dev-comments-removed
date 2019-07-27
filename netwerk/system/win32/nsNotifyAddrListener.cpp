





#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <ole2.h>
#include <netcon.h>
#include <objbase.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <tcpmib.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <iprtrmib.h>
#include "plstr.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsNotifyAddrListener.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "mozilla/Services.h"
#include "nsCRT.h"
#include "mozilla/Preferences.h"

#include <iptypes.h>
#include <iphlpapi.h>

using namespace mozilla;

static HMODULE sNetshell;
static decltype(NcFreeNetconProperties)* sNcFreeNetconProperties;

static HMODULE sIphlpapi;
static decltype(NotifyIpInterfaceChange)* sNotifyIpInterfaceChange;
static decltype(CancelMibChangeNotify2)* sCancelMibChangeNotify2;

#define NETWORK_NOTIFY_CHANGED_PREF "network.notify.changed"

static void InitIphlpapi(void)
{
    if (!sIphlpapi) {
        sIphlpapi = LoadLibraryW(L"Iphlpapi.dll");
        if (sIphlpapi) {
            sNotifyIpInterfaceChange = (decltype(NotifyIpInterfaceChange)*)
                GetProcAddress(sIphlpapi, "NotifyIpInterfaceChange");
            sCancelMibChangeNotify2 = (decltype(CancelMibChangeNotify2)*)
                GetProcAddress(sIphlpapi, "CancelMibChangeNotify2");
        } else {
            NS_WARNING("Failed to load Iphlpapi.dll - cannot detect network"
                       " changes!");
        }
    }
}

static void InitNetshellLibrary(void)
{
    if (!sNetshell) {
        sNetshell = LoadLibraryW(L"Netshell.dll");
        if (sNetshell) {
            sNcFreeNetconProperties = (decltype(NcFreeNetconProperties)*)
                GetProcAddress(sNetshell, "NcFreeNetconProperties");
        }
    }
}

static void FreeDynamicLibraries(void)
{
    if (sNetshell) {
        sNcFreeNetconProperties = nullptr;
        FreeLibrary(sNetshell);
        sNetshell = nullptr;
    }
    if (sIphlpapi) {
        sNotifyIpInterfaceChange = nullptr;
        sCancelMibChangeNotify2 = nullptr;
        FreeLibrary(sIphlpapi);
        sIphlpapi = nullptr;
    }
}

NS_IMPL_ISUPPORTS(nsNotifyAddrListener,
                  nsINetworkLinkService,
                  nsIRunnable,
                  nsIObserver)

nsNotifyAddrListener::nsNotifyAddrListener()
    : mLinkUp(true)  
    , mStatusKnown(false)
    , mCheckAttempted(false)
    , mShutdownEvent(nullptr)
    , mAllowChangedEvent(true)
{
    InitIphlpapi();
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
nsNotifyAddrListener::GetLinkType(uint32_t *aLinkType)
{
  NS_ENSURE_ARG_POINTER(aLinkType);

  
  *aLinkType = nsINetworkLinkService::LINK_TYPE_UNKNOWN;
  return NS_OK;
}


static void WINAPI OnInterfaceChange(PVOID callerContext,
                                     PMIB_IPINTERFACE_ROW row,
                                     MIB_NOTIFICATION_TYPE notificationType)
{
    nsNotifyAddrListener *notify = static_cast<nsNotifyAddrListener*>(callerContext);
    notify->CheckLinkStatus();
}

NS_IMETHODIMP
nsNotifyAddrListener::Run()
{
    PR_SetCurrentThreadName("Link Monitor");
    if (!sNotifyIpInterfaceChange || !sCancelMibChangeNotify2) {
        
        
        HANDLE ev = CreateEvent(nullptr, FALSE, FALSE, nullptr);
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
    } else {
        
        HANDLE interfacechange;
        
        DWORD ret = sNotifyIpInterfaceChange(
            AF_UNSPEC, 
            (PIPINTERFACE_CHANGE_CALLBACK)OnInterfaceChange,
            this,  
            false, 
            &interfacechange);

        if (ret == NO_ERROR) {
            ret = WaitForSingleObject(mShutdownEvent, INFINITE);
        }
        sCancelMibChangeNotify2(interfacechange);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsNotifyAddrListener::Observe(nsISupports *subject,
                              const char *topic,
                              const char16_t *data)
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

    Preferences::AddBoolVarCache(&mAllowChangedEvent,
                                 NETWORK_NOTIFY_CHANGED_PREF, true);

    mShutdownEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
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

    
    
    
    mThread = nullptr;

    CloseHandle(mShutdownEvent);
    mShutdownEvent = nullptr;

    return rv;
}




nsresult
nsNotifyAddrListener::SendEvent(const char *aEventID)
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





bool
nsNotifyAddrListener::CheckICSGateway(PIP_ADAPTER_ADDRESSES aAdapter)
{
    if (!aAdapter->FirstUnicastAddress)
        return false;

    LPSOCKADDR aAddress = aAdapter->FirstUnicastAddress->Address.lpSockaddr;
    if (!aAddress)
        return false;

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

bool
nsNotifyAddrListener::CheckICSStatus(PWCHAR aAdapterName)
{
    InitNetshellLibrary();

    
    
    
    bool isICSGatewayAdapter = false;

    HRESULT hr;
    nsRefPtr<INetSharingManager> netSharingManager;
    hr = CoCreateInstance(
                CLSID_NetSharingManager,
                nullptr,
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
            if (SUCCEEDED(connectionVariant.punkVal->QueryInterface(
                              IID_INetConnection,
                              getter_AddRefs(connection)))) {
                connectionVariant.punkVal->Release();

                NETCON_PROPERTIES *properties;
                if (SUCCEEDED(connection->GetProperties(&properties))) {
                    if (!wcscmp(properties->pszwName, aAdapterName))
                        isICSGatewayAdapter = true;

                    if (sNcFreeNetconProperties)
                        sNcFreeNetconProperties(properties);
                }
            }
        }
    }

    return isICSGatewayAdapter;
}

DWORD
nsNotifyAddrListener::CheckAdaptersAddresses(void)
{
    ULONG len = 16384;

    PIP_ADAPTER_ADDRESSES adapterList = (PIP_ADAPTER_ADDRESSES) moz_xmalloc(len);

    ULONG flags = GAA_FLAG_SKIP_DNS_SERVER|GAA_FLAG_SKIP_MULTICAST|
        GAA_FLAG_SKIP_ANYCAST;

    DWORD ret = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapterList, &len);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        free(adapterList);
        adapterList = static_cast<PIP_ADAPTER_ADDRESSES> (moz_xmalloc(len));

        ret = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapterList, &len);
    }

    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
        free(adapterList);
        return ERROR_NOT_SUPPORTED;
    }

    
    
    
    
    
    
    ULONG sum = 0;

    if (ret == ERROR_SUCCESS) {
        bool linkUp = false;

        for (PIP_ADAPTER_ADDRESSES adapter = adapterList; adapter;
             adapter = adapter->Next) {
            if (adapter->OperStatus != IfOperStatusUp ||
                !adapter->FirstUnicastAddress ||
                adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||
                CheckICSGateway(adapter) ) {
                continue;
            }

            
            for (int i = 0; adapter->AdapterName[i]; ++i) {
                sum <<= 2;
                sum += adapter->AdapterName[i];
            }

            
            for (PIP_ADAPTER_UNICAST_ADDRESS pip = adapter->FirstUnicastAddress;
                 pip; pip = pip->Next) {
                SOCKET_ADDRESS *sockAddr = &pip->Address;
                for (int i = 0; i < sockAddr->iSockaddrLength; ++i) {
                    sum += (reinterpret_cast<unsigned char *>
                            (sockAddr->lpSockaddr))[i];
                }
            }
            linkUp = true;
        }
        mLinkUp = linkUp;
        mStatusKnown = true;
    }
    free(adapterList);

    if (mLinkUp) {
        
        mIPInterfaceChecksum = sum;
    }

    CoUninitialize();

    return ret;
}






void
nsNotifyAddrListener::CheckLinkStatus(void)
{
    DWORD ret;
    const char *event;
    bool prevLinkUp = mLinkUp;
    ULONG prevCsum = mIPInterfaceChecksum;

    
    
    
    
    if (NS_IsMainThread()) {
        NS_WARNING("CheckLinkStatus called on main thread! No check "
                   "performed. Assuming link is up, status is unknown.");
        mLinkUp = true;

        if (!mStatusKnown) {
            event = NS_NETWORK_LINK_DATA_UNKNOWN;
        } else if (!prevLinkUp) {
            event = NS_NETWORK_LINK_DATA_UP;
        } else {
            
            event = nullptr;
        }

        if (event) {
            SendEvent(event);
        }
    } else {
        ret = CheckAdaptersAddresses();
        if (ret != ERROR_SUCCESS) {
            mLinkUp = true;
        }

        if (mLinkUp && (prevCsum != mIPInterfaceChecksum)) {
            
            
            if (mAllowChangedEvent) {
                SendEvent(NS_NETWORK_LINK_DATA_CHANGED);
            }
        }
        if (prevLinkUp != mLinkUp) {
            
            SendEvent(mLinkUp ?
                      NS_NETWORK_LINK_DATA_UP : NS_NETWORK_LINK_DATA_DOWN);
        }
    }
}
