





#ifndef mozilla_dom_bluetooth_bluetootheventservice_h__
#define mozilla_dom_bluetooth_bluetootheventservice_h__

#include "nsThreadUtils.h"
#include "nsClassHashtable.h"
#include "nsIObserver.h"
#include "BluetoothCommon.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSignal;
class BluetoothReplyRunnable;
class BluetoothNamedValue;

class BluetoothService : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  









  nsresult RegisterBluetoothSignalHandler(const nsAString& aNodeName,
                                          BluetoothSignalObserver* aMsgHandler);

  









  nsresult UnregisterBluetoothSignalHandler(const nsAString& aNodeName,
                                            BluetoothSignalObserver* aMsgHandler);

  






  nsresult DistributeSignal(const BluetoothSignal& aEvent);

  












  nsresult Start(BluetoothReplyRunnable* aResultRunnable);

  












  nsresult Stop(BluetoothReplyRunnable* aResultRunnable);

  







  static BluetoothService* Get();
  
  





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

  virtual int
  GetDeviceServiceChannelInternal(const nsAString& aObjectPath,
                                  const nsAString& aPattern,
                                  int aAttributeId) = 0;

  virtual nsTArray<PRUint32>
  AddReservedServicesInternal(const nsAString& aAdapterPath,
                              const nsTArray<PRUint32>& aServices) = 0;

  virtual bool
  RemoveReservedServicesInternal(const nsAString& aAdapterPath,
                                 const nsTArray<PRUint32>& aServiceHandles) = 0;

  virtual nsresult
  CreatePairedDeviceInternal(const nsAString& aAdapterPath,
                             const nsAString& aAddress,
                             int aTimeout,
                             BluetoothReplyRunnable* aRunnable) = 0;

  virtual nsresult
  RemoveDeviceInternal(const nsAString& aAdapterPath,
                       const nsAString& aObjectPath,
                       BluetoothReplyRunnable* aRunnable) = 0;

  virtual bool SetPinCodeInternal(const nsAString& aDeviceAddress, const nsAString& aPinCode) = 0;
  virtual bool SetPasskeyInternal(const nsAString& aDeviceAddress, PRUint32 aPasskey) = 0;
  virtual bool SetPairingConfirmationInternal(const nsAString& aDeviceAddress, bool aConfirm) = 0;
  virtual bool SetAuthorizationInternal(const nsAString& aDeviceAddress, bool aAllow) = 0;

  











  nsCOMPtr<nsIThread> mBluetoothCommandThread;

protected:
  BluetoothService()
  {
    mBluetoothSignalObserverTable.Init();
  }

  virtual ~BluetoothService()
  {
  }

  nsresult StartStopBluetooth(BluetoothReplyRunnable* aResultRunnable,
                              bool aStart);
  
  
  static BluetoothService* Create();

  typedef mozilla::ObserverList<BluetoothSignal> BluetoothSignalObserverList;
  typedef nsClassHashtable<nsStringHashKey, BluetoothSignalObserverList >
  BluetoothSignalObserverTable;

  BluetoothSignalObserverTable mBluetoothSignalObserverTable;
};

END_BLUETOOTH_NAMESPACE

#endif
