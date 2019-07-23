






































#include "mozilla/ipc/AsyncChannel.h"
#include "mozilla/ipc/GeckoThread.h"
#include "mozilla/ipc/ProtocolUtils.h"

#include "nsDebug.h"
#include "nsTraceRefcnt.h"
#include "nsXULAppAPI.h"

using mozilla::MutexAutoLock;

template<>
struct RunnableMethodTraits<mozilla::ipc::AsyncChannel>
{
    static void RetainCallee(mozilla::ipc::AsyncChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::AsyncChannel* obj) { }
};

namespace mozilla {
namespace ipc {

AsyncChannel::AsyncChannel(AsyncListener* aListener)
  : mTransport(0),
    mListener(aListener),
    mChannelState(ChannelClosed),
    mMutex("mozilla.ipc.AsyncChannel.mMutex"),
    mCvar(mMutex, "mozilla.ipc.AsyncChannel.mCvar"),
    mIOLoop(),
    mWorkerLoop()
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
    mTransport->set_listener(this);

    
    
    bool needOpen = true;
    if(!aIOLoop) {
        
        needOpen = false;
        aIOLoop = BrowserProcessSubThread
                  ::GetMessageLoop(BrowserProcessSubThread::IO);
        
        
        mChannelState = ChannelConnected;
    }

    mChild = needOpen;

    mIOLoop = aIOLoop;
    mWorkerLoop = MessageLoop::current();

    NS_ASSERTION(mIOLoop, "need an IO loop");
    NS_ASSERTION(mWorkerLoop, "need a worker loop");

    if (needOpen) {             
        MutexAutoLock lock(mMutex);

        mIOLoop->PostTask(FROM_HERE, 
                          NewRunnableMethod(this,
                                            &AsyncChannel::OnChannelOpened));

        
        while (mChannelState != ChannelConnected) {
            mCvar.Wait();
        }
    }

    return true;
}

void
AsyncChannel::Close()
{
    {
        MutexAutoLock lock(mMutex);

        if (ChannelConnected != mChannelState)
            
            
            NS_RUNTIMEABORT("Close() called on closed channel!");

        AssertWorkerThread();

        
        SendGoodbye();

        mChannelState = ChannelClosing;

        
        mIOLoop->PostTask(
            FROM_HERE, NewRunnableMethod(this, &AsyncChannel::OnCloseChannel));

        while (ChannelClosing == mChannelState)
            mCvar.Wait();

        
        
        mChannelState = ChannelClosed;
    }

    return NotifyChannelClosed();
}

bool
AsyncChannel::Send(Message* msg)
{
    AssertWorkerThread();
    mMutex.AssertNotCurrentThreadOwns();
    NS_ABORT_IF_FALSE(MSG_ROUTING_NONE != msg->routing_id(), "need a route");

    {
        MutexAutoLock lock(mMutex);

        if (!Connected()) {
            ReportConnectionError("AsyncChannel");
            return false;
        }

        mIOLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this, &AsyncChannel::OnSend, msg));
    }

    return true;
}

void
AsyncChannel::OnDispatchMessage(const Message& msg)
{
    AssertWorkerThread();
    NS_ASSERTION(!msg.is_reply(), "can't process replies here");
    NS_ASSERTION(!(msg.is_sync() || msg.is_rpc()), "async dispatch only");

    if (MaybeInterceptGoodbye(msg))
        
        
        return;

    
    

    (void)MaybeHandleError(mListener->OnMessageReceived(msg), "AsyncChannel");
}


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

void
AsyncChannel::SendGoodbye()
{
    AssertWorkerThread();

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &AsyncChannel::OnSend, new GoodbyeMessage()));
}

bool
AsyncChannel::MaybeInterceptGoodbye(const Message& msg)
{
    
    
    if (MSG_ROUTING_NONE != msg.routing_id())
        return false;

    if (msg.is_sync() || msg.is_rpc() || GOODBYE_MESSAGE_TYPE != msg.type())
        NS_RUNTIMEABORT("received unknown MSG_ROUTING_NONE message when expecting `Goodbye'");

    MutexAutoLock lock(mMutex);
    
    
    mChannelState = ChannelClosing;

    printf("NOTE: %s process received `Goodbye', closing down\n",
           mChild ? "child" : "parent");

    return true;
}

void
AsyncChannel::NotifyChannelClosed()
{
    if (ChannelClosed != mChannelState)
        NS_RUNTIMEABORT("channel should have been closed!");

    
    
    mListener->OnChannelClose();
    Clear();
}

void
AsyncChannel::NotifyMaybeChannelError()
{
    
    
    if (ChannelClosing == mChannelState) {
        
        
        mChannelState = ChannelClosed;
        return NotifyChannelClosed();
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

    PrintErrorMessage(channelName, errorMsg);
    return false;
}

void
AsyncChannel::ReportConnectionError(const char* channelName)
{
    const char* errorMsg;
    switch (mChannelState) {
    case ChannelClosed:
        errorMsg = "Closed channel: cannot send/recv";
        break;
    case ChannelOpening:
        errorMsg = "Opening channel: not yet ready for send/recv";
        break;
    case ChannelError:
        errorMsg = "Channel error: cannot send/recv";
        break;

    default:
        NOTREACHED();
    }

    PrintErrorMessage(channelName, errorMsg);
}





void
AsyncChannel::OnMessageReceived(const Message& msg)
{
    AssertIOThread();
    NS_ASSERTION(mChannelState != ChannelError, "Shouldn't get here!");

    
    mWorkerLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &AsyncChannel::OnDispatchMessage, msg));
}

void
AsyncChannel::OnChannelOpened()
{
    AssertIOThread();
    mChannelState = ChannelOpening;
    mTransport->Connect();
}

void
AsyncChannel::OnChannelConnected(int32 peer_pid)
{
    AssertIOThread();

    MutexAutoLock lock(mMutex);
    mChannelState = ChannelConnected;
    mCvar.Notify();
}

void
AsyncChannel::OnChannelError()
{
    AssertIOThread();

    MutexAutoLock lock(mMutex);

    
    
    if (ChannelClosing != mChannelState)
        mChannelState = ChannelError;

    mWorkerLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &AsyncChannel::NotifyMaybeChannelError));
}

void
AsyncChannel::OnSend(Message* aMsg)
{
    AssertIOThread();
    mTransport->Send(aMsg);
    
}

void
AsyncChannel::OnCloseChannel()
{
    AssertIOThread();

    mTransport->Close();

    MutexAutoLock lock(mMutex);
    mChannelState = ChannelClosed;
    mCvar.Notify();
}


} 
} 
