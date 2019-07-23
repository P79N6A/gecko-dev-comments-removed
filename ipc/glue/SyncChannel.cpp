






































#include "mozilla/ipc/SyncChannel.h"
#include "mozilla/ipc/GeckoThread.h"

#include "nsDebug.h"

using mozilla::MutexAutoLock;

template<>
struct RunnableMethodTraits<mozilla::ipc::SyncChannel>
{
    static void RetainCallee(mozilla::ipc::SyncChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::SyncChannel* obj) { }
};

namespace mozilla {
namespace ipc {


typedef mozilla::ipc::SyncChannel::Listener Listener;

bool
SyncChannel::Send(Message* msg, Message* reply)
{
    NS_ASSERTION(ChannelIdle == mChannelState
                 || ChannelWaiting == mChannelState,
                 "trying to Send() to a channel not yet open");

    NS_PRECONDITION(msg->is_sync(), "can only Send() sync messages here");

    MutexAutoLock lock(mMutex);

    mChannelState = ChannelWaiting;
    mPendingReply = msg->type() + 1;
    AsyncChannel::Send(msg);

    while (1) {
        
        
        
        
        
        
        
        
        mCvar.Wait();

        if (mRecvd.is_reply() && mPendingReply == mRecvd.type()) {
            
            mPendingReply = 0;
            *reply = mRecvd;

            if (!WaitingForReply()) {
                mChannelState = ChannelIdle;
            }

            return true;
        }
        else {
            
            NS_ASSERTION(!mRecvd.is_reply(), "can't process replies here");
            
            mWorkerLoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &SyncChannel::OnDispatchMessage, mRecvd));
        }
    }
}

void
SyncChannel::OnDispatchMessage(const Message& msg)
{
    NS_ASSERTION(!msg.is_reply(), "can't process replies here");
    NS_ASSERTION(!msg.is_rpc(), "sync or async only here");

    if (!msg.is_sync()) {
        return AsyncChannel::OnDispatchMessage(msg);
    }

    Message* reply;
    switch (static_cast<Listener*>(mListener)->OnMessageReceived(msg, reply)) {
    case Listener::MsgProcessed:
        mIOLoop->PostTask(FROM_HERE,
                          NewRunnableMethod(this,
                                            &SyncChannel::OnSendReply,
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
SyncChannel::OnMessageReceived(const Message& msg)
{
    MutexAutoLock lock(mMutex);

    if (ChannelIdle == mChannelState) {
        
        if (msg.is_sync()) {
            mWorkerLoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &SyncChannel::OnDispatchMessage, msg));
        }
        else {
            return AsyncChannel::OnMessageReceived(msg);
        }
    }
    else if (ChannelWaiting == mChannelState) {
        
        mRecvd = msg;
        mCvar.Notify();
    }
    else {
        
        NOTREACHED();
    }
}

void
SyncChannel::OnSendReply(Message* aReply)
{
    mTransport->Send(aReply);
}


} 
} 
