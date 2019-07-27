





#ifndef mozilla_ipc_BluetoothDaemonConnectionConsumer_h
#define mozilla_ipc_BluetoothDaemonConnectionConsumer_h

#include "mozilla/Attributes.h"
#include "mozilla/FileUtils.h"
#include "mozilla/ipc/ConnectionOrientedSocket.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace ipc {




class DaemonSocketConsumer
{
public:
  




  virtual void OnConnectSuccess(int aIndex) = 0;

  




  virtual void OnConnectError(int aIndex) = 0;

  




  virtual void OnDisconnect(int aIndex) = 0;

protected:
  DaemonSocketConsumer();
  virtual ~DaemonSocketConsumer();
};

}
}

#endif

