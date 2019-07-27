





#ifndef mozilla_ipc_DaemonSocket_h
#define mozilla_ipc_DaemonSocket_h

#include "mozilla/ipc/ConnectionOrientedSocket.h"

namespace mozilla {
namespace ipc {

class DaemonSocketConsumer;
class DaemonSocketIO;
class DaemonSocketIOConsumer;






class DaemonSocket : public ConnectionOrientedSocket
{
public:
  DaemonSocket(DaemonSocketIOConsumer* aIOConsumer,
               DaemonSocketConsumer* aConsumer,
               int aIndex);
  virtual ~DaemonSocket();

  
  

  nsresult PrepareAccept(UnixSocketConnector* aConnector,
                         MessageLoop* aConsumerLoop,
                         MessageLoop* aIOLoop,
                         ConnectionOrientedSocketIO*& aIO) override;

  
  

  void SendSocketData(UnixSocketIOBuffer* aBuffer) override;

  
  

  void Close() override;
  void OnConnectSuccess() override;
  void OnConnectError() override;
  void OnDisconnect() override;

private:
  DaemonSocketIO* mIO;
  DaemonSocketIOConsumer* mIOConsumer;
  DaemonSocketConsumer* mConsumer;
  int mIndex;
};

}
}

#endif
