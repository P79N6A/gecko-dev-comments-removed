






#include "mozilla/ipc/AsyncChannel.h"
#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/ipc/ProtocolUtils.h"

#include "nsDebug.h"
#include "nsTraceRefcnt.h"
#include "nsXULAppAPI.h"

using namespace mozilla;
using namespace std;

template<>
struct RunnableMethodTraits<mozilla::ipc::AsyncChannel>
{
    static void RetainCallee(mozilla::ipc::AsyncChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::AsyncChannel* obj) { }
};

template<>
struct RunnableMethodTraits<mozilla::ipc::AsyncChannel::ProcessLink>
{
    static void RetainCallee(mozilla::ipc::AsyncChannel::ProcessLink* obj) { }
    static void ReleaseCallee(mozilla::ipc::AsyncChannel::ProcessLink* obj) { }
};













template<>
struct RunnableMethodTraits<mozilla::ipc::AsyncChannel::Transport>
{
    static void RetainCallee(mozilla::ipc::AsyncChannel::Transport* obj) { }
    static void ReleaseCallee(mozilla::ipc::AsyncChannel::Transport* obj) { }
};

namespace {


class GoodbyeMessage : public IPC::Message
{
public:
    enum { ID = GOODBYE_MESSAGE_TYPE };
    GoodbyeMessage() :
        IPC::Message(MSG_ROUTING_NONE, ID, PRIORITY_NORMAL)
    {
    }
    
    
    static bool Read(const Message* msg)
    {
        return true;
    }
    void Log(const std::string& aPrefix,
             FILE* aOutf) const
    {
        fputs("(special `Goodbye' message)", aOutf);
    }
};

} 

namespace mozilla {
namespace ipc {

AsyncChannel::Link::Link(AsyncChannel *aChan)
    : mChan(aChan)
{
}

AsyncChannel::Link::~Link()
{
    mChan = 0;
}

AsyncChannel::ProcessLink::ProcessLink(AsyncChannel *aChan)
    : Link(aChan)
    , mExistingListener(NULL)
{
}

AsyncChannel::ProcessLink::~ProcessLink()
{
    mIOLoop = 0;
    if (mTransport) {
        mTransport->set_listener(0);
        
        
        
        mTransport = 0;
    }
}

void 
AsyncChannel::ProcessLink::Open(mozilla::ipc::Transport* aTransport,
                                MessageLoop *aIOLoop,
                                Side aSide)
{
    NS_PRECONDITION(aTransport, "need transport layer");

    

    mTransport = aTransport;

    
    
    bool needOpen = true;
    if(aIOLoop) {
        
        
        needOpen = true;
        mChan->mChild = (aSide == AsyncChannel::Unknown) || (aSide == AsyncChannel::Child);
    } else {
        NS_PRECONDITION(aSide == Unknown, "expected default side arg");

        
        mChan->mChild = false;
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

        
        while (!mChan->Connected()) {
            mChan->mMonitor->Wait();
        }
    }
}

void
AsyncChannel::ProcessLink::EchoMessage(Message *msg)
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    
    
    
    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &ProcessLink::OnEchoMessage, msg));
    
}

void
AsyncChannel::ProcessLink::SendMessage(Message *msg)
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(mTransport, &Transport::Send, msg));
}

void
AsyncChannel::ProcessLink::SendClose()
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    mIOLoop->PostTask(
        FROM_HERE, NewRunnableMethod(this, &ProcessLink::OnCloseChannel));
}

AsyncChannel::ThreadLink::ThreadLink(AsyncChannel *aChan,
                                     AsyncChannel *aTargetChan)
    : Link(aChan),
      mTargetChan(aTargetChan)
{
}

AsyncChannel::ThreadLink::~ThreadLink()
{
    mTargetChan = 0;
}

void
AsyncChannel::ThreadLink::EchoMessage(Message *msg)
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    mChan->OnMessageReceivedFromLink(*msg);
    delete msg;
}

void
AsyncChannel::ThreadLink::SendMessage(Message *msg)
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    mTargetChan->OnMessageReceivedFromLink(*msg);
    delete msg;
}

void
AsyncChannel::ThreadLink::SendClose()
{
    mChan->AssertWorkerThread();
    mChan->mMonitor->AssertCurrentThreadOwns();

    mChan->mChannelState = ChannelClosed;

    
    
    
    
    
    mTargetChan->OnChannelErrorFromLink();
}

AsyncChannel::AsyncChannel(AsyncListener* aListener)
  : mListener(aListener),
    mChannelState(ChannelClosed),
    mWorkerLoop(),
    mChild(false),
    mChannelErrorTask(NULL),
    mLink(NULL)
{
    MOZ_COUNT_CTOR(AsyncChannel);
}

AsyncChannel::~AsyncChannel()
{
    MOZ_COUNT_DTOR(AsyncChannel);
    Clear();
}

bool
AsyncChannel::Open(Transport* aTransport,
                   MessageLoop* aIOLoop,
                   AsyncChannel::Side aSide)
{
    ProcessLink *link;
    NS_PRECONDITION(!mLink, "Open() called > once");
    mMonitor = new RefCountedMonitor();
    mWorkerLoop = MessageLoop::current();
    mLink = link = new ProcessLink(this);
    link->Open(aTransport, aIOLoop, aSide); 
    return true;
}

















bool
AsyncChannel::Open(AsyncChannel *aTargetChan, 
                   MessageLoop *aTargetLoop,
                   AsyncChannel::Side aSide)
{
    NS_PRECONDITION(aTargetChan, "Need a target channel");
    NS_PRECONDITION(ChannelClosed == mChannelState, "Not currently closed");

    CommonThreadOpenInit(aTargetChan, aSide);

    Side oppSide = Unknown;
    switch(aSide) {
      case Child: oppSide = Parent; break;
      case Parent: oppSide = Child; break;
      case Unknown: break;
    }

    mMonitor = new RefCountedMonitor();

    MonitorAutoLock lock(*mMonitor);
    mChannelState = ChannelOpening;
    aTargetLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(aTargetChan, &AsyncChannel::OnOpenAsSlave,
                          this, oppSide));

    while (ChannelOpening == mChannelState)
        mMonitor->Wait();
    NS_ASSERTION(ChannelConnected == mChannelState, "not connected when awoken");
    return (ChannelConnected == mChannelState);
}

void 
AsyncChannel::CommonThreadOpenInit(AsyncChannel *aTargetChan, Side aSide)
{
    mWorkerLoop = MessageLoop::current();
    mLink = new ThreadLink(this, aTargetChan);
    mChild = (aSide == Child); 
}


void
AsyncChannel::OnOpenAsSlave(AsyncChannel *aTargetChan, Side aSide)
{
    NS_PRECONDITION(ChannelClosed == mChannelState, 
                    "Not currently closed");
    NS_PRECONDITION(ChannelOpening == aTargetChan->mChannelState,
                    "Target channel not in the process of opening");
    
    CommonThreadOpenInit(aTargetChan, aSide);
    mMonitor = aTargetChan->mMonitor;

    MonitorAutoLock lock(*mMonitor);
    NS_ASSERTION(ChannelOpening == aTargetChan->mChannelState,
                 "Target channel not in the process of opening");
    mChannelState = ChannelConnected;
    aTargetChan->mChannelState = ChannelConnected;
    aTargetChan->mMonitor->Notify();
}

void
AsyncChannel::Close()
{
    AssertWorkerThread();

    {
        
        
        
        
        nsRefPtr<RefCountedMonitor> monitor(mMonitor);
        MonitorAutoLock lock(*monitor);

        if (ChannelError == mChannelState ||
            ChannelTimeout == mChannelState) {
            
            
            
            
            
            if (mListener) {
                MonitorAutoUnlock unlock(*monitor);
                NotifyMaybeChannelError();
            }
            return;
        }

        if (ChannelConnected != mChannelState)
            
            
            NS_RUNTIMEABORT("Close() called on closed channel!");

        AssertWorkerThread();

        
        SendSpecialMessage(new GoodbyeMessage());

        SynchronouslyClose();
    }

    NotifyChannelClosed();
}

void 
AsyncChannel::SynchronouslyClose()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();
    mLink->SendClose();
    while (ChannelClosed != mChannelState)
        mMonitor->Wait();
}

bool
AsyncChannel::Send(Message* _msg)
{
    nsAutoPtr<Message> msg(_msg);
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();
    NS_ABORT_IF_FALSE(MSG_ROUTING_NONE != msg->routing_id(), "need a route");

    {
        MonitorAutoLock lock(*mMonitor);

        if (!Connected()) {
            ReportConnectionError("AsyncChannel");
            return false;
        }

        mLink->SendMessage(msg.forget());
    }

    return true;
}

bool
AsyncChannel::Echo(Message* _msg)
{
    nsAutoPtr<Message> msg(_msg);
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();
    NS_ABORT_IF_FALSE(MSG_ROUTING_NONE != msg->routing_id(), "need a route");

    {
        MonitorAutoLock lock(*mMonitor);

        if (!Connected()) {
            ReportConnectionError("AsyncChannel");
            return false;
        }

        mLink->EchoMessage(msg.forget());
    }

    return true;
}

void
AsyncChannel::OnDispatchMessage(const Message& msg)
{
    AssertWorkerThread();
    NS_ASSERTION(!msg.is_reply(), "can't process replies here");
    NS_ASSERTION(!(msg.is_sync() || msg.is_rpc()), "async dispatch only");

    if (MSG_ROUTING_NONE == msg.routing_id()) {
        if (!OnSpecialMessage(msg.type(), msg))
            
            NS_RUNTIMEABORT("unhandled special message!");
        return;
    }

    
    

    (void)MaybeHandleError(mListener->OnMessageReceived(msg), "AsyncChannel");
}

bool
AsyncChannel::OnSpecialMessage(uint16_t id, const Message& msg)
{
    return false;
}

void
AsyncChannel::SendSpecialMessage(Message* msg) const
{
    AssertWorkerThread();
    mLink->SendMessage(msg);
}

void
AsyncChannel::OnNotifyMaybeChannelError()
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

    mChannelErrorTask = NULL;

    
    
    
    
    {
        MonitorAutoLock lock(*mMonitor);
        
    }

    if (ShouldDeferNotifyMaybeError()) {
        mChannelErrorTask =
            NewRunnableMethod(this, &AsyncChannel::OnNotifyMaybeChannelError);
        
        mWorkerLoop->PostDelayedTask(FROM_HERE, mChannelErrorTask, 10);
        return;
    }

    NotifyMaybeChannelError();
}

void
AsyncChannel::NotifyChannelClosed()
{
    mMonitor->AssertNotCurrentThreadOwns();

    if (ChannelClosed != mChannelState)
        NS_RUNTIMEABORT("channel should have been closed!");

    
    
    mListener->OnChannelClose();

    Clear();
}

void
AsyncChannel::NotifyMaybeChannelError()
{
    mMonitor->AssertNotCurrentThreadOwns();

    
    
    if (ChannelClosing == mChannelState) {
        
        
        mChannelState = ChannelClosed;
        NotifyChannelClosed();
        return;
    }

    
    mChannelState = ChannelError;
    mListener->OnChannelError();

    Clear();
}

void
AsyncChannel::Clear()
{
    mListener = 0;
    mWorkerLoop = 0;

    delete mLink;
    mLink = 0;
    mMonitor = 0;

    if (mChannelErrorTask) {
        mChannelErrorTask->Cancel();
        mChannelErrorTask = NULL;
    }
}

static void
PrintErrorMessage(bool isChild, const char* channelName, const char* msg)
{
    printf_stderr("\n###!!! [%s][%s] Error: %s\n\n",
                  isChild ? "Child" : "Parent", channelName, msg);
}

bool
AsyncChannel::MaybeHandleError(Result code, const char* channelName)
{
    if (MsgProcessed == code)
        return true;

    const char* errorMsg = nullptr;
    switch (code) {
    case MsgNotKnown:
        errorMsg = "Unknown message: not processed";
        break;
    case MsgNotAllowed:
        errorMsg = "Message not allowed: cannot be sent/recvd in this state";
        break;
    case MsgPayloadError:
        errorMsg = "Payload error: message could not be deserialized";
        break;
    case MsgProcessingError:
        errorMsg = "Processing error: message was deserialized, but the handler returned false (indicating failure)";
        break;
    case MsgRouteError:
        errorMsg = "Route error: message sent to unknown actor ID";
        break;
    case MsgValueError:
        errorMsg = "Value error: message was deserialized, but contained an illegal value";
        break;

    default:
        NS_RUNTIMEABORT("unknown Result code");
        return false;
    }

    PrintErrorMessage(mChild, channelName, errorMsg);

    mListener->OnProcessingError(code);

    return false;
}

void
AsyncChannel::ReportConnectionError(const char* channelName) const
{
    const char* errorMsg = nullptr;
    switch (mChannelState) {
    case ChannelClosed:
        errorMsg = "Closed channel: cannot send/recv";
        break;
    case ChannelOpening:
        errorMsg = "Opening channel: not yet ready for send/recv";
        break;
    case ChannelTimeout:
        errorMsg = "Channel timeout: cannot send/recv";
        break;
    case ChannelClosing:
        errorMsg = "Channel closing: too late to send/recv, messages will be lost";
        break;
    case ChannelError:
        errorMsg = "Channel error: cannot send/recv";
        break;

    default:
        NS_RUNTIMEABORT("unreached");
    }

    PrintErrorMessage(mChild, channelName, errorMsg);

    mListener->OnProcessingError(MsgDropped);
}

void
AsyncChannel::DispatchOnChannelConnected(int32_t peer_pid)
{
    AssertWorkerThread();
    if (mListener)
        mListener->OnChannelConnected(peer_pid);
}





void
AsyncChannel::ProcessLink::OnMessageReceived(const Message& msg)
{
    AssertIOThread();
    NS_ASSERTION(mChan->mChannelState != ChannelError, "Shouldn't get here!");
    MonitorAutoLock lock(*mChan->mMonitor);
    mChan->OnMessageReceivedFromLink(msg);
}

void
AsyncChannel::ProcessLink::OnEchoMessage(Message* msg)
{
    AssertIOThread();
    OnMessageReceived(*msg);
    delete msg;
}

void
AsyncChannel::ProcessLink::OnChannelOpened()
{
    mChan->AssertLinkThread();
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
AsyncChannel::ProcessLink::OnTakeConnectedChannel()
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
AsyncChannel::ProcessLink::OnChannelConnected(int32_t peer_pid)
{
    AssertIOThread();

    {
        MonitorAutoLock lock(*mChan->mMonitor);
        mChan->mChannelState = ChannelConnected;
        mChan->mMonitor->Notify();
    }

    if(mExistingListener)
        mExistingListener->OnChannelConnected(peer_pid);

    mChan->mWorkerLoop->PostTask(
        FROM_HERE, 
        NewRunnableMethod(mChan, 
                          &AsyncChannel::DispatchOnChannelConnected, 
                          peer_pid));
}

void
AsyncChannel::ProcessLink::OnChannelError()
{
    AssertIOThread();
    MonitorAutoLock lock(*mChan->mMonitor);
    mChan->OnChannelErrorFromLink();
}

void
AsyncChannel::ProcessLink::OnCloseChannel()
{
    AssertIOThread();

    mTransport->Close();

    MonitorAutoLock lock(*mChan->mMonitor);
    mChan->mChannelState = ChannelClosed;
    mChan->mMonitor->Notify();
}





void
AsyncChannel::OnMessageReceivedFromLink(const Message& msg)
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (!MaybeInterceptSpecialIOMessage(msg))
        
        mWorkerLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &AsyncChannel::OnDispatchMessage, msg));
}

void
AsyncChannel::OnChannelErrorFromLink()
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (ChannelClosing != mChannelState)
        mChannelState = ChannelError;

    PostErrorNotifyTask();
}

void
AsyncChannel::CloseWithError()
{
    AssertWorkerThread();

    MonitorAutoLock lock(*mMonitor);
    if (ChannelConnected != mChannelState) {
        return;
    }
    SynchronouslyClose();
    mChannelState = ChannelError;
    PostErrorNotifyTask();
}

void
AsyncChannel::PostErrorNotifyTask()
{
    mMonitor->AssertCurrentThreadOwns();

    if (mChannelErrorTask)
        return;

    
    mChannelErrorTask =
        NewRunnableMethod(this, &AsyncChannel::OnNotifyMaybeChannelError);
    mWorkerLoop->PostTask(FROM_HERE, mChannelErrorTask);
}

bool
AsyncChannel::MaybeInterceptSpecialIOMessage(const Message& msg)
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (MSG_ROUTING_NONE == msg.routing_id()
        && GOODBYE_MESSAGE_TYPE == msg.type()) {
        ProcessGoodbyeMessage();
        return true;
    }
    return false;
}

void
AsyncChannel::ProcessGoodbyeMessage()
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    
    
    mChannelState = ChannelClosing;

    printf("NOTE: %s process received `Goodbye', closing down\n",
           mChild ? "child" : "parent");
}


} 
} 
