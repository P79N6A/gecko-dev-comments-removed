





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
#include "mozilla/dom/bluetooth/BluetoothTypes.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothInterface;





class BluetoothSocketResultHandler
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BluetoothSocketResultHandler)

  virtual ~BluetoothSocketResultHandler() { }

  virtual void OnError(bt_status_t aStatus)
  {
    BT_WARNING("Received error code %d", (int)aStatus);
  }

  virtual void Listen(int aSockFd) { }
  virtual void Connect(int aSockFd, const nsAString& aBdAddress,
                       int aConnectionState) { }
  virtual void Accept(int aSockFd, const nsAString& aBdAddress,
                      int aConnectionState) { }
};

class BluetoothSocketInterface
{
public:
  friend class BluetoothInterface;

  

  void Listen(BluetoothSocketType aType,
              const nsAString& aServiceName,
              const uint8_t aServiceUuid[16],
              int aChannel, bool aEncrypt, bool aAuth,
              BluetoothSocketResultHandler* aRes);

  void Connect(const nsAString& aBdAddr,
               BluetoothSocketType aType,
               const uint8_t aUuid[16],
               int aChannel, bool aEncrypt, bool aAuth,
               BluetoothSocketResultHandler* aRes);

  void Accept(int aFd, BluetoothSocketResultHandler* aRes);

protected:
  BluetoothSocketInterface(const btsock_interface_t* aInterface);
  ~BluetoothSocketInterface();

private:
  const btsock_interface_t* mInterface;
};





class BluetoothHandsfreeResultHandler
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BluetoothHandsfreeResultHandler)

  virtual ~BluetoothHandsfreeResultHandler() { }

  virtual void OnError(bt_status_t aStatus)
  {
    BT_WARNING("Received error code %d", (int)aStatus);
  }

  virtual void Init() { }
  virtual void Cleanup() { }

  virtual void Connect() { }
  virtual void Disconnect() { }
  virtual void ConnectAudio() { }
  virtual void DisconnectAudio() { }

  virtual void StartVoiceRecognition() { }
  virtual void StopVoiceRecognition() { }

  virtual void VolumeControl() { }

  virtual void DeviceStatusNotification() { }

  virtual void CopsResponse() { }
  virtual void CindResponse() { }
  virtual void FormattedAtResponse() { }
  virtual void AtResponse() { }
  virtual void ClccResponse() { }
  virtual void PhoneStateChange() { }
};

class BluetoothHandsfreeInterface
{
public:
  friend class BluetoothInterface;

  void Init(bthf_callbacks_t* aCallbacks,
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

  

  void StartVoiceRecognition(BluetoothHandsfreeResultHandler* aRes);
  void StopVoiceRecognition(BluetoothHandsfreeResultHandler* aRes);

  

  void VolumeControl(BluetoothHandsfreeVolumeType aType, int aVolume,
                     BluetoothHandsfreeResultHandler* aRes);

  

  void DeviceStatusNotification(BluetoothHandsfreeNetworkState aNtkState,
                                BluetoothHandsfreeServiceType aSvcType,
                                int aSignal, int aBattChg,
                                BluetoothHandsfreeResultHandler* aRes);

  

  void CopsResponse(const char* aCops,
                    BluetoothHandsfreeResultHandler* aRes);
  void CindResponse(int aSvc, int aNumActive, int aNumHeld,
                    BluetoothHandsfreeCallState aCallSetupState, int aSignal,
                    int aRoam, int aBattChg,
                    BluetoothHandsfreeResultHandler* aRes);
  void FormattedAtResponse(const char* aRsp,
                           BluetoothHandsfreeResultHandler* aRes);
  void AtResponse(BluetoothHandsfreeAtResponse aResponseCode, int aErrorCode,
                  BluetoothHandsfreeResultHandler* aRes);
  void ClccResponse(int aIndex, BluetoothHandsfreeCallDirection aDir,
                    BluetoothHandsfreeCallState aState,
                    BluetoothHandsfreeCallMode aMode,
                    BluetoothHandsfreeCallMptyType aMpty,
                    const nsAString& aNumber,
                    BluetoothHandsfreeCallAddressType aType,
                    BluetoothHandsfreeResultHandler* aRes);

  

  void PhoneStateChange(int aNumActive, int aNumHeld,
                        BluetoothHandsfreeCallState aCallSetupState,
                        const nsAString& aNumber,
                        BluetoothHandsfreeCallAddressType aType,
                        BluetoothHandsfreeResultHandler* aRes);

protected:
  BluetoothHandsfreeInterface(const bthf_interface_t* aInterface);
  ~BluetoothHandsfreeInterface();

private:
  const bthf_interface_t* mInterface;
};





class BluetoothA2dpResultHandler
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BluetoothA2dpResultHandler)

  virtual ~BluetoothA2dpResultHandler() { }

  virtual void OnError(bt_status_t aStatus)
  {
    BT_WARNING("received error code %d", (int)aStatus);
  }

  virtual void Init() { }
  virtual void Cleanup() { }
  virtual void Connect() { }
  virtual void Disconnect() { }
};

class BluetoothA2dpInterface
{
public:
  friend class BluetoothInterface;

  void Init(btav_callbacks_t *aCallbacks,
            BluetoothA2dpResultHandler* aRes);
  void Cleanup(BluetoothA2dpResultHandler* aRes);

  void Connect(const nsAString& aBdAddr,
               BluetoothA2dpResultHandler* aRes);
  void Disconnect(const nsAString& aBdAddr,
                  BluetoothA2dpResultHandler* aRes);

protected:
  BluetoothA2dpInterface(const btav_interface_t* aInterface);
  ~BluetoothA2dpInterface();

private:
  const btav_interface_t* mInterface;
};





class BluetoothAvrcpResultHandler
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(BluetoothAvrcpResultHandler)

  virtual ~BluetoothAvrcpResultHandler() { }

  virtual void OnError(bt_status_t aStatus)
  {
    BT_WARNING("received error code %d", (int)aStatus);
  }

  virtual void Init() { }
  virtual void Cleanup() { }

  virtual void GetPlayStatusRsp() { }

  virtual void ListPlayerAppAttrRsp() { }
  virtual void ListPlayerAppValueRsp() { }

  virtual void GetPlayerAppValueRsp() { }
  virtual void GetPlayerAppAttrTextRsp() { }
  virtual void GetPlayerAppValueTextRsp() { }

  virtual void GetElementAttrRsp() { }

  virtual void SetPlayerAppValueRsp() { }

  virtual void RegisterNotificationRsp() { }

  virtual void SetVolume() { }
};

class BluetoothAvrcpInterface
{
#if ANDROID_VERSION >= 18
public:
  friend class BluetoothInterface;

  void Init(btrc_callbacks_t* aCallbacks,
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
    BT_LOGR("Received error code %d", aStatus);
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


  

  void GetAdapterProperties(BluetoothResultHandler* aRes);
  void GetAdapterProperty(const nsAString& aName,
                          BluetoothResultHandler* aRes);
  void SetAdapterProperty(const BluetoothNamedValue& aProperty,
                          BluetoothResultHandler* aRes);

  

  void GetRemoteDeviceProperties(const nsAString& aRemoteAddr,
                                 BluetoothResultHandler* aRes);
  void GetRemoteDeviceProperty(const nsAString& aRemoteAddr,
                               const nsAString& aName,
                               BluetoothResultHandler* aRes);
  void SetRemoteDeviceProperty(const nsAString& aRemoteAddr,
                               const BluetoothNamedValue& aProperty,
                               BluetoothResultHandler* aRes);

  

  void GetRemoteServiceRecord(const nsAString& aRemoteAddr,
                              const uint8_t aUuid[16],
                              BluetoothResultHandler* aRes);
  void GetRemoteServices(const nsAString& aRemoteAddr,
                         BluetoothResultHandler* aRes);

  

  void StartDiscovery(BluetoothResultHandler* aRes);
  void CancelDiscovery(BluetoothResultHandler* aRes);

  

  void CreateBond(const nsAString& aBdAddr, BluetoothResultHandler* aRes);
  void RemoveBond(const nsAString& aBdAddr, BluetoothResultHandler* aRes);
  void CancelBond(const nsAString& aBdAddr, BluetoothResultHandler* aRes);

  

  void PinReply(const nsAString& aBdAddr, bool aAccept,
                const nsAString& aPinCode,
                BluetoothResultHandler* aRes);

  void SspReply(const nsAString& aBdAddr, const nsAString& aVariant,
                bool aAccept, uint32_t aPasskey,
                BluetoothResultHandler* aRes);

  

  void DutModeConfigure(bool aEnable, BluetoothResultHandler* aRes);
  void DutModeSend(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen,
                   BluetoothResultHandler* aRes);

  

  void LeTestMode(uint16_t aOpcode, uint8_t* aBuf, uint8_t aLen,
                  BluetoothResultHandler* aRes);

  

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
