





#ifndef mozilla_dom_bluetooth_bluetoothdevice_h__
#define mozilla_dom_bluetooth_bluetoothdevice_h__

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/BluetoothDevice2Binding.h"
#include "BluetoothCommon.h"
#include "nsString.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
  class Promise;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothClassOfDevice;
class BluetoothNamedValue;
class BluetoothValue;
class BluetoothSignal;
class BluetoothSocket;

class BluetoothDevice MOZ_FINAL : public DOMEventTargetHelper
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

  


  IMPL_EVENT_HANDLER(attributechanged);

  


  already_AddRefed<Promise> FetchUuids(ErrorResult& aRv);

  


  static already_AddRefed<BluetoothDevice>
    Create(nsPIDOMWindow* aOwner, const BluetoothValue& aValue);

  void Notify(const BluetoothSignal& aParam); 
  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  virtual void DisconnectFromOwner() MOZ_OVERRIDE;

  


  bool operator==(BluetoothDevice& aDevice) const
  {
    nsString address;
    aDevice.GetAddress(address);
    return mAddress.Equals(address);
  }

private:
  BluetoothDevice(nsPIDOMWindow* aOwner, const BluetoothValue& aValue);
  ~BluetoothDevice();

  




  void SetPropertyByValue(const BluetoothNamedValue& aValue);

  




  void HandlePropertyChanged(const BluetoothValue& aValue);

  


  void DispatchAttributeEvent(const nsTArray<nsString>& aTypes);

  




  BluetoothDeviceAttribute
    ConvertStringToDeviceAttribute(const nsAString& aString);

  





  bool IsDeviceAttributeChanged(BluetoothDeviceAttribute aType,
                                const BluetoothValue& aValue);

  


  


  nsString mAddress;

  


  nsRefPtr<BluetoothClassOfDevice> mCod;

  


  nsString mName;

  


  bool mPaired;

  


  nsTArray<nsString> mUuids;
};

END_BLUETOOTH_NAMESPACE

#endif
