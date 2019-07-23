






































#include "mozilla/ipc/AsyncChannel.h"
#include "mozilla/ipc/GeckoThread.h"

#include "mozilla/dom/ContentProcessChild.h"
using mozilla::dom::ContentProcessChild;

#include "nsDebug.h"
#include "nsXULAppAPI.h"

template<>
struct RunnableMethodTraits<mozilla::ipc::AsyncChannel>
{
    static void RetainCallee(mozilla::ipc::AsyncChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::AsyncChannel* obj) { }
};

template<>
struct RunnableMethodTraits<ContentProcessChild>
{
    static void RetainCallee(ContentProcessChild* obj) { }
    static void ReleaseCallee(ContentProcessChild* obj) { }
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
    NS_ASSERTION(ChannelConnected == mChannelState,
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
    NS_ASSERTION(mChannelState != ChannelError, "Shouldn't get here!");

    
    mWorkerLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this,
                                            &AsyncChannel::OnDispatchMessage,
                                            msg));
}

void
AsyncChannel::OnChannelConnected(int32 peer_pid)
{
    mChannelState = ChannelConnected;
}

void
AsyncChannel::OnChannelError()
{
    mChannelState = ChannelError;

    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        
    }
    else {
        
#ifdef DEBUG
        
        mWorkerLoop->PostTask(FROM_HERE,
            NewRunnableMethod(ContentProcessChild::GetSingleton(),
                              &ContentProcessChild::Quit));

        
        MessageLoop::current()->Quit();
#else
        
        NS_DebugBreak(NS_DEBUG_ABORT, nsnull, nsnull, nsnull, 0);
#endif
    }
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
