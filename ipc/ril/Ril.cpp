







































#include <fcntl.h>
#include <unistd.h>

#include <queue>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>

#include "base/eintr_wrapper.h"
#include "base/message_loop.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Monitor.h"
#include "mozilla/Util.h"
#include "nsAutoPtr.h"
#include "nsIThread.h"
#include "nsXULAppAPI.h"
#include "Ril.h"

#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args)
#else
#define LOG(args...)  printf(args);
#endif

using namespace base;
using namespace std;

namespace mozilla {
namespace ipc {

struct RilClient : public RefCounted<RilClient>,
                   public MessageLoopForIO::Watcher

{
    typedef queue<RilRawData*> RilRawDataQueue;

    RilClient() : mSocket(-1)
                , mMutex("RilClient.mMutex")
                , mBlockedOnWrite(false)
                , mCurrentRilRawData(NULL)
    { }
    virtual ~RilClient() { }

    bool OpenSocket();

    virtual void OnFileCanReadWithoutBlocking(int fd);
    virtual void OnFileCanWriteWithoutBlocking(int fd);

    ScopedClose mSocket;
    MessageLoopForIO::FileDescriptorWatcher mReadWatcher;
    MessageLoopForIO::FileDescriptorWatcher mWriteWatcher;
    nsAutoPtr<RilRawData> mIncoming;
    Mutex mMutex;
    RilRawDataQueue mOutgoingQ;
    bool mBlockedOnWrite;
    MessageLoopForIO* mIOLoop;
    nsAutoPtr<RilRawData> mCurrentRilRawData;
    size_t mCurrentWriteOffset;
};

static RefPtr<RilClient> sClient;
static RefPtr<RilConsumer> sConsumer;





class RilWriteTask : public Task {
    virtual void Run();
};

void RilWriteTask::Run() {
    sClient->OnFileCanWriteWithoutBlocking(sClient->mSocket.mFd);
}

static void
ConnectToRil(Monitor* aMonitor, bool* aSuccess)
{
    MOZ_ASSERT(!sClient);

    sClient = new RilClient();
    if (!(*aSuccess = sClient->OpenSocket())) {
        sClient = nsnull;
    }

    {
        MonitorAutoLock lock(*aMonitor);
        lock.Notify();
    }
    
}

bool
RilClient::OpenSocket()
{
#if defined(MOZ_WIDGET_GONK)
    
    
    struct sockaddr_un addr;
    socklen_t alen;
    size_t namelen;
    int err;
    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sun_path, "/dev/socket/rilb2g");
    addr.sun_family = AF_LOCAL;
    mSocket.mFd = socket(AF_LOCAL, SOCK_STREAM, 0);
    alen = strlen("/dev/socket/rilb2g") + offsetof(struct sockaddr_un, sun_path) + 1;
#else
    struct hostent *hp;
    struct sockaddr_in addr;
    socklen_t alen;

    hp = gethostbyname("localhost");
    if (hp == 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = hp->h_addrtype;
    addr.sin_port = htons(6200);
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
    mSocket.mFd = socket(hp->h_addrtype, SOCK_STREAM, 0);
    alen = sizeof(addr);
#endif

    if (mSocket.mFd < 0) {
        LOG("Cannot create socket for RIL!\n");
        return -1;
    }

    if (connect(mSocket.mFd, (struct sockaddr *) &addr, alen) < 0) {
        LOG("Cannot open socket for RIL!\n");
        close(mSocket.mFd);
        return false;
    }
    LOG("Socket open for RIL\n");

    
    int flags = fcntl(mSocket.mFd, F_GETFD);
    if (-1 == flags) {
        return false;
    }

    flags |= FD_CLOEXEC;
    if (-1 == fcntl(mSocket.mFd, F_SETFD, flags)) {
        return false;
    }

    
    if (-1 == fcntl(mSocket.mFd, F_SETFL, O_NONBLOCK)) {
        return false;
    }
    mIOLoop = MessageLoopForIO::current();
    if (!mIOLoop->WatchFileDescriptor(mSocket.mFd,
                                      true,
                                      MessageLoopForIO::WATCH_READ,
                                      &mReadWatcher,
                                      this)) {
        return false;
    }
    return true;
}

void
RilClient::OnFileCanReadWithoutBlocking(int fd)
{
    
    
    
    
    
    
    
    

    MOZ_ASSERT(fd == mSocket.mFd);
    while (true) {
        if (!mIncoming) {
            mIncoming = new RilRawData();
            int ret = read(fd, mIncoming->mData, 1024);
            if (ret <= 0) {
                LOG("Cannot read from network, error %d\n", ret);
                return;
            }
            mIncoming->mSize = ret;
            sConsumer->MessageReceived(mIncoming.forget());
            if (ret < 1024) {
                return;
            }
        }
    }
}

void
RilClient::OnFileCanWriteWithoutBlocking(int fd)
{
    
    
    
    
    
    

    MOZ_ASSERT(fd == mSocket.mFd);

    while (!mOutgoingQ.empty() || mCurrentRilRawData != NULL) {
        if(!mCurrentRilRawData) {
            mCurrentRilRawData = mOutgoingQ.front();
            mOutgoingQ.pop();
            mCurrentWriteOffset = 0;
        }
        const uint8_t *toWrite;

        toWrite = mCurrentRilRawData->mData;
 
        while (mCurrentWriteOffset < mCurrentRilRawData->mSize) {
            ssize_t write_amount = mCurrentRilRawData->mSize - mCurrentWriteOffset;
            ssize_t written;
            written = write (fd, toWrite + mCurrentWriteOffset,
                             write_amount);
            if(written > 0) {
                mCurrentWriteOffset += written;
            }
            if (written != write_amount) {
                break;
            }
        }

        if(mCurrentWriteOffset != mCurrentRilRawData->mSize) {
            MessageLoopForIO::current()->WatchFileDescriptor(
                fd,
                false,
                MessageLoopForIO::WATCH_WRITE,
                &mWriteWatcher,
                this);
            return;
        }
        mCurrentRilRawData = NULL;
    }
}


static void
DisconnectFromRil(Monitor* aMonitor)
{
    
    
    sClient = nsnull;
    {
        MonitorAutoLock lock(*aMonitor);
        lock.Notify();
    }
}





bool
StartRil(RilConsumer* aConsumer)
{
    MOZ_ASSERT(aConsumer);
    sConsumer = aConsumer;

    Monitor monitor("StartRil.monitor");
    bool success;
    {
        MonitorAutoLock lock(monitor);

        XRE_GetIOMessageLoop()->PostTask(
            FROM_HERE,
            NewRunnableFunction(ConnectToRil, &monitor, &success));

        lock.Wait();
    }

    return success;
}

bool
SendRilRawData(RilRawData** aMessage)
{
    if (!sClient) {
        return false;
    }

    RilRawData *msg = *aMessage;
    *aMessage = nsnull;

    {
        MutexAutoLock lock(sClient->mMutex);
        sClient->mOutgoingQ.push(msg);
    }
    sClient->mIOLoop->PostTask(FROM_HERE, new RilWriteTask());

    return true;
}

void
StopRil()
{
    Monitor monitor("StopRil.monitor");
    {
        MonitorAutoLock lock(monitor);

        XRE_GetIOMessageLoop()->PostTask(
            FROM_HERE,
            NewRunnableFunction(DisconnectFromRil, &monitor));

        lock.Wait();
    }

    sConsumer = nsnull;
}


} 
} 
