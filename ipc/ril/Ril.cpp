





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

#undef LOG
#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk", args)
#else
#define LOG(args...)  printf(args);
#endif

#define RIL_SOCKET_NAME "/dev/socket/rilproxy"

using namespace base;
using namespace std;



const uint32_t RIL_TEST_PORT = 6200;

namespace mozilla {
namespace ipc {

struct RilClient : public RefCounted<RilClient>,
                   public MessageLoopForIO::Watcher

{
    typedef queue<RilRawData*> RilRawDataQueue;

    RilClient() : mSocket(-1)
                , mMutex("RilClient.mMutex")
                , mBlockedOnWrite(false)
                , mIOLoop(MessageLoopForIO::current())
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





class RilReconnectTask : public CancelableTask {
    RilReconnectTask() : mCanceled(false) { }

    virtual void Run();
    virtual void Cancel() { mCanceled = true; }

    bool mCanceled;

public:
    static void Enqueue(int aDelayMs = 0) {
        MessageLoopForIO* ioLoop = MessageLoopForIO::current();
        MOZ_ASSERT(ioLoop && sClient->mIOLoop == ioLoop);
        if (sTask) {
            return;
        }
        sTask = new RilReconnectTask();
        if (aDelayMs) {
            ioLoop->PostDelayedTask(FROM_HERE, sTask, aDelayMs);
        } else {
            ioLoop->PostTask(FROM_HERE, sTask);
        }
    }

    static void CancelIt() {
        if (!sTask) {
            return;
        }
        sTask->Cancel();
        sTask = nullptr;
    }

private:
    
    
    static CancelableTask* sTask;
};
CancelableTask* RilReconnectTask::sTask;

void RilReconnectTask::Run() {
    
    
    
    sTask = nullptr;
    if (mCanceled) {
        if (sClient->OpenSocket()) {
            return;
        }
    }
    Enqueue(1000);
}

class RilWriteTask : public Task
{
    virtual void Run();
};

void RilWriteTask::Run()
{
    if(sClient->mSocket.get() < 0) {
        NS_WARNING("Trying to write to non-open socket!");
        return;
    }
    sClient->OnFileCanWriteWithoutBlocking(sClient->mSocket.rwget());
}

static void
ConnectToRil(Monitor* aMonitor, bool* aSuccess)
{
    MOZ_ASSERT(!sClient);

    sClient = new RilClient();
    RilReconnectTask::Enqueue();
    *aSuccess = true;
    {
        MonitorAutoLock lock(*aMonitor);
        lock.Notify();
    }
    
}

bool
RilClient::OpenSocket()
{
    ScopedClose sck;
#if defined(MOZ_WIDGET_GONK)
    
    
    struct sockaddr_un addr;
    socklen_t alen;
    size_t namelen;
    int err;
    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sun_path, RIL_SOCKET_NAME);
    addr.sun_family = AF_LOCAL;
    sck = socket(AF_LOCAL, SOCK_STREAM, 0);
    alen = strlen(RIL_SOCKET_NAME) + offsetof(struct sockaddr_un, sun_path) + 1;
#else
    struct hostent *hp;
    struct sockaddr_in addr;
    socklen_t alen;

    hp = gethostbyname("localhost");
    if (hp == 0) return false;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = hp->h_addrtype;
    addr.sin_port = htons(RIL_TEST_PORT);
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
    sck = socket(hp->h_addrtype, SOCK_STREAM, 0);
    alen = sizeof(addr);
#endif

    if (sck < 0) {
        LOG("Cannot create socket for RIL!\n");
        return false;
    }

    if (connect(sck, (struct sockaddr *) &addr, alen) < 0) {
#if defined(MOZ_WIDGET_GONK)
        LOG("Cannot open socket for RIL!\n");
#endif
        return false;
    }

    
    int flags = fcntl(sck, F_GETFD);
    if (-1 == flags) {
        return false;
    }

    flags |= FD_CLOEXEC;
    if (-1 == fcntl(sck, F_SETFD, flags)) {
        return false;
    }

    
    if (-1 == fcntl(sck, F_SETFL, O_NONBLOCK)) {
        return false;
    }
    if (!mIOLoop->WatchFileDescriptor(sck,
                                      true,
                                      MessageLoopForIO::WATCH_READ,
                                      &mReadWatcher,
                                      this)) {
        return false;
    }
    mSocket.reset(sck.forget());
    LOG("Socket open for RIL\n");
    return true;
}

void
RilClient::OnFileCanReadWithoutBlocking(int fd)
{
    
    
    
    
    
    
    
    

    MOZ_ASSERT(fd == mSocket.get());
    while (true) {
        if (!mIncoming) {
            mIncoming = new RilRawData();
            ssize_t ret = read(fd, mIncoming->mData, RilRawData::MAX_DATA_SIZE);
            if (ret <= 0) {
                if (ret == -1) {
                    if (errno == EINTR) {
                        continue; 
                    }
                    else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        return; 
                    }
                    
                }
                LOG("Cannot read from network, error %d\n", ret);
                
                
                mIncoming.forget();
                mReadWatcher.StopWatchingFileDescriptor();
                mWriteWatcher.StopWatchingFileDescriptor();
                mSocket.reset(-1);
                RilReconnectTask::Enqueue();
                return;
            }
            mIncoming->mSize = ret;
            sConsumer->MessageReceived(mIncoming.forget());
            if (ret < ssize_t(RilRawData::MAX_DATA_SIZE)) {
                return;
            }
        }
    }
}

void
RilClient::OnFileCanWriteWithoutBlocking(int fd)
{
    
    
    
    
    
    

    MOZ_ASSERT(fd == mSocket.get());

    while (true) {
        {
            MutexAutoLock lock(mMutex);

            if (mOutgoingQ.empty() && !mCurrentRilRawData) {
                return;
            }

            if(!mCurrentRilRawData) {
                mCurrentRilRawData = mOutgoingQ.front();
                mOutgoingQ.pop();
                mCurrentWriteOffset = 0;
            }
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
    
    
    RilReconnectTask::CancelIt();
    
    
    sClient = nullptr;
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
    *aMessage = nullptr;

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

    sConsumer = nullptr;
}


} 
} 
