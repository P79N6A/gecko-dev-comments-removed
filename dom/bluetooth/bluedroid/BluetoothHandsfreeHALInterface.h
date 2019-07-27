





#ifndef mozilla_dom_bluetooth_bluedroid_bluetoothhandsfreehalinterface_h__
#define mozilla_dom_bluetooth_bluedroid_bluetoothhandsfreehalinterface_h__

#include <hardware/bluetooth.h>
#include <hardware/bt_hf.h>
#include "BluetoothCommon.h"
#include "BluetoothInterface.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothHALInterface;

class BluetoothHandsfreeHALInterface final
  : public BluetoothHandsfreeInterface
{
public:
  friend class BluetoothHALInterface;

  void Init(BluetoothHandsfreeNotificationHandler* aNotificationHandler,
            int aMaxNumClients,
            BluetoothHandsfreeResultHandler* aRes);
  void Cleanup(BluetoothHandsfreeResultHandler* aRes);

  

  void Connect(const nsAString& aBdAddr,
               BluetoothHandsfreeResultHandler* aRes);
  void Disconnect(const nsAString& aBdAddr,
                  BluetoothHandsfreeResultHandler* aRes);
  void ConnectAudio(const nsAString& aBdAddr,
                    BluetoothHandsfreeResultHandler* aRes);
  void DisconnectAudio(const nsAString& aBdAddr,
                       BluetoothHandsfreeResultHandler* aRes);

  

  void StartVoiceRecognition(const nsAString& aBdAddr,
                             BluetoothHandsfreeResultHandler* aRes);
  void StopVoiceRecognition(const nsAString& aBdAddr,
                            BluetoothHandsfreeResultHandler* aRes);

  

  void VolumeControl(BluetoothHandsfreeVolumeType aType, int aVolume,
                     const nsAString& aBdAddr,
                     BluetoothHandsfreeResultHandler* aRes);

  

  void DeviceStatusNotification(BluetoothHandsfreeNetworkState aNtkState,
                                BluetoothHandsfreeServiceType aSvcType,
                                int aSignal, int aBattChg,
                                BluetoothHandsfreeResultHandler* aRes);

  

  void CopsResponse(const char* aCops, const nsAString& aBdAddr,
                    BluetoothHandsfreeResultHandler* aRes);
  void CindResponse(int aSvc, int aNumActive, int aNumHeld,
                    BluetoothHandsfreeCallState aCallSetupState, int aSignal,
                    int aRoam, int aBattChg, const nsAString& aBdAddr,
                    BluetoothHandsfreeResultHandler* aRes);
  void FormattedAtResponse(const char* aRsp, const nsAString& aBdAddr,
                           BluetoothHandsfreeResultHandler* aRes);
  void AtResponse(BluetoothHandsfreeAtResponse aResponseCode, int aErrorCode,
                  const nsAString& aBdAddr,
                  BluetoothHandsfreeResultHandler* aRes);
  void ClccResponse(int aIndex, BluetoothHandsfreeCallDirection aDir,
                    BluetoothHandsfreeCallState aState,
                    BluetoothHandsfreeCallMode aMode,
                    BluetoothHandsfreeCallMptyType aMpty,
                    const nsAString& aNumber,
                    BluetoothHandsfreeCallAddressType aType,
                    const nsAString& aBdAddr,
                    BluetoothHandsfreeResultHandler* aRes);

  

  void PhoneStateChange(int aNumActive, int aNumHeld,
                        BluetoothHandsfreeCallState aCallSetupState,
                        const nsAString& aNumber,
                        BluetoothHandsfreeCallAddressType aType,
                        BluetoothHandsfreeResultHandler* aRes);

  

  void ConfigureWbs(const nsAString& aBdAddr,
                    BluetoothHandsfreeWbsConfig aConfig,
                    BluetoothHandsfreeResultHandler* aRes);

protected:
  BluetoothHandsfreeHALInterface(const bthf_interface_t* aInterface);
  ~BluetoothHandsfreeHALInterface();

private:
  const bthf_interface_t* mInterface;
};

END_BLUETOOTH_NAMESPACE

#endif
