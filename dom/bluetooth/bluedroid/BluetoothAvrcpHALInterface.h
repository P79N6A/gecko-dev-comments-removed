





#ifndef mozilla_dom_bluetooth_bluedroid_bluetoothavrcphalinterface_h__
#define mozilla_dom_bluetooth_bluedroid_bluetoothavrcphalinterface_h__

#include <hardware/bluetooth.h>
#if ANDROID_VERSION >= 18
#include <hardware/bt_rc.h>
#endif
#include "BluetoothCommon.h"
#include "BluetoothInterface.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothHALInterface;

class BluetoothAvrcpHALInterface final
  : public BluetoothAvrcpInterface
{
public:
  friend class BluetoothHALInterface;

  void Init(BluetoothAvrcpNotificationHandler* aNotificationHandler,
            BluetoothAvrcpResultHandler* aRes);
  void Cleanup(BluetoothAvrcpResultHandler* aRes);

  void GetPlayStatusRsp(ControlPlayStatus aPlayStatus,
                        uint32_t aSongLen, uint32_t aSongPos,
                        BluetoothAvrcpResultHandler* aRes);

  void ListPlayerAppAttrRsp(int aNumAttr,
                            const BluetoothAvrcpPlayerAttribute* aPAttrs,
                            BluetoothAvrcpResultHandler* aRes);
  void ListPlayerAppValueRsp(int aNumVal, uint8_t* aPVals,
                             BluetoothAvrcpResultHandler* aRes);

  
  void GetPlayerAppValueRsp(uint8_t aNumAttrs,
                            const uint8_t* aIds, const uint8_t* aValues,
                            BluetoothAvrcpResultHandler* aRes);
  
  void GetPlayerAppAttrTextRsp(int aNumAttr,
                               const uint8_t* aIds, const char** aTexts,
                               BluetoothAvrcpResultHandler* aRes);
  
  void GetPlayerAppValueTextRsp(int aNumVal,
                                const uint8_t* aIds, const char** aTexts,
                                BluetoothAvrcpResultHandler* aRes);

  void GetElementAttrRsp(uint8_t aNumAttr,
                         const BluetoothAvrcpElementAttribute* aAttr,
                         BluetoothAvrcpResultHandler* aRes);

  void SetPlayerAppValueRsp(BluetoothAvrcpStatus aRspStatus,
                            BluetoothAvrcpResultHandler* aRes);

  void RegisterNotificationRsp(BluetoothAvrcpEvent aEvent,
                               BluetoothAvrcpNotification aType,
                               const BluetoothAvrcpNotificationParam& aParam,
                               BluetoothAvrcpResultHandler* aRes);

  void SetVolume(uint8_t aVolume, BluetoothAvrcpResultHandler* aRes);

protected:
  BluetoothAvrcpHALInterface(
#if ANDROID_VERSION >= 18
    const btrc_interface_t* aInterface
#endif
    );
  ~BluetoothAvrcpHALInterface();

private:
#if ANDROID_VERSION >= 18
  const btrc_interface_t* mInterface;
#endif
};

END_BLUETOOTH_NAMESPACE

#endif
