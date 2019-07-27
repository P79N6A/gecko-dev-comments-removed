





#ifndef mozilla_ipc_listensocket_h
#define mozilla_ipc_listensocket_h

#include "nsString.h"
#include "mozilla/ipc/SocketBase.h"

class MessageLoop;
class nsIThread;

namespace mozilla {
namespace ipc {

class ConnectionOrientedSocket;
class ListenSocketConsumer;
class ListenSocketIO;
class UnixSocketConnector;

class ListenSocket final : public SocketBase
{
public:
  





  ListenSocket(ListenSocketConsumer* aConsumer, int aIndex);

  










  nsresult Listen(UnixSocketConnector* aConnector,
                  nsIThread* aConsumerThread,
                  MessageLoop* aIOLoop,
                  ConnectionOrientedSocket* aCOSocket);

  








  nsresult Listen(UnixSocketConnector* aConnector,
                  ConnectionOrientedSocket* aCOSocket);

  








  nsresult Listen(ConnectionOrientedSocket* aCOSocket);

  
  

  void Close() override;
  void OnConnectSuccess() override;
  void OnConnectError() override;
  void OnDisconnect() override;

protected:
  virtual ~ListenSocket();

private:
  ListenSocketIO* mIO;
  ListenSocketConsumer* mConsumer;
  int mIndex;
};

} 
} 

#endif 
