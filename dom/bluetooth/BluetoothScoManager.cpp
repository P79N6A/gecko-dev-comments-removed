





#include "base/basictypes.h"

#include "BluetoothScoManager.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothServiceUuid.h"

#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "nsContentUtils.h"
#include "nsIDOMDOMRequest.h"
#include "nsIObserverService.h"
#include "nsISystemMessagesInternal.h"
#include "nsVariant.h"

using namespace mozilla;
using namespace mozilla::ipc;
USING_BLUETOOTH_NAMESPACE

class mozilla::dom::bluetooth::BluetoothScoManagerObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  BluetoothScoManagerObserver()
  {
  }

  bool Init()
  {
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    MOZ_ASSERT(obs);

    if (NS_FAILED(obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false))) {
      NS_WARNING("Failed to add shutdown observer!");
      return false;
    }
    return true;
  }

  bool Shutdown()
  {
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (!obs ||
        (NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID)))) {
      NS_WARNING("Can't unregister observers!");
      return false;
    }
    return true;
  }

  ~BluetoothScoManagerObserver()
  {
    Shutdown();
  }
};

NS_IMPL_ISUPPORTS1(BluetoothScoManagerObserver, nsIObserver)

namespace {
StaticRefPtr<BluetoothScoManager> gBluetoothScoManager;
StaticRefPtr<BluetoothScoManagerObserver> sScoObserver;
bool gInShutdown = false;
} 

NS_IMETHODIMP
BluetoothScoManagerObserver::Observe(nsISupports* aSubject,
                                     const char* aTopic,
                                     const PRUnichar* aData)
{
  MOZ_ASSERT(gBluetoothScoManager);
  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {    
    return gBluetoothScoManager->HandleShutdown();
  }

  MOZ_ASSERT(false, "BluetoothScoManager got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}

BluetoothScoManager::BluetoothScoManager()
  : mConnected(false)
{
}

bool
BluetoothScoManager::Init()
{
  sScoObserver = new BluetoothScoManagerObserver();
  if (sScoObserver->Init()) {
    NS_WARNING("Cannot set up SCO observers!");
  }
  return true;
}

BluetoothScoManager::~BluetoothScoManager()
{
  Cleanup();
}

void
BluetoothScoManager::Cleanup()
{
  sScoObserver->Shutdown();
  sScoObserver = nullptr;
}


BluetoothScoManager*
BluetoothScoManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (gBluetoothScoManager) {
    return gBluetoothScoManager;
  }

  
  if (gInShutdown) {
    NS_WARNING("BluetoothScoManager can't be created during shutdown");
    return nullptr;
  }

  
  nsRefPtr<BluetoothScoManager> manager = new BluetoothScoManager();
  NS_ENSURE_TRUE(manager, nullptr);

  if (!manager->Init()) {
    return nullptr;
  }

  gBluetoothScoManager = manager;
  return gBluetoothScoManager;
}


void
BluetoothScoManager::ReceiveSocketData(mozilla::ipc::UnixSocketRawData* aMessage)
{
  
  MOZ_NOT_REACHED("This should never be called!");
}

nsresult
BluetoothScoManager::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  gInShutdown = true;
  CloseSocket();
  gBluetoothScoManager = nullptr;
  return NS_OK;
}

bool
BluetoothScoManager::Connect(const nsAString& aDeviceObjectPath)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (gInShutdown) {
    MOZ_ASSERT(false, "Connect called while in shutdown!");
    return false;
  }

  if (mConnected) {
    NS_WARNING("Sco socket has been ready");
    return true;
  }

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    NS_WARNING("BluetoothService not available!");
    return false;
  }

  nsresult rv = bs->GetScoSocket(aDeviceObjectPath,
                                 true,
                                 false,
                                 this);

  return NS_FAILED(rv) ? false : true;
}

void
BluetoothScoManager::Disconnect()
{
  CloseSocket();
  mConnected = false;
}


bool
BluetoothScoManager::GetConnected()
{
  return mConnected;
}

void
BluetoothScoManager::SetConnected(bool aConnected)
{
  mConnected = aConnected;
}

void
BluetoothScoManager::OnConnectSuccess()
{
}

void
BluetoothScoManager::OnConnectError()
{
}

void
BluetoothScoManager::OnDisconnect()
{
}
