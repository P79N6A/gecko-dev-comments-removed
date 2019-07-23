





































#include "mozilla/ipc/RPCChannel.h"
#include "mozilla/ipc/GeckoThread.h"

#include "nsDebug.h"

using mozilla::MutexAutoLock;

template<>
struct RunnableMethodTraits<mozilla::ipc::RPCChannel>
{
    static void RetainCallee(mozilla::ipc::RPCChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::RPCChannel* obj) { }
};

namespace mozilla {
namespace ipc {


bool
RPCChannel::Open(Transport* aTransport, MessageLoop* aIOLoop)
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
    }

    mIOLoop = aIOLoop;
    mWorkerLoop = MessageLoop::current();

    NS_ASSERTION(mIOLoop, "need an IO loop");
    NS_ASSERTION(mWorkerLoop, "need a worker loop");

    if (needOpen) {
        mIOLoop->PostTask(FROM_HERE, 
                          NewRunnableMethod(this,
                                            &RPCChannel::OnChannelOpened));
    }

    return true;
}

void
RPCChannel::Close()
{
    

    mChannelState = ChannelClosed;
}

bool
RPCChannel::Call(Message* msg, Message* reply)
{
    NS_PRECONDITION(MSG_ROUTING_NONE != msg->routing_id(), "need a route");

    mMutex.Lock();

    mPending.push(*msg);
    mIOLoop->PostTask(FROM_HERE, NewRunnableMethod(this,
                                                   &RPCChannel::SendCall,
                                                   msg));
    while (1) {
        
        
        
        mCvar.Wait();

        Message recvd = mPending.top();
        mPending.pop();

        if (recvd.is_reply()) {
            
            
            NS_ASSERTION(0 < mPending.size(), "invalid RPC stack");
            mPending.pop();
            *reply = recvd;

            mMutex.Unlock();
            return true;
        }
        else {
            mMutex.Unlock();

            
            if (!ProcessIncomingCall(recvd))
                return false;

            mMutex.Lock();
        }
    }

    delete msg;

    return true;
}

bool
RPCChannel::ProcessIncomingCall(Message call)
{
   Message* reply;

    switch (mListener->OnCallReceived(call, reply)) {
    case Listener::MsgProcessed:
        mIOLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this,
                                            &RPCChannel::SendReply,
                                            reply));
        return true;

    case Listener::MsgNotKnown:
    case Listener::MsgNotAllowed:
    case Listener::MsgPayloadError:
    case Listener::MsgRouteError:
    case Listener::MsgValueError:
        
        return false;

    default:
        NOTREACHED();
        return false;
    }
}

void
RPCChannel::OnIncomingCall(Message msg)
{
    NS_ASSERTION(0 == mPending.size(),
                 "woke up the worker thread when it had outstanding work!");
    ProcessIncomingCall(msg);
}






void
RPCChannel::OnMessageReceived(const Message& msg)
{MutexAutoLock lock(mMutex);
    if (0 == mPending.size()) {
        
        mWorkerLoop->PostTask(FROM_HERE,
                              NewRunnableMethod(this,
                                                &RPCChannel::OnIncomingCall,
                                                msg));
    }
    else {
        
        mPending.push(msg);
        mCvar.Notify();
    }
}

void
RPCChannel::OnChannelConnected(int32 peer_pid)
{
    mChannelState = ChannelConnected;
}

void
RPCChannel::OnChannelError()
{
    
    mChannelState = ChannelError;
}

void
RPCChannel::OnChannelOpened()
{
    mChannelState = ChannelOpening;
    mTransport->Connect();
}

void
RPCChannel::SendCall(Message* aCall)
{
    mTransport->Send(aCall);
}

void
RPCChannel::SendReply(Message* aReply)
{
    mTransport->Send(aReply);
}


} 
} 
