





#ifndef mozilla_dom_bluetooth_bluetootheventservice_h__
#define mozilla_dom_bluetooth_bluetootheventservice_h__

#include "BluetoothCommon.h"
#include "BluetoothProfileManagerBase.h"
#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsIDOMFile.h"
#include "nsIObserver.h"
#include "nsTObserverArray.h"
#include "nsThreadUtils.h"

class nsIDOMBlob;

namespace mozilla {
namespace dom {
class BlobChild;
class BlobParent;
}
namespace ipc {
class UnixSocketConsumer;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothManager;
class BluetoothNamedValue;
class BluetoothReplyRunnable;
class BluetoothSignal;

typedef mozilla::ObserverList<BluetoothSignal> BluetoothSignalObserverList;

class BluetoothService : public nsIObserver
                       , public BluetoothSignalObserver
{
  class ToggleBtTask;
  friend class ToggleBtTask;

  class StartupTask;
  friend class StartupTask;

public:
  class ToggleBtAck : public nsRunnable
  {
  public:
    ToggleBtAck(bool aEnabled);
    NS_IMETHOD Run();

  private:
    bool mEnabled;
  };
  friend class ToggleBtAck;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  






  virtual void
  RegisterBluetoothSignalHandler(const nsAString& aNodeName,
                                 BluetoothSignalObserver* aMsgHandler);

  






  virtual void
  UnregisterBluetoothSignalHandler(const nsAString& aNodeName,
                                   BluetoothSignalObserver* aMsgHandler);

  





  void
  UnregisterAllSignalHandlers(BluetoothSignalObserver* aMsgHandler);

  






  void
  DistributeSignal(const BluetoothSignal& aEvent);

  



  void
  Notify(const BluetoothSignal& aParam);

  







  static BluetoothService*
  Get();

  static already_AddRefed<BluetoothService>
  FactoryCreate()
  {
    nsRefPtr<BluetoothService> service = Get();
    return service.forget();
  }

  





  virtual nsresult
  GetDefaultAdapterPathInternal(BluetoothReplyRunnable* aRunnable) = 0;

  





  virtual nsresult
  GetPairedDevicePropertiesInternal(const nsTArray<nsString>& aDeviceAddresses,
                                    BluetoothReplyRunnable* aRunnable) = 0;

  





  virtual nsresult
  GetConnectedDevicePropertiesInternal(uint16_t aServiceUuid,
                                       BluetoothReplyRunnable* aRunnable) = 0;

  




  virtual nsresult
  StopDiscoveryInternal(BluetoothReplyRunnable* aRunnable) = 0;

  




  virtual nsresult
  StartDiscoveryInternal(BluetoothReplyRunnable* aRunnable) = 0;

  








  virtual nsresult
  SetProperty(BluetoothObjectType aType,
              const BluetoothNamedValue& aValue,
              BluetoothReplyRunnable* aRunnable) = 0;

  virtual nsresult
  CreatePairedDeviceInternal(const nsAString& aAddress,
                             int aTimeout,
                             BluetoothReplyRunnable* aRunnable) = 0;

  virtual nsresult
  RemoveDeviceInternal(const nsAString& aObjectPath,
                       BluetoothReplyRunnable* aRunnable) = 0;

  









  virtual nsresult
  GetServiceChannel(const nsAString& aDeviceAddress,
                    const nsAString& aServiceUuid,
                    BluetoothProfileManagerBase* aManager) = 0;

  virtual bool
  UpdateSdpRecords(const nsAString& aDeviceAddress,
                   BluetoothProfileManagerBase* aManager) = 0;

  virtual bool
  SetPinCodeInternal(const nsAString& aDeviceAddress, const nsAString& aPinCode,
                     BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool
  SetPasskeyInternal(const nsAString& aDeviceAddress, uint32_t aPasskey,
                     BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool
  SetPairingConfirmationInternal(const nsAString& aDeviceAddress, bool aConfirm,
                                 BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  Connect(const nsAString& aDeviceAddress, uint32_t aCod, uint16_t aServiceUuid,
          BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  Disconnect(const nsAString& aDeviceAddress, uint16_t aServiceUuid,
             BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  IsConnected(const uint16_t aServiceUuid, BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  SendFile(const nsAString& aDeviceAddress,
           BlobParent* aBlobParent,
           BlobChild* aBlobChild,
           BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  SendFile(const nsAString& aDeviceAddress,
           nsIDOMBlob* aBlob,
           BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  StopSendingFile(const nsAString& aDeviceAddress,
                  BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  ConfirmReceivingFile(const nsAString& aDeviceAddress, bool aConfirm,
                       BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  ConnectSco(BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  DisconnectSco(BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  IsScoConnected(BluetoothReplyRunnable* aRunnable) = 0;

#ifdef MOZ_B2G_RIL
  virtual void
  AnswerWaitingCall(BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  IgnoreWaitingCall(BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  ToggleCalls(BluetoothReplyRunnable* aRunnable) = 0;
#endif

  virtual void
  SendMetaData(const nsAString& aTitle,
               const nsAString& aArtist,
               const nsAString& aAlbum,
               int64_t aMediaNumber,
               int64_t aTotalMediaCount,
               int64_t aDuration,
               BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  SendPlayStatus(int64_t aDuration,
                 int64_t aPosition,
                 const nsAString& aPlayStatus,
                 BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  UpdatePlayStatus(uint32_t aDuration,
                   uint32_t aPosition,
                   ControlPlayStatus aPlayStatus) = 0;

  virtual nsresult
  SendSinkMessage(const nsAString& aDeviceAddresses,
                  const nsAString& aMessage) = 0;

  virtual nsresult
  SendInputMessage(const nsAString& aDeviceAddresses,
                   const nsAString& aMessage) = 0;

  bool
  IsEnabled() const
  {
    return mEnabled;
  }

  bool
  IsToggling() const;

  static void AcknowledgeToggleBt(bool aEnabled);

  



  void TryFiringAdapterAdded();
  void AdapterAddedReceived();

protected:
  BluetoothService() : mEnabled(false)
                     , mAdapterAddedReceived(false)
  {
  }

  virtual ~BluetoothService();

  bool
  Init();

  void
  Cleanup();

  nsresult
  StartBluetooth(bool aIsStartup);

  nsresult
  StopBluetooth(bool aIsStartup);

  nsresult
  StartStopBluetooth(bool aStart, bool aIsStartup);

  





  virtual nsresult
  StartInternal() = 0;

  





  virtual nsresult
  StopInternal() = 0;

  


  virtual nsresult
  HandleStartup();

  


  nsresult
  HandleStartupSettingsCheck(bool aEnable);

  


  nsresult
  HandleSettingsChanged(nsISupports* aSubject);

  


  virtual nsresult
  HandleShutdown();

  
  void
  SetEnabled(bool aEnabled);

  
  static BluetoothService*
  Create();

  void CompleteToggleBt(bool aEnabled);

  typedef nsClassHashtable<nsStringHashKey, BluetoothSignalObserverList >
  BluetoothSignalObserverTable;

  BluetoothSignalObserverTable mBluetoothSignalObserverTable;

  bool mEnabled;

private:
  bool mAdapterAddedReceived;
};

END_BLUETOOTH_NAMESPACE

#endif
