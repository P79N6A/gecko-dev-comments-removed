





#ifndef mozilla_ipc_streamsocket_h
#define mozilla_ipc_streamsocket_h

#include "mozilla/ipc/SocketBase.h"
#include "ConnectionOrientedSocket.h"

namespace mozilla {
namespace ipc {

class StreamSocketIO;
class UnixSocketConnector;

class StreamSocket : public SocketConsumerBase
                   , public ConnectionOrientedSocket
{
public:
  StreamSocket();

  





  void SendSocketData(UnixSocketIOBuffer* aBuffer);

  










  bool SendSocketData(const nsACString& aMessage);

  









  bool Connect(UnixSocketConnector* aConnector,
               const char* aAddress,
               int aDelayMs = 0);

  



  void Close();

  


  void GetSocketAddr(nsAString& aAddrStr);

protected:
  virtual ~StreamSocket();

  
  
  
  ConnectionOrientedSocketIO* PrepareAccept(UnixSocketConnector* aConnector);

private:

  
  void CloseSocket() override
  {
    Close();
  }

  StreamSocketIO* mIO;
};

} 
} 

#endif 
