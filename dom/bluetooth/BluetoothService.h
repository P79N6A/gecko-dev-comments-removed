





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

  






  virtual nsresult StopDiscoveryInternal(const nsAString& aAdapterPath,
                                         BluetoothReplyRunnable* aRunnable) = 0;

  






  virtual nsresult StartDiscoveryInternal(const nsAString& aAdapterPath,
                                          BluetoothReplyRunnable* aRunnable) = 0;

  





  virtual nsresult StartInternal() = 0;

  





  virtual nsresult StopInternal() = 0;

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
