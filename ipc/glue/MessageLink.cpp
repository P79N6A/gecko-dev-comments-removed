






#include "mozilla/ipc/MessageLink.h"
#include "mozilla/ipc/MessageChannel.h"
#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/ipc/ProtocolUtils.h"

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/ContentParent.h"
#endif

#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "nsDebug.h"
#include "nsISupportsImpl.h"
#include "nsXULAppAPI.h"

using namespace mozilla;
using namespace std;

template<>
struct RunnableMethodTraits<mozilla::ipc::ProcessLink>
{
    static void RetainCallee(mozilla::ipc::ProcessLink* obj) { }
    static void ReleaseCallee(mozilla::ipc::ProcessLink* obj) { }
};













template<>
struct RunnableMethodTraits<mozilla::ipc::MessageChannel::Transport>
{
    static void RetainCallee(mozilla::ipc::MessageChannel::Transport* obj) { }
    static void ReleaseCallee(mozilla::ipc::MessageChannel::Transport* obj) { }
};

namespace mozilla {
namespace ipc {

MessageLink::MessageLink(MessageChannel *aChan)
  : mChan(aChan)
{
}

MessageLink::~MessageLink()
{
#ifdef DEBUG
    mChan = nullptr;
#endif
}

ProcessLink::ProcessLink(MessageChannel *aChan)
  : MessageLink(aChan)
  , mTransport(nullptr)
  , mIOLoop(nullptr)
  , mExistingListener(nullptr)
#ifdef MOZ_NUWA_PROCESS
  , mIsToNuwaProcess(false)
#endif
{
}

ProcessLink::~ProcessLink()
{
#ifdef DEBUG
    mTransport = nullptr;
    mIOLoop = nullptr;
    mExistingListener = nullptr;
#endif
}

void 
ProcessLink::Open(mozilla::ipc::Transport* aTransport, MessageLoop *aIOLoop, Side aSide)
{
    NS_PRECONDITION(aTransport, "need transport layer");

    

    mTransport = aTransport;

    
    
    bool needOpen = true;
    if(aIOLoop) {
        
        
        needOpen = true;
        mChan->mSide = (aSide == UnknownSide) ? ChildSide : aSide;
    } else {
        NS_PRECONDITION(aSide == UnknownSide, "expected default side arg");

        
        mChan->mSide = ParentSide;
        needOpen = false;
        aIOLoop = XRE_GetIOMessageLoop();
    }

    mIOLoop = aIOLoop;

    NS_ASSERTION(mIOLoop, "need an IO loop");
    NS_ASSERTION(mChan->mWorkerLoop, "need a worker loop");

    {
        MonitorAutoLock lock(*mChan->mMonitor);

        if (needOpen) {
            
            
            
            mIOLoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &ProcessLink::OnChannelOpened));
        } else {
            
            
            
            mIOLoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &ProcessLink::OnTakeConnectedChannel));
        }

#ifdef MOZ_NUWA_PROCESS
        if (IsNuwaProcess() &&
            Preferences::GetBool("dom.ipc.processPrelaunch.testMode")) {
            
            
            
            
            sleep(5);
        }
#endif

        
        while (!mChan->Connected() && mChan->mChannelState != ChannelError) {
            mChan->mMonitor->Wait();
        }
    }
}

void
ProcessLink::EchoMessage(Message *msg)
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &ProcessLink::OnEchoMessage, msg));
    
}

void
ProcessLink::SendMessage(Message *msg)
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

#ifdef MOZ_NUWA_PROCESS
    if (mIsToNuwaProcess && mozilla::dom::ContentParent::IsNuwaReady()) {
        switch (msg->type()) {
        case mozilla::dom::PContent::Msg_NuwaFork__ID:
        case mozilla::dom::PContent::Reply_AddNewProcess__ID:
        case mozilla::dom::PContent::Msg_NotifyPhoneStateChange__ID:
        case GOODBYE_MESSAGE_TYPE:
            break;
        default:
#ifdef DEBUG
            MOZ_CRASH();
#else
            
            printf_stderr("Sending message to frozen Nuwa");
            return;
#endif
        }
    }
#endif

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(mTransport, &Transport::Send, msg));
}

void
ProcessLink::SendClose()
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    mIOLoop->PostTask(
        FROM_HERE, NewRunnableMethod(this, &ProcessLink::OnCloseChannel));
}

ThreadLink::ThreadLink(MessageChannel *aChan, MessageChannel *aTargetChan)
  : MessageLink(aChan),
    mTargetChan(aTargetChan)
{
}

ThreadLink::~ThreadLink()
{
    MOZ_ASSERT(mChan);
    MOZ_ASSERT(mChan->mMonitor);
    MonitorAutoLock lock(*mChan->mMonitor);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (mTargetChan) {
        MOZ_ASSERT(mTargetChan->mLink);
        static_cast<ThreadLink*>(mTargetChan->mLink)->mTargetChan = nullptr;
    }
    mTargetChan = nullptr;
}

void
ThreadLink::EchoMessage(Message *msg)
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    mChan->OnMessageReceivedFromLink(*msg);
    delete msg;
}

void
ThreadLink::SendMessage(Message *msg)
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    if (mTargetChan)
        mTargetChan->OnMessageReceivedFromLink(*msg);
    delete msg;
}

void
ThreadLink::SendClose()
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    mChan->mChannelState = ChannelClosed;

    
    
    
    
    
    if (mTargetChan)
        mTargetChan->OnChannelErrorFromLink();
}

bool
ThreadLink::Unsound_IsClosed() const
{
    MonitorAutoLock lock(*mChan->mMonitor);
    return mChan->mChannelState == ChannelClosed;
}

uint32_t
ThreadLink::Unsound_NumQueuedMessages() const
{
    
    return 0;
}





void
ProcessLink::OnMessageReceived(const Message& msg)
{
    AssertIOThread();
    NS_ASSERTION(mChan->mChannelState != ChannelError, "Shouldn't get here!");
    MonitorAutoLock lock(*mChan->mMonitor);
    mChan->OnMessageReceivedFromLink(msg);
}

void
ProcessLink::OnEchoMessage(Message* msg)
{
    AssertIOThread();
    OnMessageReceived(*msg);
    delete msg;
}

void
ProcessLink::OnChannelOpened()
{
    AssertIOThread();

    {
        MonitorAutoLock lock(*mChan->mMonitor);

        mExistingListener = mTransport->set_listener(this);
#ifdef DEBUG
        if (mExistingListener) {
            queue<Message> pending;
            mExistingListener->GetQueuedMessages(pending);
            MOZ_ASSERT(pending.empty());
        }
#endif  

        mChan->mChannelState = ChannelOpening;
        lock.Notify();
    }
    mTransport->Connect();
}

void
ProcessLink::OnTakeConnectedChannel()
{
    AssertIOThread();

    queue<Message> pending;
    {
        MonitorAutoLock lock(*mChan->mMonitor);

        mChan->mChannelState = ChannelConnected;

        mExistingListener = mTransport->set_listener(this);
        if (mExistingListener) {
            mExistingListener->GetQueuedMessages(pending);
        }
        lock.Notify();
    }

    
    while (!pending.empty()) {
        OnMessageReceived(pending.front());
        pending.pop();
    }
}

void
ProcessLink::OnChannelConnected(int32_t peer_pid)
{
    AssertIOThread();

    bool notifyChannel = false;

    {
        MonitorAutoLock lock(*mChan->mMonitor);
        
        
        if (mChan->mChannelState == ChannelOpening) {
          mChan->mChannelState = ChannelConnected;
          mChan->mMonitor->Notify();
          notifyChannel = true;
        }
    }

    if (mExistingListener)
        mExistingListener->OnChannelConnected(peer_pid);

#ifdef MOZ_NUWA_PROCESS
    mIsToNuwaProcess = (peer_pid == mozilla::dom::ContentParent::NuwaPid());
#endif

    if (notifyChannel) {
      mChan->OnChannelConnected(peer_pid);
    }
}

void
ProcessLink::OnChannelError()
{
    AssertIOThread();

    MonitorAutoLock lock(*mChan->mMonitor);

    MOZ_ALWAYS_TRUE(this == mTransport->set_listener(mExistingListener));

    mChan->OnChannelErrorFromLink();
}

void
ProcessLink::OnCloseChannel()
{
    AssertIOThread();

    mTransport->Close();

    MonitorAutoLock lock(*mChan->mMonitor);

    DebugOnly<IPC::Channel::Listener*> previousListener =
      mTransport->set_listener(mExistingListener);

    
    MOZ_ASSERT(previousListener == this ||
               previousListener == mExistingListener);

    mChan->mChannelState = ChannelClosed;
    mChan->mMonitor->Notify();
}

bool
ProcessLink::Unsound_IsClosed() const
{
    return mTransport->Unsound_IsClosed();
}

uint32_t
ProcessLink::Unsound_NumQueuedMessages() const
{
    return mTransport->Unsound_NumQueuedMessages();
}

} 
} 
