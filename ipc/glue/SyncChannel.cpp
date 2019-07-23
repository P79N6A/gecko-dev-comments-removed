






































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

bool
SyncChannel::Send(Message* msg, Message* reply)
{
    AssertWorkerThread();
    NS_ABORT_IF_FALSE(!ProcessingSyncMessage(),
                      "violation of sync handler invariant");
    NS_ABORT_IF_FALSE(msg->is_sync(), "can only Send() sync messages here");

    MutexAutoLock lock(mMutex);

    if (!Connected())
        
        return false;

    mPendingReply = msg->type() + 1;
    if (!AsyncChannel::Send(msg))
        
        return false;

    
    mCvar.Wait();

    if (!Connected())
        
        return false;

    
    
    
    
    

    
    NS_ABORT_IF_FALSE(mRecvd.is_sync() && mRecvd.is_reply() &&
                      (mPendingReply == mRecvd.type() || mRecvd.is_reply_error()),
                      "unexpected sync message");

    mPendingReply = 0;
    *reply = mRecvd;

    return true;
}

void
SyncChannel::OnDispatchMessage(const Message& msg)
{
    AssertWorkerThread();
    NS_ABORT_IF_FALSE(msg.is_sync(), "only sync messages here");
    NS_ABORT_IF_FALSE(!msg.is_reply(), "wasn't awaiting reply");

    Message* reply = 0;

    mProcessingSyncMessage = true;
    Result rv =
        static_cast<SyncListener*>(mListener)->OnMessageReceived(msg, reply);
    mProcessingSyncMessage = false;

    switch (rv) {
    case MsgProcessed:
        break;

    case MsgNotKnown:
    case MsgNotAllowed:
    case MsgPayloadError:
    case MsgRouteError:
    case MsgValueError:
        
        delete reply;
        reply = new Message();
        reply->set_sync();
        reply->set_reply();
        reply->set_reply_error();
        break;

    default:
        NOTREACHED();
        return;
    }

    mIOLoop->PostTask(FROM_HERE,
                      NewRunnableMethod(this,
                                        &SyncChannel::OnSendReply,
                                        reply));
}






void
SyncChannel::OnMessageReceived(const Message& msg)
{
    AssertIOThread();
    if (!msg.is_sync()) {
        return AsyncChannel::OnMessageReceived(msg);
    }

    MutexAutoLock lock(mMutex);

    if (!AwaitingSyncReply()) {
        
        mWorkerLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &SyncChannel::OnDispatchMessage, msg));
    }
    else {
        
        mRecvd = msg;
        mCvar.Notify();
    }
}

void
SyncChannel::OnChannelError()
{
    AssertIOThread();
    {
        MutexAutoLock lock(mMutex);

        mChannelState = ChannelError;

        if (AwaitingSyncReply()) {
            mCvar.Notify();
        }
    }

    return AsyncChannel::OnChannelError();
}

void
SyncChannel::OnSendReply(Message* aReply)
{
    AssertIOThread();
    mTransport->Send(aReply);
}


} 
} 
