





#ifndef mozilla_dom_bluetooth_bluetoothdiscoveryhandle_h
#define mozilla_dom_bluetooth_bluetoothdiscoveryhandle_h

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/bluetooth/BluetoothAdapter.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "nsISupportsImpl.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothDevice;

class BluetoothDiscoveryHandle final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothDiscoveryHandle,
                                           DOMEventTargetHelper)

  static already_AddRefed<BluetoothDiscoveryHandle>
    Create(nsPIDOMWindow* aWindow);

  static already_AddRefed<BluetoothDiscoveryHandle>
    Create(nsPIDOMWindow* aWindow,
           const nsTArray<nsString>& aServiceUuids,
           const nsAString& aLeScanUuid,
           BluetoothAdapter* aAdapter);

  void DispatchDeviceEvent(BluetoothDevice* aDevice);

  void DispatchLeDeviceEvent(BluetoothDevice* aLeDevice,
                             int32_t aRssi,
                             nsTArray<uint8_t>& aScanRecord);

  IMPL_EVENT_HANDLER(devicefound);

  void GetLeScanUuid(nsString& aLeScanUuid) const
  {
    aLeScanUuid = mLeScanUuid;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  virtual void DisconnectFromOwner() override;

private:
  BluetoothDiscoveryHandle(nsPIDOMWindow* aWindow);

  BluetoothDiscoveryHandle(nsPIDOMWindow* aWindow,
                           const nsTArray<nsString>& aServiceUuids,
                           const nsAString& aLeScanUuid,
                           BluetoothAdapter* aAdapter);

  ~BluetoothDiscoveryHandle();

  






  nsString mLeScanUuid;

  





  nsTArray<nsString> mServiceUuids;

  





  nsRefPtr<BluetoothAdapter> mAdapter;
};

END_BLUETOOTH_NAMESPACE

#endif 
