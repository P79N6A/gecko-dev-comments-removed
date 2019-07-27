







#include "mozilla/ipc/Nfc.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#undef CHROMIUM_LOG
#if (defined(MOZ_WIDGET_GONK) && defined(DEBUG))
#include <android/log.h>
#define CHROMIUM_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args)
#else
#define CHROMIUM_LOG(args...)
#endif

#include "jsfriendapi.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/ipc/NfcConnector.h"
#include "nsThreadUtils.h" 

using namespace mozilla::ipc;

namespace mozilla {
namespace ipc {





NfcListenSocket::NfcListenSocket(NfcSocketListener* aListener)
  : mListener(aListener)
{ }

void
NfcListenSocket::OnConnectSuccess()
{
  if (mListener) {
    mListener->OnConnectSuccess(NfcSocketListener::LISTEN_SOCKET);
  }
}

void
NfcListenSocket::OnConnectError()
{
  if (mListener) {
    mListener->OnConnectError(NfcSocketListener::LISTEN_SOCKET);
  }
}

void
NfcListenSocket::OnDisconnect()
{
  if (mListener) {
    mListener->OnDisconnect(NfcSocketListener::LISTEN_SOCKET);
  }
}

} 
} 
