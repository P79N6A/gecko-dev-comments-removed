







#ifndef mozilla_ipc_Nfc_h
#define mozilla_ipc_Nfc_h 1

#include <mozilla/dom/workers/Workers.h>
#include <mozilla/ipc/UnixSocket.h>

namespace mozilla {
namespace ipc {

class NfcConsumer : public mozilla::ipc::UnixSocketConsumer
{
public:
  virtual ~NfcConsumer() { }

  static nsresult Register(mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);
  static void Shutdown();

private:
  NfcConsumer(mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);

  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage);

  virtual void OnConnectSuccess();
  virtual void OnConnectError();
  virtual void OnDisconnect();

private:
  nsRefPtr<mozilla::dom::workers::WorkerCrossThreadDispatcher> mDispatcher;
  nsCString mAddress;
  bool mShutdown;
};

} 
} 

#endif 
