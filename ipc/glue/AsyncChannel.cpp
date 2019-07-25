






































#include "mozilla/ipc/AsyncChannel.h"
#include "mozilla/ipc/BrowserProcessSubThread.h"
#include "mozilla/ipc/ProtocolUtils.h"

#include "nsDebug.h"
#include "nsTraceRefcnt.h"
#include "nsXULAppAPI.h"

using mozilla::MonitorAutoLock;

template<>
struct RunnableMethodTraits<mozilla::ipc::AsyncChannel>
{
    static void RetainCallee(mozilla::ipc::AsyncChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::AsyncChannel* obj) { }
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

AsyncChannel::AsyncChannel(AsyncListener* aListener)
  : mTransport(0),
    mListener(aListener),
    mChannelState(ChannelClosed),
    mMonitor("mozilla.ipc.AsyncChannel.mMonitor"),
    mIOLoop(),
    mWorkerLoop(),
    mChild(false),
    mChannelErrorTask(NULL),
    mExistingListener(NULL)
{
    MOZ_COUNT_CTOR(AsyncChannel);
}

AsyncChannel::~AsyncChannel()
{
    MOZ_COUNT_DTOR(AsyncChannel);
    Clear();
}

bool
AsyncChannel::Open(Transport* aTransport, MessageLoop* aIOLoop)
{
    NS_PRECONDITION(!mTransport, "Open() called > once");
    NS_PRECONDITION(aTransport, "need transport layer");

    

    mTransport = aTransport;
    mExistingListener = mTransport->set_listener(this);

    
    
    bool needOpen = true;
    if(!aIOLoop) {
        
        needOpen = false;
        aIOLoop = XRE_GetIOMessageLoop();
        
        
        mChannelState = ChannelConnected;
    }

    mChild = needOpen;

    mIOLoop = aIOLoop;
    mWorkerLoop = MessageLoop::current();

    NS_ASSERTION(mIOLoop, "need an IO loop");
    NS_ASSERTION(mWorkerLoop, "need a worker loop");

    if (needOpen) {             
        MonitorAutoLock lock(mMonitor);

        mIOLoop->PostTask(FROM_HERE, 
                          NewRunnableMethod(this,
                                            &AsyncChannel::OnChannelOpened));

        
        while (mChannelState != ChannelConnected) {
            mMonitor.Wait();
        }
    }

    return true;
}

void
AsyncChannel::Close()
{
    AssertWorkerThread();

    {
        MonitorAutoLock lock(mMonitor);

        if (ChannelError == mChannelState ||
            ChannelTimeout == mChannelState) {
            
            
            
            
            
            if (mListener) {
                MonitorAutoUnlock unlock(mMonitor);
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
    mMonitor.AssertCurrentThreadOwns();

    mIOLoop->PostTask(
        FROM_HERE, NewRunnableMethod(this, &AsyncChannel::OnCloseChannel));

    while (ChannelClosed != mChannelState)
        mMonitor.Wait();
}

bool
AsyncChannel::Send(Message* msg)
{
    AssertWorkerThread();
    mMonitor.AssertNotCurrentThreadOwns();
    NS_ABORT_IF_FALSE(MSG_ROUTING_NONE != msg->routing_id(), "need a route");

    {
        MonitorAutoLock lock(mMonitor);

        if (!Connected()) {
            ReportConnectionError("AsyncChannel");
            return false;
        }

        SendThroughTransport(msg);
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
AsyncChannel::OnSpecialMessage(uint16 id, const Message& msg)
{
    return false;
}

void
AsyncChannel::SendSpecialMessage(Message* msg) const
{
    AssertWorkerThread();
    SendThroughTransport(msg);
}

void
AsyncChannel::SendThroughTransport(Message* msg) const
{
    AssertWorkerThread();

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(mTransport, &Transport::Send, msg));
}

void
AsyncChannel::OnNotifyMaybeChannelError()
{
    AssertWorkerThread();
    mMonitor.AssertNotCurrentThreadOwns();

    
    
    
    
    {
        MonitorAutoLock lock(mMonitor);
        
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
    mMonitor.AssertNotCurrentThreadOwns();

    if (ChannelClosed != mChannelState)
        NS_RUNTIMEABORT("channel should have been closed!");

    
    
    mListener->OnChannelClose();

    Clear();
}

void
AsyncChannel::NotifyMaybeChannelError()
{
    mMonitor.AssertNotCurrentThreadOwns();

    
    
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
    mIOLoop = 0;
    mWorkerLoop = 0;

    if (mTransport) {
        mTransport->set_listener(0);

        
        
        mTransport = 0;
    }
    if (mChannelErrorTask) {
        mChannelErrorTask->Cancel();
        mChannelErrorTask = NULL;
    }
}

static void
PrintErrorMessage(bool isChild, const char* channelName, const char* msg)
{
#ifdef DEBUG
    fprintf(stderr, "\n###!!! [%s][%s] Error: %s\n\n",
            isChild ? "Child" : "Parent", channelName, msg);
#endif
}

bool
AsyncChannel::MaybeHandleError(Result code, const char* channelName)
{
    if (MsgProcessed == code)
        return true;

    const char* errorMsg;
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
    const char* errorMsg;
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
AsyncChannel::OnMessageReceived(const Message& msg)
{
    AssertIOThread();
    NS_ASSERTION(mChannelState != ChannelError, "Shouldn't get here!");

    MonitorAutoLock lock(mMonitor);

    if (!MaybeInterceptSpecialIOMessage(msg))
        
        mWorkerLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &AsyncChannel::OnDispatchMessage, msg));
}

void
AsyncChannel::OnChannelOpened()
{
    AssertIOThread();
    {
        MonitorAutoLock lock(mMonitor);
        mChannelState = ChannelOpening;
    }
    mTransport->Connect();
}

void
AsyncChannel::DispatchOnChannelConnected(int32 peer_pid)
{
    AssertWorkerThread();
    if (mListener)
        mListener->OnChannelConnected(peer_pid);
}

void
AsyncChannel::OnChannelConnected(int32 peer_pid)
{
    AssertIOThread();

    {
        MonitorAutoLock lock(mMonitor);
        mChannelState = ChannelConnected;
        mMonitor.Notify();
    }

    if(mExistingListener)
        mExistingListener->OnChannelConnected(peer_pid);

    mWorkerLoop->PostTask(FROM_HERE, NewRunnableMethod(this, 
                                                       &AsyncChannel::DispatchOnChannelConnected, 
                                                       peer_pid));
}

void
AsyncChannel::OnChannelError()
{
    AssertIOThread();

    MonitorAutoLock lock(mMonitor);

    if (ChannelClosing != mChannelState)
        mChannelState = ChannelError;

    PostErrorNotifyTask();
}

void
AsyncChannel::PostErrorNotifyTask()
{
    AssertIOThread();
    mMonitor.AssertCurrentThreadOwns();

    NS_ASSERTION(!mChannelErrorTask, "OnChannelError called twice?");

    
    mChannelErrorTask =
        NewRunnableMethod(this, &AsyncChannel::OnNotifyMaybeChannelError);
    mWorkerLoop->PostTask(FROM_HERE, mChannelErrorTask);
}

void
AsyncChannel::OnCloseChannel()
{
    AssertIOThread();

    mTransport->Close();

    MonitorAutoLock lock(mMonitor);
    mChannelState = ChannelClosed;
    mMonitor.Notify();
}

bool
AsyncChannel::MaybeInterceptSpecialIOMessage(const Message& msg)
{
    AssertIOThread();
    mMonitor.AssertCurrentThreadOwns();

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
    AssertIOThread();
    mMonitor.AssertCurrentThreadOwns();

    
    
    mChannelState = ChannelClosing;

    printf("NOTE: %s process received `Goodbye', closing down\n",
           mChild ? "child" : "parent");
}


} 
} 
