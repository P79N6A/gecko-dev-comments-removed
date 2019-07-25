






































#include "mozilla/ipc/SyncChannel.h"

#include "nsDebug.h"
#include "nsTraceRefcnt.h"

using mozilla::MutexAutoLock;

template<>
struct RunnableMethodTraits<mozilla::ipc::SyncChannel>
{
    static void RetainCallee(mozilla::ipc::SyncChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::SyncChannel* obj) { }
};

namespace mozilla {
namespace ipc {

const int32 SyncChannel::kNoTimeout = PR_INT32_MIN;

SyncChannel::SyncChannel(SyncListener* aListener)
  : AsyncChannel(aListener),
    mPendingReply(0),
    mProcessingSyncMessage(false),
    mNextSeqno(0),
    mTimeoutMs(kNoTimeout)
{
  MOZ_COUNT_CTOR(SyncChannel);
}

SyncChannel::~SyncChannel()
{
    MOZ_COUNT_DTOR(SyncChannel);
}


bool SyncChannel::sIsPumpingMessages = false;

bool
SyncChannel::EventOccurred()
{
    AssertWorkerThread();
    mMutex.AssertCurrentThreadOwns();
    NS_ABORT_IF_FALSE(AwaitingSyncReply(), "not in wait loop");

    return (!Connected() || 0 != mRecvd.type());
}

bool
SyncChannel::Send(Message* msg, Message* reply)
{
    AssertWorkerThread();
    mMutex.AssertNotCurrentThreadOwns();
    NS_ABORT_IF_FALSE(!ProcessingSyncMessage(),
                      "violation of sync handler invariant");
    NS_ABORT_IF_FALSE(msg->is_sync(), "can only Send() sync messages here");

    msg->set_seqno(NextSeqno());

    MutexAutoLock lock(mMutex);

    if (!Connected()) {
        ReportConnectionError("SyncChannel");
        return false;
    }

    mPendingReply = msg->type() + 1;
    int32 msgSeqno = msg->seqno();
    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &SyncChannel::OnSend, msg));

    while (1) {
        bool maybeTimedOut = !SyncChannel::WaitForNotify();

        if (EventOccurred())
            break;

        
        NS_ABORT_IF_FALSE(maybeTimedOut,
                          "neither received a reply nor detected a hang!");
        if (!ShouldContinueFromTimeout())
            return false;
    }

    if (!Connected()) {
        ReportConnectionError("SyncChannel");
        return false;
    }

    
    
    
    
    

    
    NS_ABORT_IF_FALSE(mRecvd.is_sync() && mRecvd.is_reply() &&
                      (mRecvd.is_reply_error() ||
                       (mPendingReply == mRecvd.type() &&
                        msgSeqno == mRecvd.seqno())),
                      "unexpected sync message");

    mPendingReply = 0;
    *reply = mRecvd;
    mRecvd = Message();

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

    if (!MaybeHandleError(rv, "SyncChannel")) {
        
        delete reply;
        reply = new Message();
        reply->set_sync();
        reply->set_reply();
        reply->set_reply_error();
    }

    reply->set_seqno(msg.seqno());

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &SyncChannel::OnSend, reply));
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
        NotifyWorkerThread();
    }
}

void
SyncChannel::OnChannelError()
{
    AssertIOThread();

    AsyncChannel::OnChannelError();

    MutexAutoLock lock(mMutex);
    if (AwaitingSyncReply())
        NotifyWorkerThread();
}





namespace {

bool
IsTimeoutExpired(PRIntervalTime aStart, PRIntervalTime aTimeout)
{
    return (aTimeout != PR_INTERVAL_NO_TIMEOUT) &&
        (aTimeout <= (PR_IntervalNow() - aStart));
}

} 

bool
SyncChannel::ShouldContinueFromTimeout()
{
    AssertWorkerThread();
    mMutex.AssertCurrentThreadOwns();

    bool cont = true;

    if (!cont) {
        
        
        
        
        
        
        
        
        
        
        
        
        SynchronouslyClose();
        mChannelState = ChannelTimeout;
    }
        
    return cont;
}




#ifndef OS_WIN

bool
SyncChannel::WaitForNotify()
{
    PRIntervalTime timeout = (kNoTimeout == mTimeoutMs) ?
                             PR_INTERVAL_NO_TIMEOUT :
                             PR_MillisecondsToInterval(mTimeoutMs);
    
    PRIntervalTime waitStart = PR_IntervalNow();

    mCvar.Wait(timeout);

    
    
    return !IsTimeoutExpired(waitStart, timeout);
}

void
SyncChannel::NotifyWorkerThread()
{
    mCvar.Notify();
}

#endif  


} 
} 
