







#ifndef mozilla_ipc_Nfc_h
#define mozilla_ipc_Nfc_h 1

#include <mozilla/ipc/UnixSocket.h>

namespace mozilla {
namespace ipc {

class NfcSocketListener
{
public:
  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aData) = 0;
};

class NfcConsumer MOZ_FINAL : public mozilla::ipc::UnixSocketConsumer
{
public:
  NfcConsumer(NfcSocketListener* aListener);

  void Shutdown();
  bool PostToNfcDaemon(const uint8_t* aData, size_t aSize);

private:
  void ReceiveSocketData(
    nsAutoPtr<UnixSocketRawData>& aData) MOZ_OVERRIDE;

  void OnConnectSuccess() MOZ_OVERRIDE;
  void OnConnectError() MOZ_OVERRIDE;
  void OnDisconnect() MOZ_OVERRIDE;

private:
  NfcSocketListener* mListener;
  nsCString mAddress;
  bool mShutdown;
};

} 
} 

#endif 
