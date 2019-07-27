





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

  









  bool Listen(UnixSocketConnector* aConnector,
              ConnectionOrientedSocket* aCOSocket);

  









  bool Listen(ConnectionOrientedSocket* aCOSocket);

  



  void Close();

  


  void GetSocketAddr(nsAString& aAddrStr);

  
  

  void CloseSocket() override;

private:
  ListenSocketIO* mIO;
};

} 
} 

#endif 
