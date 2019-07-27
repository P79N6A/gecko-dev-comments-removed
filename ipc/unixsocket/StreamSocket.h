





#ifndef mozilla_ipc_streamsocket_h
#define mozilla_ipc_streamsocket_h

#include "ConnectionOrientedSocket.h"

namespace mozilla {
namespace ipc {

class StreamSocketConsumer;
class StreamSocketIO;
class UnixSocketConnector;

class StreamSocket final : public ConnectionOrientedSocket
{
public:
  StreamSocket(StreamSocketConsumer* aConsumer, int aIndex);

  




  void ReceiveSocketData(nsAutoPtr<UnixSocketBuffer>& aBuffer);

  







  nsresult Connect(UnixSocketConnector* aConnector, int aDelayMs = 0);

  
  

  nsresult PrepareAccept(UnixSocketConnector* aConnector,
                         ConnectionOrientedSocketIO*& aIO) override;

  
  

  void SendSocketData(UnixSocketIOBuffer* aBuffer) override;

  
  

  void Close() override;
  void OnConnectSuccess() override;
  void OnConnectError() override;
  void OnDisconnect() override;

protected:
  virtual ~StreamSocket();

private:
  StreamSocketConsumer* mConsumer;
  int mIndex;
  StreamSocketIO* mIO;
};

} 
} 

#endif 
