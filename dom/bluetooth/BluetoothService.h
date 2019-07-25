





#ifndef mozilla_dom_bluetooth_bluetootheventservice_h__
#define mozilla_dom_bluetooth_bluetootheventservice_h__

#include "BluetoothCommon.h"
#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsIObserver.h"
#include "nsIThread.h"
#include "nsTObserverArray.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothManager;
class BluetoothNamedValue;
class BluetoothReplyRunnable;
class BluetoothSignal;

class BluetoothService : public nsIObserver
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

  









  nsresult RegisterBluetoothSignalHandler(const nsAString& aNodeName,
                                          BluetoothSignalObserver* aMsgHandler);

  









  nsresult UnregisterBluetoothSignalHandler(const nsAString& aNodeName,
                                            BluetoothSignalObserver* aMsgHandler);

  






  nsresult DistributeSignal(const BluetoothSignal& aEvent);

  







  nsresult Start();

  








  nsresult Stop();

  


  nsresult HandleStartup();

  


  nsresult HandleSettingsChanged(const nsAString& aData);

  


  nsresult HandleShutdown();

  


  void RegisterManager(BluetoothManager* aManager);

  


  void UnregisterManager(BluetoothManager* aManager);

  







  static BluetoothService* Get();

  static already_AddRefed<BluetoothService> FactoryCreate()
  {
    nsRefPtr<BluetoothService> service = Get();
    return service.forget();
  }

  





  virtual nsresult GetDefaultAdapterPathInternal(BluetoothReplyRunnable* aRunnable) = 0;

  





  virtual nsresult GetPairedDevicePropertiesInternal(const nsTArray<nsString>& aDeviceAddresses,
                                                     BluetoothReplyRunnable* aRunnable) = 0;

  






  virtual nsresult StopDiscoveryInternal(const nsAString& aAdapterPath,
                                         BluetoothReplyRunnable* aRunnable) = 0;

  






  virtual nsresult StartDiscoveryInternal(const nsAString& aAdapterPath,
                                          BluetoothReplyRunnable* aRunnable) = 0;

  





  virtual nsresult StartInternal() = 0;

  





  virtual nsresult StopInternal() = 0;

  








  virtual nsresult
  GetProperties(BluetoothObjectType aType,
                const nsAString& aPath,
                BluetoothReplyRunnable* aRunnable) = 0;

  









  virtual nsresult
  SetProperty(BluetoothObjectType aType,
              const nsAString& aPath,
              const BluetoothNamedValue& aValue,
              BluetoothReplyRunnable* aRunnable) = 0;

  








  virtual bool
  GetDevicePath(const nsAString& aAdapterPath,
                const nsAString& aDeviceAddress,
                nsAString& aDevicePath) = 0;

  virtual nsTArray<PRUint32>
  AddReservedServicesInternal(const nsAString& aAdapterPath,
                              const nsTArray<uint32_t>& aServices) = 0;

  virtual bool
  RemoveReservedServicesInternal(const nsAString& aAdapterPath,
                                 const nsTArray<uint32_t>& aServiceHandles) = 0;

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
  GetSocketViaService(const nsAString& aObjectPath,
                      const nsAString& aService,
                      int aType,
                      bool aAuth,
                      bool aEncrypt,
                      BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool
  CloseSocket(int aFd, BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool SetPinCodeInternal(const nsAString& aDeviceAddress, const nsAString& aPinCode) = 0;
  virtual bool SetPasskeyInternal(const nsAString& aDeviceAddress, uint32_t aPasskey) = 0;
  virtual bool SetPairingConfirmationInternal(const nsAString& aDeviceAddress, bool aConfirm) = 0;
  virtual bool SetAuthorizationInternal(const nsAString& aDeviceAddress, bool aAllow) = 0;

  virtual bool IsEnabled()
  {
    return mEnabled;
  }

protected:
  BluetoothService()
  : mEnabled(false)
#ifdef DEBUG
    , mLastRequestedEnable(false)
#endif
  {
    mBluetoothSignalObserverTable.Init();
  }

  virtual ~BluetoothService()
  { }

  nsresult StartStopBluetooth(bool aStart);

  
  void SetEnabled(bool aEnabled);

  
  
  static BluetoothService* Create();

  











  nsCOMPtr<nsIThread> mBluetoothCommandThread;

  typedef mozilla::ObserverList<BluetoothSignal> BluetoothSignalObserverList;
  typedef nsClassHashtable<nsStringHashKey, BluetoothSignalObserverList >
  BluetoothSignalObserverTable;

  BluetoothSignalObserverTable mBluetoothSignalObserverTable;

  typedef nsTObserverArray<BluetoothManager*> BluetoothManagerList;
  BluetoothManagerList mLiveManagers;

  bool mEnabled;
#ifdef DEBUG
  bool mLastRequestedEnable;
#endif
};

END_BLUETOOTH_NAMESPACE

#endif
