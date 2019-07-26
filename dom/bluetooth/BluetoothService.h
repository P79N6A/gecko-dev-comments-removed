





#ifndef mozilla_dom_bluetooth_bluetootheventservice_h__
#define mozilla_dom_bluetooth_bluetootheventservice_h__

#include "BluetoothCommon.h"
#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsIObserver.h"
#include "nsIThread.h"
#include "nsTObserverArray.h"

namespace mozilla {
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
  class ToggleBtAck;
  friend class ToggleBtAck;

  class ToggleBtTask;
  friend class ToggleBtTask;

  class StartupTask;
  friend class StartupTask;

public:
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
  RegisterManager(BluetoothManager* aManager);

  


  void
  UnregisterManager(BluetoothManager* aManager);

  



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
  StopDiscoveryInternal(const nsAString& aAdapterPath,
                        BluetoothReplyRunnable* aRunnable) = 0;

  






  virtual nsresult
  StartDiscoveryInternal(const nsAString& aAdapterPath,
                         BluetoothReplyRunnable* aRunnable) = 0;

  








  virtual nsresult
  GetProperties(BluetoothObjectType aType,
                const nsAString& aPath,
                BluetoothReplyRunnable* aRunnable) = 0;

  






  virtual nsresult
  GetDevicePropertiesInternal(const BluetoothSignal& aSignal) = 0;

  









  virtual nsresult
  SetProperty(BluetoothObjectType aType,
              const nsAString& aPath,
              const BluetoothNamedValue& aValue,
              BluetoothReplyRunnable* aRunnable) = 0;

  








  virtual bool
  GetDevicePath(const nsAString& aAdapterPath,
                const nsAString& aDeviceAddress,
                nsAString& aDevicePath) = 0;

  virtual nsresult
  CreatePairedDeviceInternal(const nsAString& aAdapterPath,
                             const nsAString& aAddress,
                             int aTimeout,
                             BluetoothReplyRunnable* aRunnable) = 0;

  virtual nsresult
  RemoveDeviceInternal(const nsAString& aAdapterPath,
                       const nsAString& aObjectPath,
                       BluetoothReplyRunnable* aRunnable) = 0;

  virtual nsresult
  GetScoSocket(const nsAString& aObjectPath,
               bool aAuth,
               bool aEncrypt,
               mozilla::ipc::UnixSocketConsumer* aConsumer) = 0;

  virtual nsresult
  GetSocketViaService(const nsAString& aObjectPath,
                      const nsAString& aService,
                      BluetoothSocketType aType,
                      bool aAuth,
                      bool aEncrypt,
                      mozilla::ipc::UnixSocketConsumer* aSocketConsumer,
                      BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool
  SetPinCodeInternal(const nsAString& aDeviceAddress, const nsAString& aPinCode,
                     BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool
  SetPasskeyInternal(const nsAString& aDeviceAddress, uint32_t aPasskey,
                     BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool
  SetPairingConfirmationInternal(const nsAString& aDeviceAddress, bool aConfirm,
                                 BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool
  SetAuthorizationInternal(const nsAString& aDeviceAddress, bool aAllow,
                           BluetoothReplyRunnable* aRunnable) = 0;

  virtual nsresult
  PrepareAdapterInternal(const nsAString& aPath) = 0;

  virtual bool
  ConnectHeadset(const nsAString& aDeviceAddress,
                 const nsAString& aAdapterPath,
                 BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  DisconnectHeadset(BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool
  ConnectObjectPush(const nsAString& aDeviceAddress,
                    const nsAString& aAdapterPath,
                    BluetoothReplyRunnable* aRunnable) = 0;

  virtual void
  DisconnectObjectPush(BluetoothReplyRunnable* aRunnable) = 0;

  bool
  IsEnabled() const
  {
    return mEnabled;
  }

protected:
  BluetoothService()
  : mEnabled(false), mSettingsCheckInProgress(false),
    mRegisteredForLocalAgent(false)
#ifdef DEBUG
    , mLastRequestedEnable(false)
#endif
  {
    mBluetoothSignalObserverTable.Init();
  }

  virtual ~BluetoothService();

  bool
  Init();

  void
  Cleanup();

  nsresult
  StartStopBluetooth(bool aStart);

  





  virtual nsresult
  StartInternal() = 0;

  





  virtual nsresult
  StopInternal() = 0;

  


  virtual nsresult
  HandleStartup();

  


  nsresult
  HandleStartupSettingsCheck(bool aEnable);

  


  nsresult
  HandleSettingsChanged(const nsAString& aData);

  


  virtual nsresult
  HandleShutdown();

  
  void
  SetEnabled(bool aEnabled);

  
  static BluetoothService*
  Create();

  











  nsCOMPtr<nsIThread> mBluetoothCommandThread;

  typedef nsClassHashtable<nsStringHashKey, BluetoothSignalObserverList >
  BluetoothSignalObserverTable;

  BluetoothSignalObserverTable mBluetoothSignalObserverTable;

  typedef nsTObserverArray<BluetoothManager*> BluetoothManagerList;
  BluetoothManagerList mLiveManagers;

  bool mEnabled;
  bool mSettingsCheckInProgress;
  bool mRegisteredForLocalAgent;

#ifdef DEBUG
  bool mLastRequestedEnable;
#endif
};

END_BLUETOOTH_NAMESPACE

#endif
