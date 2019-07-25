





#include "nsDOMClassInfo.h"
#include "nsDOMEvent.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "mozilla/LazyIdleThread.h"

#include "BluetoothAdapter.h"

#if defined(MOZ_WIDGET_GONK)
#include <bluedroid/bluetooth.h>
#endif

USING_BLUETOOTH_NAMESPACE

class ToggleBtResultTask : public nsRunnable
{
  public:
    ToggleBtResultTask(nsRefPtr<BluetoothAdapter>& adapterPtr, bool result)
      : mResult(result)
    {
      MOZ_ASSERT(!NS_IsMainThread());

      mAdapterPtr.swap(adapterPtr);
    }

    NS_IMETHOD Run() 
    {
      MOZ_ASSERT(NS_IsMainThread());

      if (!mResult) {
        
        NS_WARNING("BT firmware loading fails.\n");
      }
 
      
      
      mAdapterPtr->FirePowered();
      mAdapterPtr = nsnull;

      return NS_OK;
    }

  private:
    nsRefPtr<BluetoothAdapter> mAdapterPtr;
    bool mResult;
};

class ToggleBtTask : public nsRunnable
{
  public:
    ToggleBtTask(bool onOff, BluetoothAdapter* adapterPtr) 
      : mOnOff(onOff),
        mAdapterPtr(adapterPtr) 
    {
      MOZ_ASSERT(NS_IsMainThread());
    }

    NS_IMETHOD Run() 
    {
      MOZ_ASSERT(!NS_IsMainThread());

      bool result;

      
#if defined(MOZ_WIDGET_GONK)  
      if (mOnOff) {
        result = bt_enable();
      } else {
        result = bt_disable();
      }
#else 
      result = true;
#endif

      
      nsCOMPtr<nsIRunnable> resultRunnable = new ToggleBtResultTask(mAdapterPtr, result);

      if (NS_FAILED(NS_DispatchToMainThread(resultRunnable))) {
        NS_WARNING("Failed to dispatch to main thread!");
      }

      return NS_OK;
    }

  private:
    nsRefPtr<BluetoothAdapter> mAdapterPtr;
    bool mOnOff;
};

DOMCI_DATA(BluetoothAdapter, BluetoothAdapter)

NS_IMPL_CYCLE_COLLECTION_CLASS(BluetoothAdapter)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BluetoothAdapter, 
                                                  nsDOMEventTargetHelper)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(powered)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BluetoothAdapter, 
                                                nsDOMEventTargetHelper)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(powered)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothAdapter)
  NS_INTERFACE_MAP_ENTRY(nsIDOMBluetoothAdapter)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(BluetoothAdapter)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothAdapter, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothAdapter, nsDOMEventTargetHelper)

BluetoothAdapter::BluetoothAdapter() 
  : mPower(false)
{
}

NS_IMETHODIMP
BluetoothAdapter::GetPower(bool* aPower)
{
  *aPower = mPower;

  return NS_OK;
}

NS_IMETHODIMP
BluetoothAdapter::SetPower(bool aPower)
{
  if (mPower != aPower) {
    mPower = aPower;

    return ToggleBluetoothAsync();
  }

  return NS_OK;
}

nsresult
BluetoothAdapter::ToggleBluetoothAsync()
{
  if (!mToggleBtThread) {
    mToggleBtThread = new LazyIdleThread(15000);
  }

  nsCOMPtr<nsIRunnable> r = new ToggleBtTask(mPower, this);

  return mToggleBtThread->Dispatch(r, NS_DISPATCH_NORMAL);
}

nsresult
BluetoothAdapter::FirePowered()
{
  nsRefPtr<nsDOMEvent> event = new nsDOMEvent(nsnull, nsnull);
  nsresult rv = event->InitEvent(NS_LITERAL_STRING("powered"), false, false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = event->SetTrusted(true);
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy;
  rv = DispatchEvent(event, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_EVENT_HANDLER(BluetoothAdapter, powered)
