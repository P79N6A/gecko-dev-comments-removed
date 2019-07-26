






#include "mozilla/DebugOnly.h"

#include "mozilla/ipc/SyncChannel.h"
#include "mozilla/ipc/RPCChannel.h"

#include "nsDebug.h"
#include "nsTraceRefcnt.h"

using mozilla::MonitorAutoLock;

template<>
struct RunnableMethodTraits<mozilla::ipc::SyncChannel>
{
    static void RetainCallee(mozilla::ipc::SyncChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::SyncChannel* obj) { }
};

namespace mozilla {
namespace ipc {

const int32_t SyncChannel::kNoTimeout = INT32_MIN;

SyncChannel::SyncChannel(SyncListener* aListener)
  : AsyncChannel(aListener)
#ifdef OS_WIN
  , mTopFrame(NULL)
#endif
  , mPendingReply(0)
  , mProcessingSyncMessage(false)
  , mNextSeqno(0)
  , mInTimeoutSecondHalf(false)
  , mTimeoutMs(kNoTimeout)
{
    MOZ_COUNT_CTOR(SyncChannel);
#ifdef OS_WIN
    mEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    NS_ASSERTION(mEvent, "CreateEvent failed! Nothing is going to work!");
#endif
}

SyncChannel::~SyncChannel()
{
    MOZ_COUNT_DTOR(SyncChannel);
#ifdef OS_WIN
    CloseHandle(mEvent);
#endif
}


bool SyncChannel::sIsPumpingMessages = false;

bool
SyncChannel::EventOccurred()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();
    NS_ABORT_IF_FALSE(AwaitingSyncReply(), "not in wait loop");

    return !Connected() ||
           mRecvd.type() != 0 ||
           mRecvd.is_reply_error() ||
           !mUrgent.empty();
}

bool
SyncChannel::ProcessUrgentMessages()
{
    while (!mUrgent.empty()) {
        Message msg = mUrgent.front();
        mUrgent.pop_front();

        MOZ_ASSERT(msg.priority() == IPC::Message::PRIORITY_HIGH);

        {
            MOZ_ASSERT(msg.is_sync() || msg.is_rpc());

            MonitorAutoUnlock unlock(*mMonitor);
            SyncChannel::OnDispatchMessage(msg);
        }

        
        if (!Connected()) {
            ReportConnectionError("SyncChannel");
            return false;
        }

        
        
        MOZ_ASSERT(mRecvd.type() == 0);
    }

    return true;
}

bool
SyncChannel::Send(Message* _msg, Message* reply)
{
    if (mPendingReply) {
        
        
        
        NS_ERROR("Nested sync messages are not supported");
        return false;
    }

    nsAutoPtr<Message> msg(_msg);

    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();
    NS_ABORT_IF_FALSE(!ProcessingSyncMessage(),
                      "violation of sync handler invariant");
    NS_ABORT_IF_FALSE(msg->is_sync(), "can only Send() sync messages here");

#ifdef OS_WIN
    SyncStackFrame frame(this, false);
#endif

    msg->set_seqno(NextSeqno());

    MonitorAutoLock lock(*mMonitor);

    if (!Connected()) {
        ReportConnectionError("SyncChannel");
        return false;
    }

    mPendingReply = msg->type() + 1;
    DebugOnly<int32_t> msgSeqno = msg->seqno();
    mLink->SendMessage(msg.forget());

    while (1) {
        
        
        
        
        
        if (!Connected()) {
            ReportConnectionError("SyncChannel");
            return false;
        }

        while (!EventOccurred()) {
            bool maybeTimedOut = !SyncChannel::WaitForNotify();

            if (EventOccurred())
                break;

            
            
            if (!mUrgent.empty())
                break;

            if (maybeTimedOut && !ShouldContinueFromTimeout())
                return false;
        }

        if (!Connected()) {
            ReportConnectionError("SyncChannel");

            return false;
        }

        
        
        if (!ProcessUrgentMessages())
            return false;

        if (mRecvd.is_reply_error() || mRecvd.type() != 0) {
            
            
            
            
            
            
            NS_ABORT_IF_FALSE(mRecvd.is_sync() && mRecvd.is_reply() &&
                              (mRecvd.is_reply_error() ||
                               (mPendingReply == mRecvd.type() &&
                                msgSeqno == mRecvd.seqno())),
                              "unexpected sync message");

            mPendingReply = 0;
            if (mRecvd.is_reply_error())
                return false;

            *reply = TakeReply();

            MOZ_ASSERT(mUrgent.empty());
            return true;
        }
    }

    return true;
}

void
SyncChannel::OnDispatchMessage(const Message& msg)
{
    AssertWorkerThread();
    NS_ABORT_IF_FALSE(msg.is_sync() || msg.is_rpc(), "only sync messages here");
    NS_ABORT_IF_FALSE(!msg.is_reply(), "wasn't awaiting reply");

    Message* reply = 0;

    mProcessingSyncMessage = true;
    Result rv;
    if (msg.is_sync())
        rv = static_cast<SyncListener*>(mListener.get())->OnMessageReceived(msg, reply);
    else
        rv = static_cast<RPCChannel::RPCListener*>(mListener.get())->OnCallReceived(msg, reply);
    mProcessingSyncMessage = false;

    if (!MaybeHandleError(rv, "SyncChannel")) {
        
        delete reply;
        reply = new Message();
        if (msg.is_sync())
            reply->set_sync();
        else if (msg.is_rpc())
            reply->set_rpc();
        reply->set_reply();
        reply->set_reply_error();
    }

    reply->set_seqno(msg.seqno());

    {
        MonitorAutoLock lock(*mMonitor);
        if (ChannelConnected == mChannelState)
            mLink->SendMessage(reply);
    }
}






void
SyncChannel::OnMessageReceivedFromLink(const Message& msg)
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (MaybeInterceptSpecialIOMessage(msg))
        return;

    if (msg.priority() == IPC::Message::PRIORITY_HIGH) {
        
        
        if (!AwaitingSyncReply()) {
            mWorkerLoop->PostTask(
                FROM_HERE,
                NewRunnableMethod(this, &SyncChannel::OnDispatchMessage, msg));
        } else {
            mUrgent.push_back(msg);
            NotifyWorkerThread();
        }
        return;
    }

    if (!msg.is_sync()) {
        AsyncChannel::OnMessageReceivedFromLink(msg);
        return;
    }

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
SyncChannel::OnChannelErrorFromLink()
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (AwaitingSyncReply())
        NotifyWorkerThread();

    AsyncChannel::OnChannelErrorFromLink();
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
    mMonitor->AssertCurrentThreadOwns();

    bool cont;
    {
        MonitorAutoUnlock unlock(*mMonitor);
        cont = static_cast<SyncListener*>(mListener.get())->OnReplyTimeout();
    }

    if (!cont) {
        
        
        
        
        
        
        
        
        
        
        
        
        SynchronouslyClose();
        mChannelState = ChannelTimeout;
    }
        
    return cont;
}

bool
SyncChannel::WaitResponse(bool aWaitTimedOut)
{
  if (aWaitTimedOut) {
    if (mInTimeoutSecondHalf) {
      
      return false;
    }
    
    mInTimeoutSecondHalf = true;
  } else {
    mInTimeoutSecondHalf = false;
  }
  return true;
}





#ifndef OS_WIN

bool
SyncChannel::WaitForNotify()
{
    PRIntervalTime timeout = (kNoTimeout == mTimeoutMs) ?
                             PR_INTERVAL_NO_TIMEOUT :
                             PR_MillisecondsToInterval(mTimeoutMs);
    
    PRIntervalTime waitStart = PR_IntervalNow();

    mMonitor->Wait(timeout);

    
    
    return WaitResponse(IsTimeoutExpired(waitStart, timeout));
}

void
SyncChannel::NotifyWorkerThread()
{
    mMonitor->Notify();
}

#endif  


} 
} 
