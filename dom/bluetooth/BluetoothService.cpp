





#include "base/basictypes.h"

#include "BluetoothService.h"
#include "BluetoothTypes.h"
#include "BluetoothReplyRunnable.h"

#include "nsIDOMDOMRequest.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "nsObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/LazyIdleThread.h"
#include "mozilla/Util.h"

using namespace mozilla;

USING_BLUETOOTH_NAMESPACE

static nsRefPtr<BluetoothService> gBluetoothService;
static bool gInShutdown = false;

NS_IMPL_ISUPPORTS1(BluetoothService, nsIObserver)

class ToggleBtAck : public nsRunnable
{
public:
  ToggleBtAck(bool aEnabled) :
    mEnabled(aEnabled)
  {
  }
  
  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!mEnabled || gInShutdown) {
      if (gBluetoothService->mBluetoothCommandThread) {
        nsCOMPtr<nsIThread> t;
        gBluetoothService->mBluetoothCommandThread.swap(t);
        t->Shutdown();
      }
    }
    
    if (gInShutdown) {
      gBluetoothService = nullptr;
    }

    return NS_OK;
  }

  bool mEnabled;
};

class ToggleBtTask : public nsRunnable
{
public:
  ToggleBtTask(bool aEnabled,
               BluetoothReplyRunnable* aRunnable)
    : mEnabled(aEnabled),
      mRunnable(aRunnable)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run() 
  {
    MOZ_ASSERT(!NS_IsMainThread());

    nsString replyError;
    if (mEnabled) {
      if (NS_FAILED(gBluetoothService->StartInternal())) {
        replyError.AssignLiteral("Bluetooth service not available - We should never reach this point!");
      }
    }
    else {
      if (NS_FAILED(gBluetoothService->StopInternal())) {        
        replyError.AssignLiteral("Bluetooth service not available - We should never reach this point!");
      }
    }

    
    
    
    nsCOMPtr<nsIRunnable> ackTask = new ToggleBtAck(mEnabled);
    if (NS_FAILED(NS_DispatchToMainThread(ackTask))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
    
    if (!mRunnable) {
      return NS_OK;
    }
    
    
    BluetoothReply* reply;
    if (!replyError.IsEmpty()) {
      reply = new BluetoothReply(BluetoothReplyError(replyError));
    }
    else {
      reply = new BluetoothReply(BluetoothReplySuccess());
    }
    mRunnable->SetReply(reply);
      
    if (NS_FAILED(NS_DispatchToMainThread(mRunnable))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
    
    return NS_OK;
  }

private:
  bool mEnabled;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
};

nsresult
BluetoothService::RegisterBluetoothSignalHandler(const nsAString& aNodeName,
                                                 BluetoothSignalObserver* aHandler)
{
  MOZ_ASSERT(NS_IsMainThread());
  BluetoothSignalObserverList* ol;
  if (!mBluetoothSignalObserverTable.Get(aNodeName, &ol)) {
    ol = new BluetoothSignalObserverList();
    mBluetoothSignalObserverTable.Put(aNodeName, ol);
  }
  ol->AddObserver(aHandler);
  return NS_OK;
}

nsresult
BluetoothService::UnregisterBluetoothSignalHandler(const nsAString& aNodeName,
                                                   BluetoothSignalObserver* aHandler)
{
  MOZ_ASSERT(NS_IsMainThread());
  BluetoothSignalObserverList* ol;
  if (!mBluetoothSignalObserverTable.Get(aNodeName, &ol)) {
    NS_WARNING("Node does not exist to remove BluetoothSignalListener from!");
    return NS_OK;
  }
  ol->RemoveObserver(aHandler);
  if (ol->Length() == 0) {
    mBluetoothSignalObserverTable.Remove(aNodeName);
  }
  return NS_OK;
}

nsresult
BluetoothService::DistributeSignal(const BluetoothSignal& signal)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  BluetoothSignalObserverList* ol;
  if (!mBluetoothSignalObserverTable.Get(signal.path(), &ol)) {
    return NS_OK;
  }
  ol->Broadcast(signal);
  return NS_OK;
}

nsresult
BluetoothService::StartStopBluetooth(BluetoothReplyRunnable* aResultRunnable,
                                     bool aStart)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (gInShutdown && aStart) {
    NS_ERROR("Start called while in shutdown!");
    return NS_ERROR_FAILURE;
  }
  if (!mBluetoothCommandThread) {
    nsresult rv = NS_NewNamedThread("BluetoothCmd",
                                    getter_AddRefs(mBluetoothCommandThread));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  nsCOMPtr<nsIRunnable> r = new ToggleBtTask(aStart, aResultRunnable);
  if (NS_FAILED(mBluetoothCommandThread->Dispatch(r, NS_DISPATCH_NORMAL))) {
    NS_WARNING("Cannot dispatch firmware loading task!");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
BluetoothService::Start(BluetoothReplyRunnable* aResultRunnable)
{
  return StartStopBluetooth(aResultRunnable, true);
}

nsresult
BluetoothService::Stop(BluetoothReplyRunnable* aResultRunnable)
{
  return StartStopBluetooth(aResultRunnable, false);
}


BluetoothService*
BluetoothService::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (gBluetoothService) {
    return gBluetoothService;
  }
  
  
  if (gInShutdown) {
    NS_WARNING("BluetoothService returns null during shutdown");
    return nullptr;
  }

  
  gBluetoothService = BluetoothService::Create();
  if (!gBluetoothService) {
    NS_WARNING("Cannot create bluetooth service!");
    return nullptr;
  }
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  MOZ_ASSERT(obs);

  if (NS_FAILED(obs->AddObserver(gBluetoothService, "xpcom-shutdown", false))) {
    NS_ERROR("AddObserver failed!");
  }
  return gBluetoothService;
}

nsresult
BluetoothService::Observe(nsISupports* aSubject, const char* aTopic,
                          const PRUnichar* aData)
{
  NS_ASSERTION(!strcmp(aTopic, "xpcom-shutdown"),
               "BluetoothService got unexpected topic!");
  gInShutdown = true;
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (obs && NS_FAILED(obs->RemoveObserver(this, "xpcom-shutdown"))) {
    NS_WARNING("Can't unregister bluetooth service with xpcom shutdown!");
  }

  return Stop(nullptr);
}
