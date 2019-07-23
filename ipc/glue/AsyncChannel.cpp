






































#include "mozilla/ipc/AsyncChannel.h"
#include "mozilla/ipc/GeckoThread.h"

#include "nsDebug.h"

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
        
        
        mChannelState = ChannelIdle;
    }

    mIOLoop = aIOLoop;
    mWorkerLoop = MessageLoop::current();

    NS_ASSERTION(mIOLoop, "need an IO loop");
    NS_ASSERTION(mWorkerLoop, "need a worker loop");

    if (needOpen) {
        mIOLoop->PostTask(FROM_HERE, 
                          NewRunnableMethod(this,
                                            &AsyncChannel::OnChannelOpened));
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
    NS_ASSERTION(ChannelIdle == mChannelState
                 || ChannelWaiting == mChannelState,
                 "trying to Send() to a channel not yet open");

    NS_PRECONDITION(MSG_ROUTING_NONE != msg->routing_id(), "need a route");
    mIOLoop->PostTask(FROM_HERE,
                      NewRunnableMethod(this, &AsyncChannel::OnSend, msg));
    return true;
}

void
AsyncChannel::OnDispatchMessage(const Message& msg)
{
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
    
    mWorkerLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this,
                                            &AsyncChannel::OnDispatchMessage,
                                            msg));
}

void
AsyncChannel::OnChannelConnected(int32 peer_pid)
{
    mChannelState = ChannelIdle;
}

void
AsyncChannel::OnChannelError()
{
    
    mChannelState = ChannelError;
}

void
AsyncChannel::OnChannelOpened()
{
    mChannelState = ChannelOpening;
    mTransport->Connect();
}

void
AsyncChannel::OnSend(Message* aMsg)
{
    mTransport->Send(aMsg);
    
}


} 
} 
