





#ifndef mozilla_dom_bluetooth_bluetoothgattcharacteristic_h__
#define mozilla_dom_bluetooth_bluetoothgattcharacteristic_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/BluetoothGattCharacteristicBinding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/dom/bluetooth/BluetoothGattDescriptor.h"
#include "nsCOMPtr.h"
#include "nsWrapperCache.h"
#include "nsPIDOMWindow.h"

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
                              const BluetoothGattId& aCharId);

private:
  ~BluetoothGattCharacteristic();

  






  void HandleDescriptorsDiscovered(const BluetoothValue& aValue);

  


  nsCOMPtr<nsPIDOMWindow> mOwner;

  


  nsRefPtr<BluetoothGattService> mService;

  


  nsTArray<nsRefPtr<BluetoothGattDescriptor>> mDescriptors;

  




  BluetoothGattId mCharId;

  


  nsString mUuidStr;
};

END_BLUETOOTH_NAMESPACE

#endif
