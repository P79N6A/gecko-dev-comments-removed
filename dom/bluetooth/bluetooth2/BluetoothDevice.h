





#ifndef mozilla_dom_bluetooth_bluetoothdevice_h__
#define mozilla_dom_bluetooth_bluetoothdevice_h__

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/BluetoothDevice2Binding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "nsString.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
  class Promise;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothClassOfDevice;
class BluetoothGatt;
class BluetoothNamedValue;
class BluetoothValue;
class BluetoothSignal;
class BluetoothSocket;

class BluetoothDevice final : public DOMEventTargetHelper
                            , public BluetoothSignalObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothDevice,
                                           DOMEventTargetHelper)

  


  void GetAddress(nsString& aAddress) const
  {
    aAddress = mAddress;
  }

  BluetoothClassOfDevice* Cod() const
  {
    return mCod;
  }

  void GetName(nsString& aName) const
  {
    aName = mName;
  }

  bool Paired() const
  {
    return mPaired;
  }

  void GetUuids(nsTArray<nsString>& aUuids) const
  {
    aUuids = mUuids;
  }

  BluetoothDeviceType Type() const
  {
    return mType;
  }

  BluetoothGatt* GetGatt();

  


  IMPL_EVENT_HANDLER(attributechanged);

  


  already_AddRefed<Promise> FetchUuids(ErrorResult& aRv);

  


  static already_AddRefed<BluetoothDevice>
    Create(nsPIDOMWindow* aOwner, const BluetoothValue& aValue);

  void Notify(const BluetoothSignal& aParam); 
  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;
  virtual void DisconnectFromOwner() override;

private:
  BluetoothDevice(nsPIDOMWindow* aOwner, const BluetoothValue& aValue);
  ~BluetoothDevice();

  




  void SetPropertyByValue(const BluetoothNamedValue& aValue);

  




  void HandlePropertyChanged(const BluetoothValue& aValue);

  


  void DispatchAttributeEvent(const Sequence<nsString>& aTypes);

  




  BluetoothDeviceType ConvertUint32ToDeviceType(const uint32_t aValue);

  




  BluetoothDeviceAttribute
    ConvertStringToDeviceAttribute(const nsAString& aString);

  





  bool IsDeviceAttributeChanged(BluetoothDeviceAttribute aType,
                                const BluetoothValue& aValue);

  


  


  nsString mAddress;

  


  nsRefPtr<BluetoothClassOfDevice> mCod;

  


  nsString mName;

  


  bool mPaired;

  


  nsTArray<nsString> mUuids;

  


  BluetoothDeviceType mType;

  


  nsRefPtr<BluetoothGatt> mGatt;
};

END_BLUETOOTH_NAMESPACE








template <>
class nsDefaultComparator <nsRefPtr<mozilla::dom::bluetooth::BluetoothDevice>,
                           nsRefPtr<mozilla::dom::bluetooth::BluetoothDevice>> {
  public:

    bool Equals(
      const nsRefPtr<mozilla::dom::bluetooth::BluetoothDevice>& aDeviceA,
      const nsRefPtr<mozilla::dom::bluetooth::BluetoothDevice>& aDeviceB) const
    {
      nsString addressA, addressB;
      aDeviceA->GetAddress(addressA);
      aDeviceB->GetAddress(addressB);

      return addressA.Equals(addressB);
    }
};









template <>
class nsDefaultComparator <nsRefPtr<mozilla::dom::bluetooth::BluetoothDevice>,
                           nsString> {
public:
  bool Equals(
    const nsRefPtr<mozilla::dom::bluetooth::BluetoothDevice>& aDevice,
    const nsString& aAddress) const
  {
    nsString deviceAddress;
    aDevice->GetAddress(deviceAddress);

    return deviceAddress.Equals(aAddress);
  }
};

#endif
