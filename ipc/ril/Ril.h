





#ifndef mozilla_ipc_Ril_h
#define mozilla_ipc_Ril_h 1

#include <mozilla/dom/workers/Workers.h>
#include <mozilla/ipc/UnixSocket.h>

namespace mozilla {
namespace ipc {

class RilConsumer : public mozilla::ipc::UnixSocketConsumer
{
public:
  RilConsumer(unsigned long aClientId,
              mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);
  virtual ~RilConsumer() { }

  void Shutdown();

private:
  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage);

  virtual void OnConnectSuccess();
  virtual void OnConnectError();
  virtual void OnDisconnect();

private:
  nsRefPtr<mozilla::dom::workers::WorkerCrossThreadDispatcher> mDispatcher;
  unsigned long mClientId;
  nsCString mAddress;
  bool mShutdown;
};

} 
} 

#endif 
