





#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h> 

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
    JS::Value argv[] = { OBJECT_TO_JSVAL(array) };
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
  virtual bool CreateAddr(bool aIsServer,
                          socklen_t& aAddrSize,
                          sockaddr_any& aAddr,
                          const char* aAddress);
  virtual bool SetUp(int aFd);
  virtual void GetSocketAddr(const sockaddr_any& aAddr,
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
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
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

bool
RilConnector::CreateAddr(bool aIsServer,
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
        aAddr.in.sin_port = htons(RIL_TEST_PORT + mClientId);
        aAddr.in.sin_addr.s_addr = htons(INADDR_LOOPBACK);
        aAddrSize = sizeof(sockaddr_in);
        break;
    default:
        NS_WARNING("Socket type not handled by connector!");
        return false;
    }
    return true;
}

bool
RilConnector::SetUp(int aFd)
{
    
    return true;
}

void
RilConnector::GetSocketAddr(const sockaddr_any& aAddr,
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
