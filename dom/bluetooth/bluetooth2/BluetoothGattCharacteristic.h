





#ifndef mozilla_dom_bluetooth_bluetoothgattcharacteristic_h__
#define mozilla_dom_bluetooth_bluetoothgattcharacteristic_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/BluetoothGattCharacteristicBinding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/dom/bluetooth/BluetoothGattDescriptor.h"
#include "mozilla/dom/TypedArray.h"
#include "nsCOMPtr.h"
#include "nsWrapperCache.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {
class Promise;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothGattService;
class BluetoothSignal;
class BluetoothValue;

class BluetoothGattCharacteristic final : public nsISupports
                                        , public nsWrapperCache
                                        , public BluetoothSignalObserver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(BluetoothGattCharacteristic)

  


  BluetoothGattService* Service() const
  {
    return mService;
  }

  void GetDescriptors(
    nsTArray<nsRefPtr<BluetoothGattDescriptor>>& aDescriptors) const
  {
    aDescriptors = mDescriptors;
  }

  void GetUuid(nsString& aUuidStr) const
  {
    aUuidStr = mUuidStr;
  }

  int InstanceId() const
  {
    return mCharId.mInstanceId;
  }

  void GetValue(JSContext* cx, JS::MutableHandle<JSObject*> aValue) const;

  void GetProperties(GattCharacteristicProperties& aProperties) const;

  


  already_AddRefed<Promise> ReadValue(ErrorResult& aRv);
  already_AddRefed<Promise> WriteValue(const ArrayBuffer& aValue,
                                       ErrorResult& aRv);

  


  already_AddRefed<Promise> StartNotifications(ErrorResult& aRv);
  already_AddRefed<Promise> StopNotifications(ErrorResult& aRv);

  


  const BluetoothGattId& GetCharacteristicId() const
  {
    return mCharId;
  }

  void Notify(const BluetoothSignal& aData); 

  nsPIDOMWindow* GetParentObject() const
  {
     return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  BluetoothGattCharacteristic(nsPIDOMWindow* aOwner,
                              BluetoothGattService* aService,
                              const BluetoothGattCharAttribute& aChar);

private:
  ~BluetoothGattCharacteristic();

  






  void HandleDescriptorsDiscovered(const BluetoothValue& aValue);

  




  void HandleCharacteristicValueUpdated(const BluetoothValue& aValue);

  


  nsCOMPtr<nsPIDOMWindow> mOwner;

  


  nsRefPtr<BluetoothGattService> mService;

  


  nsTArray<nsRefPtr<BluetoothGattDescriptor>> mDescriptors;

  




  BluetoothGattId mCharId;

  


  nsString mUuidStr;

  


  nsTArray<uint8_t> mValue;

  


  BluetoothGattCharProp mProperties;

  


  BluetoothGattWriteType mWriteType;
};

END_BLUETOOTH_NAMESPACE

#endif
