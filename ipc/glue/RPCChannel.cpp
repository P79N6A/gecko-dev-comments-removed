






































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


typedef mozilla::ipc::RPCChannel::Listener Listener;

bool
RPCChannel::Call(Message* msg, Message* reply)
{
    NS_ASSERTION(ChannelIdle == mChannelState
                 || ChannelWaiting == mChannelState,
                 "trying to Send() to a channel not yet open");

    NS_PRECONDITION(msg->is_rpc(), "can only Call() RPC messages here");

    mMutex.Lock();

    mChannelState = ChannelWaiting;

    mPending.push(*msg);
    AsyncChannel::Send(msg);

    while (1) {
        
        
        
        
        
        mCvar.Wait();

        Message recvd = mPending.top();
        mPending.pop();

        if (!recvd.is_rpc()) {
            SyncChannel::OnDispatchMessage(recvd);
            
        }
        
        else if (recvd.is_reply()) {
            NS_ASSERTION(0 < mPending.size(), "invalid RPC stack");

            const Message& pending = mPending.top();
            if (recvd.type() != (pending.type()+1)) {
                
                NS_ASSERTION(0, "somebody's misbehavin'");
            }

            
            
            mPending.pop();
            *reply = recvd;

            if (!WaitingForReply()) {
                mChannelState = ChannelIdle;
            }

            mMutex.Unlock();
            return true;
        }
        
        else {
            mMutex.Unlock();

            
            OnDispatchMessage(recvd);
            

            mMutex.Lock();
        }
    }

    delete msg;

    return true;
}

void
RPCChannel::OnDispatchMessage(const Message& call)
{
    if (!call.is_rpc()) {
        return SyncChannel::OnDispatchMessage(call);
    }

    Message* reply;
    switch (static_cast<Listener*>(mListener)->OnCallReceived(call, reply)) {
    case Listener::MsgProcessed:
        mIOLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this,
                                            &RPCChannel::OnSendReply,
                                            reply));
        return;

    case Listener::MsgNotKnown:
    case Listener::MsgNotAllowed:
    case Listener::MsgPayloadError:
    case Listener::MsgRouteError:
    case Listener::MsgValueError:
        
        return;

    default:
        NOTREACHED();
        return;
    }
}






void
RPCChannel::OnMessageReceived(const Message& msg)
{
    MutexAutoLock lock(mMutex);

    if (0 == mPending.size()) {
        
        mWorkerLoop->PostTask(FROM_HERE,
                              NewRunnableMethod(this,
                                                &RPCChannel::OnDispatchMessage,
                                                msg));
    }
    else {
        
        mPending.push(msg);
        mCvar.Notify();
    }
}


} 
} 
