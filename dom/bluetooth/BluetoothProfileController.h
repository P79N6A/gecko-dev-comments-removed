





#ifndef mozilla_dom_bluetooth_bluetoothprofilecontroller_h__
#define mozilla_dom_bluetooth_bluetoothprofilecontroller_h__

#include "BluetoothUuid.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"

BEGIN_BLUETOOTH_NAMESPACE














#define GET_MAJOR_SERVICE_CLASS(cod) ((cod & 0xffe000) >> 13)


#define GET_MAJOR_DEVICE_CLASS(cod)  ((cod & 0x1f00) >> 8)


#define GET_MINOR_DEVICE_CLASS(cod)  ((cod & 0xfc) >> 2)


#define HAS_AUDIO(cod)               (cod & 0x200000)


#define HAS_OBJECT_TRANSFER(cod)     (cod & 0x100000)


#define HAS_RENDERING(cod)           (cod & 0x40000)


#define IS_PERIPHERAL(cod)           (GET_MAJOR_DEVICE_CLASS(cod) == 0xa)

class BluetoothProfileManagerBase;
class BluetoothReplyRunnable;
typedef void (*BluetoothProfileControllerCallback)();

class BluetoothProfileController : public RefCounted<BluetoothProfileController>
{
public:
  BluetoothProfileController(const nsAString& aDeviceAddress,
                             BluetoothReplyRunnable* aRunnable,
                             BluetoothProfileControllerCallback aCallback);
  ~BluetoothProfileController();

  
  void Connect(BluetoothServiceClass aClass);

  
  void Connect(uint32_t aCod);

  




  void Disconnect(BluetoothServiceClass aClass = BluetoothServiceClass::UNKNOWN);

  void OnConnect(const nsAString& aErrorStr);
  void OnDisconnect(const nsAString& aErrorStr);

  uint32_t GetCod() const
  {
    return mCod;
  }

private:
  void ConnectNext();
  void DisconnectNext();
  bool AddProfile(BluetoothProfileManagerBase* aProfile,
                  bool aCheckConnected = false);
  bool AddProfileWithServiceClass(BluetoothServiceClass aClass);

  int8_t mProfilesIndex;
  nsTArray<BluetoothProfileManagerBase*> mProfiles;

  BluetoothProfileControllerCallback mCallback;
  uint32_t mCod;
  nsString mDeviceAddress;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
  bool mSuccess;
};

END_BLUETOOTH_NAMESPACE

#endif
