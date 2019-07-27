







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

class NfcConsumer : public mozilla::ipc::UnixSocketConsumer
{
public:
  NfcConsumer(NfcSocketListener* aListener);
  virtual ~NfcConsumer() { }

  void Shutdown();
  bool PostToNfcDaemon(const uint8_t* aData, size_t aSize);

private:
  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aData);

  virtual void OnConnectSuccess();
  virtual void OnConnectError();
  virtual void OnDisconnect();

private:
  NfcSocketListener* mListener;
  nsCString mAddress;
  bool mShutdown;
};

} 
} 

#endif 
