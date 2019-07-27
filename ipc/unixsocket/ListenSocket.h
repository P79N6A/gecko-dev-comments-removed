





#ifndef mozilla_ipc_listensocket_h
#define mozilla_ipc_listensocket_h

#include "nsString.h"
#include "mozilla/ipc/SocketBase.h"

namespace mozilla {
namespace ipc {

class ConnectionOrientedSocket;
class ListenSocketIO;
class UnixSocketConnector;

class ListenSocket : public SocketBase
{
protected:
  virtual ~ListenSocket();

public:
  ListenSocket();

  








  nsresult Listen(UnixSocketConnector* aConnector,
                  ConnectionOrientedSocket* aCOSocket);

  








  nsresult Listen(ConnectionOrientedSocket* aCOSocket);

  
  

  void Close() override;

private:
  ListenSocketIO* mIO;
};

} 
} 

#endif 
