





#ifndef mozilla_dom_bluetooth_bluedroid_bluetoothinterface_h__
#define mozilla_dom_bluetooth_bluedroid_bluetoothinterface_h__

#include <hardware/bluetooth.h>
#include <hardware/bt_sock.h>
#include <hardware/bt_hf.h>
#include <hardware/bt_av.h>
#if ANDROID_VERSION >= 18
#include <hardware/bt_rc.h>
#endif
#include "BluetoothCommon.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothInterface;





class BluetoothSocketInterface
{
public:
  friend BluetoothInterface;

  

  bt_status_t Listen(btsock_type_t aType,
                     const char* aServiceName, const uint8_t* aServiceUuid,
                     int aChannel, int& aSockFd, int aFlags);

  bt_status_t Connect(const bt_bdaddr_t* aBdAddr, btsock_type_t aType,
                      const uint8_t* aUuid, int aChannel, int& aSockFd,
                      int aFlags);

protected:
  BluetoothSocketInterface(const btsock_interface_t* aInterface);
  ~BluetoothSocketInterface();

private:
  const btsock_interface_t* mInterface;
};





class BluetoothHandsfreeInterface
{
public:
  friend BluetoothInterface;

  bt_status_t Init(bthf_callbacks_t* aCallbacks);
  void        Cleanup();

  

  bt_status_t Connect(bt_bdaddr_t* aBdAddr);
  bt_status_t Disconnect(bt_bdaddr_t* aBdAddr);
  bt_status_t ConnectAudio(bt_bdaddr_t* aBdAddr);
  bt_status_t DisconnectAudio(bt_bdaddr_t* aBdAddr);

  

  bt_status_t StartVoiceRecognition();
  bt_status_t StopVoiceRecognition();

  

  bt_status_t VolumeControl(bthf_volume_type_t aType, int aVolume);

  

  bt_status_t DeviceStatusNotification(bthf_network_state_t aNtkState,
                                       bthf_service_type_t aSvcType,
                                       int aSignal, int aBattChg);

  

  bt_status_t CopsResponse(const char* aCops);
  bt_status_t CindResponse(int aSvc, int aNumActive, int aNumHeld,
                           bthf_call_state_t aCallSetupState, int aSignal,
                           int aRoam, int aBattChg);
  bt_status_t FormattedAtResponse(const char* aRsp);
  bt_status_t AtResponse(bthf_at_response_t aResponseCode, int aErrorCode);
  bt_status_t ClccResponse(int aIndex, bthf_call_direction_t aDir,
                           bthf_call_state_t aState, bthf_call_mode_t aMode,
                           bthf_call_mpty_type_t aMpty, const char* aNumber,
                           bthf_call_addrtype_t aType);

  

  bt_status_t PhoneStateChange(int aNumActive, int aNumHeld,
                               bthf_call_state_t aCallSetupState,
                               const char* aNumber,
                               bthf_call_addrtype_t aType);

protected:
  BluetoothHandsfreeInterface(const bthf_interface_t* aInterface);
  ~BluetoothHandsfreeInterface();

private:
  const bthf_interface_t* mInterface;
};





class BluetoothA2dpInterface
{
public:
  friend BluetoothInterface;

  bt_status_t Init(btav_callbacks_t *aCallbacks);
  void        Cleanup();

  bt_status_t Connect(bt_bdaddr_t *aBdAddr);
  bt_status_t Disconnect(bt_bdaddr_t *aBdAddr);

protected:
  BluetoothA2dpInterface(const btav_interface_t* aInterface);
  ~BluetoothA2dpInterface();

private:
  const btav_interface_t* mInterface;
};





class BluetoothAvrcpInterface
{
#if ANDROID_VERSION >= 18
public:
  friend BluetoothInterface;

  bt_status_t Init(btrc_callbacks_t* aCallbacks);
  void        Cleanup();

  bt_status_t GetPlayStatusRsp(btrc_play_status_t aPlayStatus,
                               uint32_t aSongLen, uint32_t aSongPos);

  bt_status_t ListPlayerAppAttrRsp(int aNumAttr, btrc_player_attr_t* aPAttrs);
  bt_status_t ListPlayerAppValueRsp(int aNumVal, uint8_t* aPVals);

  bt_status_t GetPlayerAppValueRsp(btrc_player_settings_t* aPVals);
  bt_status_t GetPlayerAppAttrTextRsp(int aNumAttr,
                                      btrc_player_setting_text_t* aPAttrs);
  bt_status_t GetPlayerAppValueTextRsp(int aNumVal,
                                       btrc_player_setting_text_t* aPVals);

  bt_status_t GetElementAttrRsp(uint8_t aNumAttr,
                                btrc_element_attr_val_t* aPAttrs);

  bt_status_t SetPlayerAppValueRsp(btrc_status_t aRspStatus);

  bt_status_t RegisterNotificationRsp(btrc_event_id_t aEventId,
                                      btrc_notification_type_t aType,
                                      btrc_register_notification_t* aPParam);

  bt_status_t SetVolume(uint8_t aVolume);

protected:
  BluetoothAvrcpInterface(const btrc_interface_t* aInterface);
  ~BluetoothAvrcpInterface();

private:
  const btrc_interface_t* mInterface;
#endif
};





class BluetoothResultHandler
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BluetoothResultHandler)

  virtual ~BluetoothResultHandler() { }

  virtual void OnError(int aStatus)
  {
    BT_LOGR("received error code %d", aStatus);
  }

  virtual void Init() { }
  virtual void Cleanup() { }
  virtual void Enable() { }
  virtual void Disable() { }

  virtual void GetAdapterProperties() { }
  virtual void GetAdapterProperty() { }
  virtual void SetAdapterProperty() { }

  virtual void GetRemoteDeviceProperties() { }
  virtual void GetRemoteDeviceProperty() { }
  virtual void SetRemoteDeviceProperty() { }

  virtual void GetRemoteServiceRecord() { }
  virtual void GetRemoteServices() { }

  virtual void StartDiscovery() { }
  virtual void CancelDiscovery() { }

  virtual void CreateBond() { }
  virtual void RemoveBond() { }
  virtual void CancelBond() { }

  virtual void PinReply() { }
  virtual void SspReply() { }

  virtual void DutModeConfigure() { }
  virtual void DutModeSend() { }

  virtual void LeTestMode() { }
};

class BluetoothInterface
{
public:
  static BluetoothInterface* GetInstance();

  void Init(bt_callbacks_t* aCallbacks, BluetoothResultHandler* aRes);
  void Cleanup(BluetoothResultHandler* aRes);

  void Enable(BluetoothResultHandler* aRes);
  void Disable(BluetoothResultHandler* aRes);


  

  int GetAdapterProperties();
  int GetAdapterProperty(bt_property_type_t aType);
  int SetAdapterProperty(const bt_property_t* aProperty);

  

  int GetRemoteDeviceProperties(bt_bdaddr_t *aRemoteAddr);
  int GetRemoteDeviceProperty(bt_bdaddr_t* aRemoteAddr,
                              bt_property_type_t aType);
  int SetRemoteDeviceProperty(bt_bdaddr_t* aRemoteAddr,
                              const bt_property_t* aProperty);

  

  int GetRemoteServiceRecord(bt_bdaddr_t* aRemoteAddr,
                             bt_uuid_t* aUuid);
  int GetRemoteServices(bt_bdaddr_t* aRemoteAddr);

  

  int StartDiscovery();
  int CancelDiscovery();

  

  int CreateBond(const bt_bdaddr_t* aBdAddr);
  int RemoveBond(const bt_bdaddr_t* aBdAddr);
  int CancelBond(const bt_bdaddr_t* aBdAddr);

  

  int PinReply(const bt_bdaddr_t* aBdAddr, uint8_t aAccept,
               uint8_t aPinLen, bt_pin_code_t* aPinCode);

  int SspReply(const bt_bdaddr_t* aBdAddr, bt_ssp_variant_t aVariant,
               uint8_t aAccept, uint32_t aPasskey);

  

  int DutModeConfigure(uint8_t aEnable);
  int DutModeSend(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen);

  

  int LeTestMode(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen);

  

  BluetoothSocketInterface* GetBluetoothSocketInterface();
  BluetoothHandsfreeInterface* GetBluetoothHandsfreeInterface();
  BluetoothA2dpInterface* GetBluetoothA2dpInterface();
  BluetoothAvrcpInterface* GetBluetoothAvrcpInterface();

protected:
  BluetoothInterface(const bt_interface_t* aInterface);
  ~BluetoothInterface();

private:
  template <class T>
  T* GetProfileInterface();

  const bt_interface_t* mInterface;
};

END_BLUETOOTH_NAMESPACE

#endif
