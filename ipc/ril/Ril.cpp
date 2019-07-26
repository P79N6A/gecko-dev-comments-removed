





#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#undef LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args)
#else
#define LOG(args...)  printf(args);
#endif

#include "jsfriendapi.h"
#include "nsThreadUtils.h" 
#include "Ril.h"

USING_WORKERS_NAMESPACE
using namespace mozilla::ipc;

namespace {

const char* RIL_SOCKET_NAME = "/dev/socket/rilproxy";



const uint32_t RIL_TEST_PORT = 6200;

class DispatchRILEvent : public WorkerTask
{
public:
    DispatchRILEvent(UnixSocketRawData* aMessage)
      : mMessage(aMessage)
    { }

    virtual bool RunTask(JSContext *aCx);

private:
    nsAutoPtr<UnixSocketRawData> mMessage;
};

bool
DispatchRILEvent::RunTask(JSContext *aCx)
{
    JSObject *obj = JS_GetGlobalObject(aCx);

    JSObject *array = JS_NewUint8Array(aCx, mMessage->mSize);
    if (!array) {
        return false;
    }

    memcpy(JS_GetArrayBufferViewData(array), mMessage->mData, mMessage->mSize);
    jsval argv[] = { OBJECT_TO_JSVAL(array) };
    return JS_CallFunctionName(aCx, obj, "onRILMessage", NS_ARRAY_LENGTH(argv),
                               argv, argv);
}

class RilConnector : public mozilla::ipc::UnixSocketConnector
{
public:
  RilConnector(unsigned long aClientId) : mClientId(aClientId)
  {}

  virtual ~RilConnector()
  {}

  virtual int Create();
  virtual void CreateAddr(bool aIsServer,
                          socklen_t& aAddrSize,
                          struct sockaddr *aAddr,
                          const char* aAddress);
  virtual bool SetUp(int aFd);
  virtual void GetSocketAddr(const sockaddr& aAddr,
                             nsAString& aAddrStr);

private:
  unsigned long mClientId;
};

int
RilConnector::Create()
{
    MOZ_ASSERT(!NS_IsMainThread());

    int fd = -1;

#if defined(MOZ_WIDGET_GONK)
    fd = socket(AF_LOCAL, SOCK_STREAM, 0);
#else
    struct hostent *hp;

    hp = gethostbyname("localhost");
    if (hp) {
        fd = socket(hp->h_addrtype, SOCK_STREAM, 0);
    }
#endif

    if (fd < 0) {
        NS_WARNING("Could not open ril socket!");
        return -1;
    }

    if (!SetUp(fd)) {
        NS_WARNING("Could not set up socket!");
    }
    return fd;
}

void
RilConnector::CreateAddr(bool aIsServer,
                         socklen_t& aAddrSize,
                         struct sockaddr *aAddr,
                         const char* aAddress)
{
    
    MOZ_ASSERT(!aIsServer);

#if defined(MOZ_WIDGET_GONK)
    struct sockaddr_un addr_un;

    memset(&addr_un, 0, sizeof(addr_un));
    strcpy(addr_un.sun_path, aAddress);
    addr_un.sun_family = AF_LOCAL;

    aAddrSize = strlen(aAddress) + offsetof(struct sockaddr_un, sun_path) + 1;
    memcpy(aAddr, &addr_un, aAddrSize);
#else
    struct hostent *hp;
    struct sockaddr_in addr_in;

    hp = gethostbyname("localhost");
    if (!hp) {
        return;
    }

    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = hp->h_addrtype;
    addr_in.sin_port = htons(RIL_TEST_PORT + mClientId);
    memcpy(&addr_in.sin_addr, hp->h_addr, hp->h_length);

    aAddrSize = sizeof(addr_in);
    memcpy(aAddr, &addr_in, aAddrSize);
#endif
}

bool
RilConnector::SetUp(int aFd)
{
    
    return true;
}

void
RilConnector::GetSocketAddr(const sockaddr& aAddr,
                            nsAString& aAddrStr)
{
    
    MOZ_NOT_REACHED("This should never be called!");
}

} 

namespace mozilla {
namespace ipc {

RilConsumer::RilConsumer(unsigned long aClientId,
                         WorkerCrossThreadDispatcher* aDispatcher)
    : mDispatcher(aDispatcher)
    , mClientId(aClientId)
    , mShutdown(false)
{
    
    
    if (!aClientId) {
        mAddress = RIL_SOCKET_NAME;
    } else {
        struct sockaddr_un addr_un;
        snprintf(addr_un.sun_path, sizeof addr_un.sun_path, "%s%lu",
                 RIL_SOCKET_NAME, aClientId);
        mAddress = addr_un.sun_path;
    }

    ConnectSocket(new RilConnector(mClientId), mAddress.get());
}

void
RilConsumer::Shutdown()
{
    mShutdown = true;
    CloseSocket();
}

void
RilConsumer::ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage)
{
    MOZ_ASSERT(NS_IsMainThread());

    nsRefPtr<DispatchRILEvent> dre(new DispatchRILEvent(aMessage.forget()));
    mDispatcher->PostTask(dre);
}

void
RilConsumer::OnConnectSuccess()
{
    
    LOG("Socket open for RIL\n");
}

void
RilConsumer::OnConnectError()
{
    LOG("%s\n", __FUNCTION__);
    CloseSocket();
}

void
RilConsumer::OnDisconnect()
{
    LOG("%s\n", __FUNCTION__);
    if (!mShutdown) {
        ConnectSocket(new RilConnector(mClientId), mAddress.get(), 1000);
    }
}

} 
} 
