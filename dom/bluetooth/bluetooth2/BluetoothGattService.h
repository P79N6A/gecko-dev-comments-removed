





#ifndef mozilla_dom_bluetooth_bluetoothgattservice_h__
#define mozilla_dom_bluetooth_bluetoothgattservice_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/BluetoothGattServiceBinding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/dom/bluetooth/BluetoothGattCharacteristic.h"
#include "nsCOMPtr.h"
#include "nsWrapperCache.h"
#include "nsPIDOMWindow.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSignal;
class BluetoothValue;

class BluetoothGattService final : public nsISupports
                                 , public nsWrapperCache
                                 , public BluetoothSignalObserver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(BluetoothGattService)

  


  bool IsPrimary() const
  {
    return mServiceId.mIsPrimary;
  }

  void GetUuid(nsString& aUuidStr) const
  {
    aUuidStr = mUuidStr;
  }

  int InstanceId() const
  {
    return mServiceId.mId.mInstanceId;
  }

  void GetIncludedServices(
    nsTArray<nsRefPtr<BluetoothGattService>>& aIncludedServices) const
  {
    aIncludedServices = mIncludedServices;
  }

  void GetCharacteristics(
    nsTArray<nsRefPtr<BluetoothGattCharacteristic>>& aCharacteristics) const
  {
    aCharacteristics = mCharacteristics;
  }

  


  const nsAString& GetAppUuid() const
  {
    return mAppUuid;
  }

  const BluetoothGattServiceId& GetServiceId() const
  {
    return mServiceId;
  }

  void Notify(const BluetoothSignal& aData); 

  nsPIDOMWindow* GetParentObject() const
  {
     return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  BluetoothGattService(nsPIDOMWindow* aOwner,
                       const nsAString& aAppUuid,
                       const BluetoothGattServiceId& aServiceId);

private:
  ~BluetoothGattService();

  







  void HandleIncludedServicesDiscovered(const BluetoothValue& aValue);

  






  void HandleCharacteristicsDiscovered(const BluetoothValue& aValue);

  


  nsCOMPtr<nsPIDOMWindow> mOwner;

  


  nsString mAppUuid;

  





  BluetoothGattServiceId mServiceId;

  


  nsString mUuidStr;

  


  nsTArray<nsRefPtr<BluetoothGattService>> mIncludedServices;

  


  nsTArray<nsRefPtr<BluetoothGattCharacteristic>> mCharacteristics;
};

END_BLUETOOTH_NAMESPACE









template <>
class nsDefaultComparator <
  nsRefPtr<mozilla::dom::bluetooth::BluetoothGattService>,
  mozilla::dom::bluetooth::BluetoothGattServiceId> {
public:
  bool Equals(
    const nsRefPtr<mozilla::dom::bluetooth::BluetoothGattService>& aService,
    const mozilla::dom::bluetooth::BluetoothGattServiceId& aServiceId) const
  {
    return aService->GetServiceId() == aServiceId;
  }
};

#endif
