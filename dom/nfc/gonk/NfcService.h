





#ifndef NfcService_h
#define NfcService_h

#include <mozilla/ipc/ListenSocket.h>
#include "mozilla/ipc/ListenSocketConsumer.h"
#include <mozilla/ipc/StreamSocket.h>
#include "mozilla/ipc/StreamSocketConsumer.h"
#include "nsCOMPtr.h"
#include "nsINfcService.h"
#include "NfcMessageHandler.h"

class nsIThread;

namespace mozilla {
namespace dom {
class NfcEventOptions;
} 

class NfcService final
  : public nsINfcService
  , public mozilla::ipc::StreamSocketConsumer
  , public mozilla::ipc::ListenSocketConsumer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINFCSERVICE

  static already_AddRefed<NfcService> FactoryCreate();

  void DispatchNfcEvent(const mozilla::dom::NfcEventOptions& aOptions);

  bool PostToNfcDaemon(const uint8_t* aData, size_t aSize);

  nsCOMPtr<nsIThread> GetThread() {
    return mThread;
  }

  
  

  void ReceiveSocketData(
    int aIndex, nsAutoPtr<mozilla::ipc::UnixSocketBuffer>& aBuffer) override;
  void OnConnectSuccess(int aIndex) override;
  void OnConnectError(int aIndex) override;
  void OnDisconnect(int aIndex) override;

private:
  enum SocketType {
    LISTEN_SOCKET,
    STREAM_SOCKET
  };

  NfcService();
  ~NfcService();

  nsCOMPtr<nsIThread> mThread;
  nsCOMPtr<nsINfcGonkEventListener> mListener;
  nsRefPtr<mozilla::ipc::ListenSocket> mListenSocket;
  nsRefPtr<mozilla::ipc::StreamSocket> mStreamSocket;
  nsAutoPtr<NfcMessageHandler> mHandler;
  nsCString mListenSocketName;
};

} 

#endif
