



#ifndef NfcService_h
#define NfcService_h

#include "mozilla/ipc/Nfc.h"
#include "mozilla/ipc/SocketBase.h"
#include "nsCOMPtr.h"
#include "nsINfcService.h"
#include "NfcMessageHandler.h"

class nsIThread;

namespace mozilla {
namespace dom {
class NfcEventOptions;
} 

class NfcService final : public nsINfcService,
                         public mozilla::ipc::NfcSocketListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINFCSERVICE

  static already_AddRefed<NfcService> FactoryCreate();

  void DispatchNfcEvent(const mozilla::dom::NfcEventOptions& aOptions);

  virtual void ReceiveSocketData(
    nsAutoPtr<mozilla::ipc::UnixSocketBuffer>& aBuffer) override;

  virtual void OnConnectSuccess(enum SocketType aSocketType) override;
  virtual void OnConnectError(enum SocketType aSocketType) override;
  virtual void OnDisconnect(enum SocketType aSocketType) override;

  nsCOMPtr<nsIThread> GetThread() {
    return mThread;
  }

private:
  NfcService();
  ~NfcService();

  nsCOMPtr<nsIThread> mThread;
  nsCOMPtr<nsINfcGonkEventListener> mListener;
  nsRefPtr<mozilla::ipc::NfcListenSocket> mListenSocket;
  nsRefPtr<mozilla::ipc::NfcConsumer> mConsumer;
  nsAutoPtr<NfcMessageHandler> mHandler;
  nsCString mListenSocketName;
};

} 

#endif 
