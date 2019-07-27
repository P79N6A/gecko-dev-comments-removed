





#ifndef mozilla_ipc_listensocket_h
#define mozilla_ipc_listensocket_h

#include "nsString.h"
#include "mozilla/ipc/SocketBase.h"

namespace mozilla {
namespace ipc {

class ConnectionOrientedSocket;
class ListenSocketConsumer;
class ListenSocketIO;
class UnixSocketConnector;

class ListenSocket final : public SocketBase
{
protected:
  virtual ~ListenSocket();

public:
  ListenSocket(ListenSocketConsumer* aConsumer, int aIndex);

  








  nsresult Listen(UnixSocketConnector* aConnector,
                  ConnectionOrientedSocket* aCOSocket);

  








  nsresult Listen(ConnectionOrientedSocket* aCOSocket);

  
  

  void Close() override;
  void OnConnectSuccess() override;
  void OnConnectError() override;
  void OnDisconnect() override;

private:
  ListenSocketConsumer* mConsumer;
  int mIndex;
  ListenSocketIO* mIO;
};

} 
} 

#endif 
