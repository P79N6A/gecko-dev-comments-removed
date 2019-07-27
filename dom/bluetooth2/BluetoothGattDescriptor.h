





#ifndef mozilla_dom_bluetooth_bluetoothgattdescriptor_h__
#define mozilla_dom_bluetooth_bluetoothgattdescriptor_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/BluetoothGattDescriptorBinding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "nsCOMPtr.h"
#include "nsWrapperCache.h"
#include "nsPIDOMWindow.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothGattCharacteristic;
class BluetoothSignal;
class BluetoothValue;

class BluetoothGattDescriptor final : public nsISupports
                                    , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(BluetoothGattDescriptor)

  


  BluetoothGattCharacteristic* Characteristic() const
  {
    return mCharacteristic;
  }

  void GetUuid(nsString& aUuidStr) const
  {
    aUuidStr = mUuidStr;
  }

  


  void Notify(const BluetoothSignal& aData); 

  nsPIDOMWindow* GetParentObject() const
  {
     return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  BluetoothGattDescriptor(nsPIDOMWindow* aOwner,
                          BluetoothGattCharacteristic* aCharacteristic,
                          const BluetoothGattId& aDescriptorId);

private:
  ~BluetoothGattDescriptor();

  


  nsCOMPtr<nsPIDOMWindow> mOwner;

  


  nsRefPtr<BluetoothGattCharacteristic> mCharacteristic;

  




  BluetoothGattId mDescriptorId;

  


  nsString mUuidStr;
};

END_BLUETOOTH_NAMESPACE

#endif
