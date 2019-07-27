





#ifndef mozilla_dom_bluetooth_bluetoothgatt_h__
#define mozilla_dom_bluetooth_bluetoothgatt_h__

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/BluetoothGattBinding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/dom/bluetooth/BluetoothGattService.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
class Promise;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;
class BluetoothService;
class BluetoothSignal;
class BluetoothValue;

class BluetoothGatt final : public DOMEventTargetHelper
                          , public BluetoothSignalObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothGatt, DOMEventTargetHelper)

  


  BluetoothConnectionState ConnectionState() const
  {
    return mConnectionState;
  }

  void GetServices(nsTArray<nsRefPtr<BluetoothGattService>>& aServices) const
  {
    aServices = mServices;
  }

  


  IMPL_EVENT_HANDLER(connectionstatechanged);

  


  already_AddRefed<Promise> Connect(ErrorResult& aRv);
  already_AddRefed<Promise> Disconnect(ErrorResult& aRv);
  already_AddRefed<Promise> DiscoverServices(ErrorResult& aRv);
  already_AddRefed<Promise> ReadRemoteRssi(ErrorResult& aRv);

  


  void Notify(const BluetoothSignal& aParam); 

  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;
  virtual void DisconnectFromOwner() override;

  BluetoothGatt(nsPIDOMWindow* aOwner,
                const nsAString& aDeviceAddr);

private:
  ~BluetoothGatt();

  





  void UpdateConnectionState(BluetoothConnectionState aState);

  




  void GenerateUuid(nsAString &aUuidString);

  






  void HandleServicesDiscovered(const BluetoothValue& aValue);

  


  


  nsString mAppUuid;

  



  int mClientIf;

  


  BluetoothConnectionState mConnectionState;

  


  nsString mDeviceAddr;

  


  nsTArray<nsRefPtr<BluetoothGattService>> mServices;

  


  bool mDiscoveringServices;
};

END_BLUETOOTH_NAMESPACE

#endif
