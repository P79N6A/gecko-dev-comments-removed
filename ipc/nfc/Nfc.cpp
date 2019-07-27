







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
#include "nsThreadUtils.h" 

using namespace mozilla::ipc;

namespace {

static const char NFC_SOCKET_NAME[] = "/dev/socket/nfcd";



static const uint32_t NFC_TEST_PORT = 6400;

class SendNfcSocketDataTask MOZ_FINAL : public nsRunnable
{
public:
  SendNfcSocketDataTask(NfcConsumer* aConsumer, UnixSocketRawData* aRawData)
    : mConsumer(aConsumer)
    , mRawData(aRawData)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!mConsumer ||
      mConsumer->GetConnectionStatus() != SOCKET_CONNECTED) {
      
      return NS_OK;
    }

    mConsumer->SendSocketData(mRawData.forget());
    return NS_OK;
  }

private:
  NfcConsumer* mConsumer;
  nsAutoPtr<UnixSocketRawData> mRawData;
};

} 

namespace mozilla {
namespace ipc {





int
NfcConnector::Create()
{
  MOZ_ASSERT(!NS_IsMainThread());

  int fd = -1;

#if defined(MOZ_WIDGET_GONK)
  fd = socket(AF_LOCAL, SOCK_STREAM, 0);
#else
  
  fd = socket(AF_INET, SOCK_STREAM, 0);
#endif

  if (fd < 0) {
    NS_WARNING("Could not open nfc socket!");
    return -1;
  }

  if (!SetUp(fd)) {
    NS_WARNING("Could not set up socket!");
  }
  return fd;
}

bool
NfcConnector::CreateAddr(bool aIsServer,
                         socklen_t& aAddrSize,
                         sockaddr_any& aAddr,
                         const char* aAddress)
{
  
  MOZ_ASSERT(!aIsServer);
  uint32_t af;
#if defined(MOZ_WIDGET_GONK)
  af = AF_LOCAL;
#else
  af = AF_INET;
#endif
  switch (af) {
  case AF_LOCAL:
    aAddr.un.sun_family = af;
    if(strlen(aAddress) > sizeof(aAddr.un.sun_path)) {
      NS_WARNING("Address too long for socket struct!");
      return false;
    }
    strcpy((char*)&aAddr.un.sun_path, aAddress);
    aAddrSize = strlen(aAddress) + offsetof(struct sockaddr_un, sun_path) + 1;
    break;
  case AF_INET:
    aAddr.in.sin_family = af;
    aAddr.in.sin_port = htons(NFC_TEST_PORT);
    aAddr.in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    aAddrSize = sizeof(sockaddr_in);
    break;
  default:
    NS_WARNING("Socket type not handled by connector!");
    return false;
  }
  return true;
}

bool
NfcConnector::SetUp(int aFd)
{
  
  return true;
}

bool
NfcConnector::SetUpListenSocket(int aFd)
{
  
  return true;
}

void
NfcConnector::GetSocketAddr(const sockaddr_any& aAddr,
                            nsAString& aAddrStr)
{
  MOZ_CRASH("This should never be called!");
}





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





NfcConsumer::NfcConsumer(NfcSocketListener* aListener)
  : mListener(aListener)
  , mShutdown(false)
{
  mAddress = NFC_SOCKET_NAME;

  Connect(new NfcConnector(), mAddress.get());
}

void
NfcConsumer::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());

  mShutdown = true;
  Close();
}

bool
NfcConsumer::PostToNfcDaemon(const uint8_t* aData, size_t aSize)
{
  MOZ_ASSERT(!NS_IsMainThread());

  UnixSocketRawData* raw = new UnixSocketRawData(aData, aSize);
  nsRefPtr<SendNfcSocketDataTask> task = new SendNfcSocketDataTask(this, raw);
  NS_DispatchToMainThread(task);
  return true;
}

void
NfcConsumer::ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aData)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mListener) {
    mListener->ReceiveSocketData(aData);
  }
}

void
NfcConsumer::OnConnectSuccess()
{
  CHROMIUM_LOG("NFC: %s\n", __FUNCTION__);

  if (mListener) {
    mListener->OnConnectSuccess(NfcSocketListener::STREAM_SOCKET);
  }
  
}

void
NfcConsumer::OnConnectError()
{
  CHROMIUM_LOG("NFC: %s\n", __FUNCTION__);

  if (mListener) {
    mListener->OnConnectError(NfcSocketListener::STREAM_SOCKET);
  }

  Close();
}

void
NfcConsumer::OnDisconnect()
{
  CHROMIUM_LOG("NFC: %s\n", __FUNCTION__);

  if (mListener) {
    mListener->OnDisconnect(NfcSocketListener::STREAM_SOCKET);
  }
  if (!mShutdown) {
    Connect(new NfcConnector(), mAddress.get(), GetSuggestedConnectDelayMs());
  }
}

ConnectionOrientedSocketIO*
NfcConsumer::GetIO()
{
  return PrepareAccept(new NfcConnector());
}

} 
} 
