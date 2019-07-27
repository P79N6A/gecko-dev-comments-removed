





#ifndef mozilla_dom_bluetooth_bluetoothprofilecontroller_h__
#define mozilla_dom_bluetooth_bluetoothprofilecontroller_h__

#include "BluetoothUuid.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"
#include "nsITimer.h"

BEGIN_BLUETOOTH_NAMESPACE














#define GET_MAJOR_SERVICE_CLASS(cod) ((cod & 0xffe000) >> 13)


#define GET_MAJOR_DEVICE_CLASS(cod)  ((cod & 0x1f00) >> 8)


#define GET_MINOR_DEVICE_CLASS(cod)  ((cod & 0xfc) >> 2)


#define HAS_AUDIO(cod)               (cod & 0x200000)


#define HAS_RENDERING(cod)           (cod & 0x40000)


#define IS_PERIPHERAL(cod)           (GET_MAJOR_DEVICE_CLASS(cod) == 0x5)


#define IS_REMOTE_CONTROL(cod)       ((GET_MINOR_DEVICE_CLASS(cod) & 0xf) == 0x3)


#define IS_KEYBOARD(cod)             ((GET_MINOR_DEVICE_CLASS(cod) & 0x10) >> 4)


#define IS_POINTING_DEVICE(cod)      ((GET_MINOR_DEVICE_CLASS(cod) & 0x20) >> 5)










#define IS_INVALID_COD(cod)          (cod >> 24)

class BluetoothProfileManagerBase;
class BluetoothReplyRunnable;
typedef void (*BluetoothProfileControllerCallback)();

class BluetoothProfileController final
{
  ~BluetoothProfileController();

public:
  NS_INLINE_DECL_REFCOUNTING(BluetoothProfileController)
  















  BluetoothProfileController(bool aConnect,
                             const nsAString& aDeviceAddress,
                             BluetoothReplyRunnable* aRunnable,
                             BluetoothProfileControllerCallback aCallback,
                             uint16_t aServiceUuid,
                             uint32_t aCod = 0);

  



  void StartSession();

  


  void EndSession();

  



  void NotifyCompletion(const nsAString& aErrorStr);

  


  void GiveupAndContinue();

private:
  
  void SetupProfiles(bool aAssignServiceClass);

  
  void AddProfile(BluetoothProfileManagerBase* aProfile,
                  bool aCheckConnected = false);

  
  void AddProfileWithServiceClass(BluetoothServiceClass aClass);

  
  void Next();

  
  bool IsBtServiceAvailable() const;

  const bool mConnect;
  nsString mDeviceAddress;
  nsRefPtr<BluetoothReplyRunnable> mRunnable;
  BluetoothProfileControllerCallback mCallback;

  bool mCurrentProfileFinished;
  bool mSuccess;
  int8_t mProfilesIndex;
  nsTArray<BluetoothProfileManagerBase*> mProfiles;

  
  union {
    uint32_t cod;
    BluetoothServiceClass service;
  } mTarget;

  nsCOMPtr<nsITimer> mTimer;
};

END_BLUETOOTH_NAMESPACE

#endif
