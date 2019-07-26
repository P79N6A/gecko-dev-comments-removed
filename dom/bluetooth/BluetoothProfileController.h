





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
  















  BluetoothProfileController(bool aConnect,
                             const nsAString& aDeviceAddress,
                             BluetoothReplyRunnable* aRunnable,
                             BluetoothProfileControllerCallback aCallback,
                             uint16_t aServiceUuid,
                             uint32_t aCod = 0);
  ~BluetoothProfileController();

  



  void Start();

  



  void OnConnect(const nsAString& aErrorStr);

  



  void OnDisconnect(const nsAString& aErrorStr);

private:
  
  void SetupProfiles(bool aAssignServiceClass);

  
  void AddProfile(BluetoothProfileManagerBase* aProfile,
                  bool aCheckConnected = false);

  
  void AddProfileWithServiceClass(BluetoothServiceClass aClass);

  
  void Next();

  const bool mConnect;
  nsString mDeviceAddress;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
  BluetoothProfileControllerCallback mCallback;

  bool mSuccess;
  int8_t mProfilesIndex;
  nsTArray<BluetoothProfileManagerBase*> mProfiles;

  
  union {
    uint32_t cod;
    BluetoothServiceClass service;
  } mTarget;
};

END_BLUETOOTH_NAMESPACE

#endif
