





#ifndef mozilla_ipc_Ril_h
#define mozilla_ipc_Ril_h 1

#include <mozilla/dom/workers/Workers.h>
#include <mozilla/ipc/StreamSocket.h>

namespace mozilla {
namespace ipc {

class RilConsumer MOZ_FINAL : public mozilla::ipc::StreamSocket
{
public:
  static nsresult Register(
    unsigned int aClientId,
    mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);
  static void Shutdown();

  ConnectionOrientedSocketIO* GetIO() MOZ_OVERRIDE;

private:
  RilConsumer(unsigned long aClientId,
              mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);

  void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage) MOZ_OVERRIDE;

  void OnConnectSuccess() MOZ_OVERRIDE;
  void OnConnectError() MOZ_OVERRIDE;
  void OnDisconnect() MOZ_OVERRIDE;

  nsRefPtr<mozilla::dom::workers::WorkerCrossThreadDispatcher> mDispatcher;
  unsigned long mClientId;
  nsCString mAddress;
  bool mShutdown;
};

} 
} 

#endif 
