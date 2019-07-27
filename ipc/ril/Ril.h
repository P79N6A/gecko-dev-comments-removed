





#ifndef mozilla_ipc_Ril_h
#define mozilla_ipc_Ril_h 1

#include <mozilla/dom/workers/Workers.h>
#include <mozilla/ipc/StreamSocket.h>
#include <mozilla/ipc/StreamSocketConsumer.h>

namespace mozilla {
namespace ipc {

class RilConsumer final : public StreamSocketConsumer
{
public:
  static nsresult Register(
    unsigned int aClientId,
    mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);
  static void Shutdown();

  void Send(UnixSocketRawData* aRawData);

private:
  RilConsumer(unsigned long aClientId,
              mozilla::dom::workers::WorkerCrossThreadDispatcher* aDispatcher);

  void Close();

  
  

  void ReceiveSocketData(int aIndex,
                         nsAutoPtr<UnixSocketBuffer>& aBuffer) override;
  void OnConnectSuccess(int aIndex) override;
  void OnConnectError(int aIndex) override;
  void OnDisconnect(int aIndex) override;

  nsRefPtr<StreamSocket> mSocket;
  nsRefPtr<mozilla::dom::workers::WorkerCrossThreadDispatcher> mDispatcher;
  nsCString mAddress;
  bool mShutdown;
};

} 
} 

#endif 
