





#ifndef mozilla_ipc_streamsocket_h
#define mozilla_ipc_streamsocket_h

#include "ConnectionOrientedSocket.h"

namespace mozilla {
namespace ipc {

class StreamSocketIO;
class UnixSocketConnector;

class StreamSocket : public ConnectionOrientedSocket
{
public:
  StreamSocket();

  





  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketBuffer>& aBuffer) = 0;

  





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
