





#ifndef mozilla_ipc_RilSocket_h
#define mozilla_ipc_RilSocket_h

#include "mozilla/ipc/ConnectionOrientedSocket.h"

class JSContext;
class MessageLoop;

namespace mozilla {
namespace dom {
namespace workers {

class WorkerCrossThreadDispatcher;

} 
} 
} 

namespace mozilla {
namespace ipc {

class RilSocketConsumer;
class RilSocketIO;
class UnixSocketConnector;

class RilSocket final : public ConnectionOrientedSocket
{
public:
  






  RilSocket(mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher,
            RilSocketConsumer* aConsumer, int aIndex);

  





  void ReceiveSocketData(JSContext* aCx, nsAutoPtr<UnixSocketBuffer>& aBuffer);

  









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
  virtual ~RilSocket();

private:
  RilSocketIO* mIO;
  nsRefPtr<mozilla::dom::workers::WorkerCrossThreadDispatcher> mDispatcher;
  RilSocketConsumer* mConsumer;
  int mIndex;
};

} 
} 

#endif 
