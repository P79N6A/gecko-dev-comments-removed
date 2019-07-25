





#ifndef mozilla_dom_bluetooth_bluetoothdbuseventservice_h__
#define mozilla_dom_bluetooth_bluetoothdbuseventservice_h__

#include "BluetoothCommon.h"
#include "mozilla/ipc/RawDBusConnection.h"
#include "BluetoothService.h"

class DBusMessage;

BEGIN_BLUETOOTH_NAMESPACE










class BluetoothDBusService : public BluetoothService
                           , private mozilla::ipc::RawDBusConnection
{
public:
  






  virtual nsresult StartInternal();

  






  virtual nsresult StopInternal();

  





  virtual nsresult GetDefaultAdapterPathInternal(BluetoothReplyRunnable* aRunnable);

  






  virtual nsresult StartDiscoveryInternal(const nsAString& aAdapterPath,
                                          BluetoothReplyRunnable* aRunnable);
  






  virtual nsresult StopDiscoveryInternal(const nsAString& aAdapterPath,
                                         BluetoothReplyRunnable* aRunnable);

private:
  nsresult SendDiscoveryMessage(const nsAString& aAdapterPath,
                                const char* aMessageName,
                                BluetoothReplyRunnable* aRunnable);
};

END_BLUETOOTH_NAMESPACE

#endif
