






































#include "mozilla/ipc/AsyncChannel.h"
#include "mozilla/ipc/GeckoThread.h"

#include "nsDebug.h"
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
    

    mChannelState = ChannelClosed;
}

bool
AsyncChannel::Send(Message* msg)
{
    AssertWorkerThread();
    NS_ABORT_IF_FALSE(MSG_ROUTING_NONE != msg->routing_id(), "need a route");

    if (!Connected())
        
        return false;

    mIOLoop->PostTask(FROM_HERE,
                      NewRunnableMethod(this, &AsyncChannel::OnSend, msg));
    return true;
}

void
AsyncChannel::OnDispatchMessage(const Message& msg)
{
    AssertWorkerThread();
    NS_ASSERTION(!msg.is_reply(), "can't process replies here");
    NS_ASSERTION(!(msg.is_sync() || msg.is_rpc()), "async dispatch only");

    switch (mListener->OnMessageReceived(msg)) {
    case MsgProcessed:
        return;

    case MsgNotKnown:
    case MsgNotAllowed:
    case MsgPayloadError:
    case MsgRouteError:
    case MsgValueError:
        
        return;

    default:
        NOTREACHED();
        return;
    }
}






void
AsyncChannel::OnMessageReceived(const Message& msg)
{
    AssertIOThread();
    NS_ASSERTION(mChannelState != ChannelError, "Shouldn't get here!");

    
    mWorkerLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this,
                                            &AsyncChannel::OnDispatchMessage,
                                            msg));
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
    mChannelState = ChannelError;

    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        
    }
    else {
        
#ifdef DEBUG
        
        XRE_ShutdownChildProcess(mWorkerLoop);

        
        MessageLoop::current()->Quit();
#else
        
        NS_DebugBreak(NS_DEBUG_ABORT, nsnull, nsnull, nsnull, 0);
#endif
    }
}

void
AsyncChannel::OnChannelOpened()
{
    AssertIOThread();
    mChannelState = ChannelOpening;
    mTransport->Connect();
}

void
AsyncChannel::OnSend(Message* aMsg)
{
    AssertIOThread();
    mTransport->Send(aMsg);
    
}


} 
} 
