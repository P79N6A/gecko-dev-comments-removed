





#ifndef mozilla_ipc_Ril_h
#define mozilla_ipc_Ril_h 1

#include <mozilla/dom/workers/Workers.h>
#include <mozilla/ipc/StreamSocket.h>

namespace mozilla {
namespace ipc {

class RilConsumer final : public mozilla::ipc::StreamSocket
{
public:
  static nsresult Register(
    unsigned int aClientId,
    mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);
  static void Shutdown();

  ConnectionOrientedSocketIO* GetIO() override;

private:
  RilConsumer(unsigned long aClientId,
              mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);

  void ReceiveSocketData(nsAutoPtr<UnixSocketBuffer>& aBuffer) override;

  void OnConnectSuccess() override;
  void OnConnectError() override;
  void OnDisconnect() override;

  nsRefPtr<mozilla::dom::workers::WorkerCrossThreadDispatcher> mDispatcher;
  unsigned long mClientId;
  nsCString mAddress;
  bool mShutdown;
};

} 
} 

#endif 
