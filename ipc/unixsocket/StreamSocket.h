





#ifndef mozilla_ipc_streamsocket_h
#define mozilla_ipc_streamsocket_h

#include "ConnectionOrientedSocket.h"

class MessageLoop;

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

  









  nsresult Connect(UnixSocketConnector* aConnector, int aDelayMs,
                   MessageLoop* aConsumerLoop, MessageLoop* aIOLoop);

  







  nsresult Connect(UnixSocketConnector* aConnector, int aDelayMs = 0);

  
  

  nsresult PrepareAccept(UnixSocketConnector* aConnector,
                         MessageLoop* aConsumerLoop,
                         MessageLoop* aIOLoop,
                         ConnectionOrientedSocketIO*& aIO) override;

  
  

  void SendSocketData(UnixSocketIOBuffer* aBuffer) override;

  
  

  void Close() override;
  void OnConnectSuccess() override;
  void OnConnectError() override;
  void OnDisconnect() override;

protected:
  virtual ~StreamSocket();

private:
  StreamSocketIO* mIO;
  StreamSocketConsumer* mConsumer;
  int mIndex;
};

} 
} 

#endif 
