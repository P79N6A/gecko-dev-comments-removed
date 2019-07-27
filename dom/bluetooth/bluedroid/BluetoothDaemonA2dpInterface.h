





#ifndef mozilla_dom_bluetooth_bluetoothdaemona2dpinterface_h
#define mozilla_dom_bluetooth_bluetoothdaemona2dpinterface_h

#include "BluetoothDaemonHelpers.h"
#include "BluetoothInterface.h"
#include "BluetoothInterfaceHelpers.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSetupResultHandler;

class BluetoothDaemonA2dpModule
{
public:
  enum {
    SERVICE_ID = 0x06
  };

  enum {
    OPCODE_ERROR = 0x00,
    OPCODE_CONNECT = 0x01,
    OPCODE_DISCONNECT = 0x02
  };

  static const int MAX_NUM_CLIENTS;

  virtual nsresult Send(DaemonSocketPDU* aPDU, void* aUserData) = 0;

  virtual nsresult RegisterModule(uint8_t aId, uint8_t aMode,
                                  uint32_t aMaxNumClients,
                                  BluetoothSetupResultHandler* aRes) = 0;

  virtual nsresult UnregisterModule(uint8_t aId,
                                    BluetoothSetupResultHandler* aRes) = 0;

  void SetNotificationHandler(
    BluetoothA2dpNotificationHandler* aNotificationHandler);

  
  
  

  nsresult ConnectCmd(const nsAString& aBdAddr,
                      BluetoothA2dpResultHandler* aRes);
  nsresult DisconnectCmd(const nsAString& aBdAddr,
                         BluetoothA2dpResultHandler* aRes);

protected:
  nsresult Send(DaemonSocketPDU* aPDU,
                BluetoothA2dpResultHandler* aRes);

  void HandleSvc(const DaemonSocketPDUHeader& aHeader,
                 DaemonSocketPDU& aPDU, void* aUserData);

  
  
  

  typedef BluetoothResultRunnable0<BluetoothA2dpResultHandler, void>
    ResultRunnable;

  typedef BluetoothResultRunnable1<BluetoothA2dpResultHandler, void,
                                   BluetoothStatus, BluetoothStatus>
    ErrorRunnable;

  void ErrorRsp(const DaemonSocketPDUHeader& aHeader,
                DaemonSocketPDU& aPDU,
                BluetoothA2dpResultHandler* aRes);

  void ConnectRsp(const DaemonSocketPDUHeader& aHeader,
                  DaemonSocketPDU& aPDU,
                  BluetoothA2dpResultHandler* aRes);

  void DisconnectRsp(const DaemonSocketPDUHeader& aHeader,
                     DaemonSocketPDU& aPDU,
                     BluetoothA2dpResultHandler* aRes);

  void HandleRsp(const DaemonSocketPDUHeader& aHeader,
                 DaemonSocketPDU& aPDU,
                 void* aUserData);

  
  
  

  class NotificationHandlerWrapper;

  typedef BluetoothNotificationRunnable2<NotificationHandlerWrapper, void,
                                         BluetoothA2dpConnectionState,
                                         nsString,
                                         BluetoothA2dpConnectionState,
                                         const nsAString&>
    ConnectionStateNotification;

  typedef BluetoothNotificationRunnable2<NotificationHandlerWrapper, void,
                                         BluetoothA2dpAudioState,
                                         nsString,
                                         BluetoothA2dpAudioState,
                                         const nsAString&>
    AudioStateNotification;

  typedef BluetoothNotificationRunnable3<NotificationHandlerWrapper, void,
                                         nsString, uint32_t, uint8_t,
                                         const nsAString&, uint32_t, uint8_t>
    AudioConfigNotification;

  class ConnectionStateInitOp;
  class AudioStateInitOp;
  class AudioConfigInitOp;

  void ConnectionStateNtf(const DaemonSocketPDUHeader& aHeader,
                          DaemonSocketPDU& aPDU);

  void AudioStateNtf(const DaemonSocketPDUHeader& aHeader,
                     DaemonSocketPDU& aPDU);

  void AudioConfigNtf(const DaemonSocketPDUHeader& aHeader,
                      DaemonSocketPDU& aPDU);

  void HandleNtf(const DaemonSocketPDUHeader& aHeader,
                 DaemonSocketPDU& aPDU,
                 void* aUserData);

  static BluetoothA2dpNotificationHandler* sNotificationHandler;
};

class BluetoothDaemonA2dpInterface final
  : public BluetoothA2dpInterface
{
  class CleanupResultHandler;
  class InitResultHandler;

public:
  BluetoothDaemonA2dpInterface(BluetoothDaemonA2dpModule* aModule);
  ~BluetoothDaemonA2dpInterface();

  void Init(
    BluetoothA2dpNotificationHandler* aNotificationHandler,
    BluetoothA2dpResultHandler* aRes);
  void Cleanup(BluetoothA2dpResultHandler* aRes);

  

  void Connect(const nsAString& aBdAddr,
               BluetoothA2dpResultHandler* aRes);
  void Disconnect(const nsAString& aBdAddr,
                  BluetoothA2dpResultHandler* aRes);

private:
  void DispatchError(BluetoothA2dpResultHandler* aRes,
                     BluetoothStatus aStatus);
  void DispatchError(BluetoothA2dpResultHandler* aRes, nsresult aRv);

  BluetoothDaemonA2dpModule* mModule;
};

END_BLUETOOTH_NAMESPACE

#endif
