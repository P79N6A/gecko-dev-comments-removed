





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

  







  nsresult Connect(UnixSocketConnector* aConnector, int aDelayMs = 0);

  
  

  void SendSocketData(UnixSocketIOBuffer* aBuffer) override;

  
  

  void Close() override;

protected:
  virtual ~StreamSocket();

  
  
  
  ConnectionOrientedSocketIO* PrepareAccept(UnixSocketConnector* aConnector);

private:
  StreamSocketIO* mIO;
};

} 
} 

#endif 
