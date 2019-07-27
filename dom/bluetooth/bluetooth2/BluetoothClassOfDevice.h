





#ifndef mozilla_dom_bluetooth_bluetoothclassofdevice_h
#define mozilla_dom_bluetooth_bluetoothclassofdevice_h

#include "BluetoothCommon.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

struct JSContext;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothClassOfDevice final : public nsISupports
                                   , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(BluetoothClassOfDevice)

  static already_AddRefed<BluetoothClassOfDevice>
    Create(nsPIDOMWindow* aOwner);

  uint16_t MajorServiceClass() const
  {
    return mMajorServiceClass;
  }

  uint8_t MajorDeviceClass() const
  {
    return mMajorDeviceClass;
  }

  uint8_t MinorDeviceClass() const
  {
    return mMinorDeviceClass;
  }

  




  bool Equals(const uint32_t aValue);

  





  uint32_t ToUint32();

  




  void Update(const uint32_t aValue);

  nsPIDOMWindow* GetParentObject() const
  {
    return mOwnerWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

private:
  BluetoothClassOfDevice(nsPIDOMWindow* aOwner);
  ~BluetoothClassOfDevice();

  


  void Reset();

  uint16_t mMajorServiceClass;
  uint8_t mMajorDeviceClass;
  uint8_t mMinorDeviceClass;

  nsCOMPtr<nsPIDOMWindow> mOwnerWindow;
};

END_BLUETOOTH_NAMESPACE

#endif 
