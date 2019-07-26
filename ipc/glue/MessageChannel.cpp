






#include "mozilla/ipc/MessageChannel.h"
#include "mozilla/ipc/ProtocolUtils.h"

#include "nsDebug.h"
#include "nsTraceRefcnt.h"

using namespace mozilla;
using namespace std;

using mozilla::MonitorAutoLock;
using mozilla::MonitorAutoUnlock;

template<>
struct RunnableMethodTraits<mozilla::ipc::MessageChannel>
{
    static void RetainCallee(mozilla::ipc::MessageChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::MessageChannel* obj) { }
};

#define IPC_ASSERT(_cond, ...)                                      \
    do {                                                            \
        if (!(_cond))                                               \
            DebugAbort(__FILE__, __LINE__, #_cond,## __VA_ARGS__);  \
    } while (0)

namespace mozilla {
namespace ipc {

const int32_t MessageChannel::kNoTimeout = INT32_MIN;


bool MessageChannel::sIsPumpingMessages = false;

MessageChannel::MessageChannel(MessageListener *aListener)
  : mListener(aListener->asWeakPtr()),
    mChannelState(ChannelClosed),
    mSide(UnknownSide),
    mLink(nullptr),
    mWorkerLoop(nullptr),
    mChannelErrorTask(nullptr),
    mWorkerLoopID(-1),
    mTimeoutMs(kNoTimeout),
    mInTimeoutSecondHalf(false),
    mNextSeqno(0),
    mPendingSyncReplies(0),
    mPendingUrgentReplies(0),
    mDispatchingSyncMessage(false),
    mRemoteStackDepthGuess(false),
    mSawRPCOutMsg(false)
{
    MOZ_COUNT_CTOR(ipc::MessageChannel);

#ifdef OS_WIN
    mTopFrame = nullptr;
#endif

    mDequeueOneTask = new RefCountedTask(NewRunnableMethod(
                                                 this,
                                                 &MessageChannel::OnMaybeDequeueOne));

#ifdef OS_WIN
    mEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    NS_ASSERTION(mEvent, "CreateEvent failed! Nothing is going to work!");
#endif
}

MessageChannel::~MessageChannel()
{
    MOZ_COUNT_DTOR(ipc::MessageChannel);
    IPC_ASSERT(mCxxStackFrames.empty(), "mismatched CxxStackFrame ctor/dtors");
#ifdef OS_WIN
    CloseHandle(mEvent);
#endif
}

bool
MessageChannel::Connected() const
{
    mMonitor->AssertCurrentThreadOwns();

    
    
    return (ChannelOpening == mChannelState || ChannelConnected == mChannelState);
}

void
MessageChannel::Clear()
{
    
    
    
    
    
    
    
    
    

    mDequeueOneTask->Cancel();

    mWorkerLoop = nullptr;
    delete mLink;
    mLink = nullptr;

    if (mChannelErrorTask) {
        mChannelErrorTask->Cancel();
        mChannelErrorTask = nullptr;
    }
}

bool
MessageChannel::Open(Transport* aTransport, MessageLoop* aIOLoop, Side aSide)
{
    NS_PRECONDITION(!mLink, "Open() called > once");

    mMonitor = new RefCountedMonitor();
    mWorkerLoop = MessageLoop::current();
    mWorkerLoopID = mWorkerLoop->id();

    ProcessLink *link = new ProcessLink(this);
    link->Open(aTransport, aIOLoop, aSide); 
    mLink = link;
    return true;
}

bool
MessageChannel::Open(MessageChannel *aTargetChan, MessageLoop *aTargetLoop, Side aSide)
{
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    NS_PRECONDITION(aTargetChan, "Need a target channel");
    NS_PRECONDITION(ChannelClosed == mChannelState, "Not currently closed");

    CommonThreadOpenInit(aTargetChan, aSide);

    Side oppSide = UnknownSide;
    switch(aSide) {
      case ChildSide: oppSide = ParentSide; break;
      case ParentSide: oppSide = ChildSide; break;
      case UnknownSide: break;
    }

    mMonitor = new RefCountedMonitor();

    MonitorAutoLock lock(*mMonitor);
    mChannelState = ChannelOpening;
    aTargetLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(aTargetChan, &MessageChannel::OnOpenAsSlave, this, oppSide));

    while (ChannelOpening == mChannelState)
        mMonitor->Wait();
    NS_ASSERTION(ChannelConnected == mChannelState, "not connected when awoken");
    return (ChannelConnected == mChannelState);
}

void
MessageChannel::OnOpenAsSlave(MessageChannel *aTargetChan, Side aSide)
{
    
    NS_PRECONDITION(ChannelClosed == mChannelState,
                    "Not currently closed");
    NS_PRECONDITION(ChannelOpening == aTargetChan->mChannelState,
                    "Target channel not in the process of opening");

    CommonThreadOpenInit(aTargetChan, aSide);
    mMonitor = aTargetChan->mMonitor;

    MonitorAutoLock lock(*mMonitor);
    NS_ASSERTION(ChannelOpening == aTargetChan->mChannelState,
                 "Target channel not in the process of opening");
    mChannelState = ChannelConnected;
    aTargetChan->mChannelState = ChannelConnected;
    aTargetChan->mMonitor->Notify();
}

void
MessageChannel::CommonThreadOpenInit(MessageChannel *aTargetChan, Side aSide)
{
    mWorkerLoop = MessageLoop::current();
    mWorkerLoopID = mWorkerLoop->id();
    mLink = new ThreadLink(this, aTargetChan);
    mSide = aSide;
}

bool
MessageChannel::Echo(Message* aMsg)
{
    nsAutoPtr<Message> msg(aMsg);
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();
    IPC_ASSERT(MSG_ROUTING_NONE != msg->routing_id(), "need a route");

    MonitorAutoLock lock(*mMonitor);

    if (!Connected()) {
        ReportConnectionError("MessageChannel");
        return false;
    }

    mLink->EchoMessage(msg.forget());
    return true;
}

bool
MessageChannel::Send(Message* aMsg)
{
    Message copy = *aMsg;
    CxxStackFrame frame(*this, OUT_MESSAGE, &copy);

    nsAutoPtr<Message> msg(aMsg);
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();
    IPC_ASSERT(MSG_ROUTING_NONE != msg->routing_id(), "need a route");

    MonitorAutoLock lock(*mMonitor);
    if (!Connected()) {
        ReportConnectionError("MessageChannel");
        return false;
    }
    mLink->SendMessage(msg.forget());
    return true;
}

bool
MessageChannel::MaybeInterceptSpecialIOMessage(const Message& aMsg)
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (MSG_ROUTING_NONE == aMsg.routing_id() &&
        GOODBYE_MESSAGE_TYPE == aMsg.type())
    {
        
        
        mChannelState = ChannelClosing;
        printf("NOTE: %s process received `Goodbye', closing down\n",
               (mSide == ChildSide) ? "child" : "parent");
        return true;
    }
    return false;
}

void
MessageChannel::OnMessageReceivedFromLink(const Message& aMsg)
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (MaybeInterceptSpecialIOMessage(aMsg))
        return;

    
    
    if ((AwaitingSyncReply() && aMsg.is_sync()) ||
        (AwaitingUrgentReply() && aMsg.is_urgent()))
    {
        mRecvd = new Message(aMsg);
        NotifyWorkerThread();
        return;
    }

    
    MOZ_ASSERT(!aMsg.compress() || !aMsg.is_urgent());

    bool compress = (aMsg.compress() && !mPending.empty() &&
                     mPending.back().type() == aMsg.type() &&
                     mPending.back().routing_id() == aMsg.routing_id());
    if (compress) {
        
        
        
        MOZ_ASSERT(mPending.back().compress());
        mPending.pop_back();
    }

    if (aMsg.is_urgent()) {
        MOZ_ASSERT(!mPendingUrgentRequest);
        mPendingUrgentRequest = new Message(aMsg);
    } else {
        mPending.push_back(aMsg);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (AwaitingRPCReply() || (AwaitingSyncReply() && aMsg.is_urgent())) {
        
        
        NotifyWorkerThread();
    } else {
        
        
        
        if (!compress) {
            
            
            mWorkerLoop->PostTask(FROM_HERE, new DequeueTask(mDequeueOneTask));
        }
    }
}

bool
MessageChannel::Send(Message* aMsg, Message* aReply)
{
    
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

#ifdef OS_WIN
    SyncStackFrame frame(this, false);
#endif

    Message copy = *aMsg;
    CxxStackFrame f(*this, OUT_MESSAGE, &copy);

    MonitorAutoLock lock(*mMonitor);

    IPC_ASSERT(aMsg->is_sync(), "can only Send() sync messages here");
    IPC_ASSERT(!DispatchingSyncMessage(), "violation of sync handler invariant");

    if (AwaitingSyncReply()) {
        
        
        
        NS_ERROR("Nested sync messages are not supported");
        return false;
    }

    AutoEnterPendingReply replies(mPendingSyncReplies);
    if (!SendAndWait(aMsg, aReply))
        return false;

    NS_ABORT_IF_FALSE(aReply->is_sync(), "reply is not sync");
    return true;
}

bool
MessageChannel::UrgentCall(Message* aMsg, Message* aReply)
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();
    IPC_ASSERT(mSide == ParentSide, "cannot send urgent requests from child");

#ifdef OS_WIN
    SyncStackFrame frame(this, false);
#endif

    Message copy = *aMsg;
    CxxStackFrame f(*this, OUT_MESSAGE, &copy);

    MonitorAutoLock lock(*mMonitor);

    
    
    IPC_ASSERT(!AwaitingUrgentReply(), "urgent calls cannot nest");
    IPC_ASSERT(!AwaitingRPCReply(), "urgent calls cannot be issued within RPC calls");
    IPC_ASSERT(!AwaitingSyncReply(), "urgent calls cannot be issued within sync sends");

    AutoEnterPendingReply replies(mPendingUrgentReplies);
    if (!SendAndWait(aMsg, aReply))
        return false;

    NS_ABORT_IF_FALSE(aReply->is_urgent(), "reply is not urgent");
    return true;
}

bool
MessageChannel::SendAndWait(Message* aMsg, Message* aReply)
{
    mMonitor->AssertCurrentThreadOwns();

    nsAutoPtr<Message> msg(aMsg);

    if (!Connected()) {
        ReportConnectionError("MessageChannel::SendAndWait");
        return false;
    }

    msg->set_seqno(NextSeqno());

    DebugOnly<int32_t> replySeqno = msg->seqno();
    DebugOnly<msgid_t> replyType = msg->type() + 1;

    mLink->SendMessage(msg.forget());

    while (true) {
        
        while (true) {
            if (mRecvd || mPendingUrgentRequest)
                break;

            bool maybeTimedOut = !WaitForSyncNotify();

            if (!Connected()) {
                ReportConnectionError("MessageChannel::SendAndWait");
                return false;
            }

            if (maybeTimedOut && !ShouldContinueFromTimeout())
                return false;
        }

        if (mPendingUrgentRequest) {
            
            
            
            
            
            
            
            bool hadSyncReply = !!mRecvd;

            nsAutoPtr<Message> recvd(mPendingUrgentRequest.forget());
            {
                MonitorAutoUnlock unlock(*mMonitor);
                DispatchMessage(*recvd);
            }
            if (!Connected()) {
                ReportConnectionError("MessageChannel::DispatchMessage");
                return false;
            }

            IPC_ASSERT(!hadSyncReply || !mRecvd, "incoherent mRecvd state");
        }

        if (mRecvd) {
            NS_ABORT_IF_FALSE(mRecvd->is_reply(), "expected reply");

            if (mRecvd->is_reply_error()) {
                mRecvd = nullptr;
                return false;
            }

            NS_ABORT_IF_FALSE(mRecvd->type() == replyType, "wrong reply type");
            NS_ABORT_IF_FALSE(mRecvd->seqno() == replySeqno, "wrong sequence number");

            *aReply = *mRecvd;
            mRecvd = nullptr;
            return true;
        }
    }

    return true;
}

bool
MessageChannel::Call(Message* aMsg, Message* aReply)
{
    if (aMsg->is_urgent())
        return UrgentCall(aMsg, aReply);
    return RPCCall(aMsg, aReply);
}

bool
MessageChannel::RPCCall(Message* aMsg, Message* aReply)
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

#ifdef OS_WIN
    SyncStackFrame frame(this, true);
#endif

    
    
    Message copy = *aMsg;
    CxxStackFrame cxxframe(*this, OUT_MESSAGE, &copy);

    MonitorAutoLock lock(*mMonitor);
    if (!Connected()) {
        ReportConnectionError("MessageChannel::Call");
        return false;
    }

    
    IPC_ASSERT(!AwaitingSyncReply() && !AwaitingUrgentReply(),
               "cannot issue RPC call whiel blocked on sync or urgent");
    IPC_ASSERT(!DispatchingSyncMessage() || aMsg->priority() == IPC::Message::PRIORITY_HIGH,
               "violation of sync handler invariant");
    IPC_ASSERT(aMsg->is_rpc(), "can only Call() RPC messages here");


    nsAutoPtr<Message> msg(aMsg);

    msg->set_seqno(NextSeqno());
    msg->set_rpc_remote_stack_depth_guess(mRemoteStackDepthGuess);
    msg->set_rpc_local_stack_depth(1 + RPCStackDepth());
    mRPCStack.push(*msg);
    mLink->SendMessage(msg.forget());

    while (true) {
        
        
        
        
        
        if (!Connected()) {
            ReportConnectionError("MessageChannel::RPCCall");
            return false;
        }

        
        
        MaybeUndeferIncall();

        
        while (!RPCEventOccurred()) {
            bool maybeTimedOut = !WaitForRPCNotify();

            
            
            if (RPCEventOccurred() ||
                (!maybeTimedOut && (!mDeferred.empty() || !mOutOfTurnReplies.empty())))
            {
                break;
            }

            if (maybeTimedOut && !ShouldContinueFromTimeout())
                return false;
        }

        Message recvd;
        MessageMap::iterator it;

        if (mPendingUrgentRequest) {
            recvd = *mPendingUrgentRequest;
            mPendingUrgentRequest = nullptr;
        } else if ((it = mOutOfTurnReplies.find(mRPCStack.top().seqno()))
                    != mOutOfTurnReplies.end())
        {
            recvd = it->second;
            mOutOfTurnReplies.erase(it);
        } else if (!mPending.empty()) {
            recvd = mPending.front();
            mPending.pop_front();
        } else {
            
            
            
            
            
            continue;
        }

        
        if (!recvd.is_rpc()) {
            
            IPC_ASSERT(!recvd.is_sync() || mPending.empty(), "other side should be blocked");

            {
                MonitorAutoUnlock unlock(*mMonitor);
                CxxStackFrame frame(*this, IN_MESSAGE, &recvd);
                DispatchMessage(recvd);
            }
            if (!Connected()) {
                ReportConnectionError("MessageChannel::DispatchMessage");
                return false;
            }
            continue;
        }

        
        
        if (recvd.is_reply()) {
            IPC_ASSERT(!mRPCStack.empty(), "invalid RPC stack");

            
            
            {
                const Message &outcall = mRPCStack.top();

                
                
                if ((mSide == ChildSide && recvd.seqno() > outcall.seqno()) ||
                    (mSide != ChildSide && recvd.seqno() < outcall.seqno()))
                {
                    mOutOfTurnReplies[recvd.seqno()] = recvd;
                    continue;
                }

                IPC_ASSERT(recvd.is_reply_error() ||
                           (recvd.type() == (outcall.type() + 1) &&
                            recvd.seqno() == outcall.seqno()),
                           "somebody's misbehavin'", true);
            }

            
            
            mRPCStack.pop();

            if (!recvd.is_reply_error()) {
                *aReply = recvd;
            }

            
            
            IPC_ASSERT(!mRPCStack.empty() || mOutOfTurnReplies.empty(),
                       "still have pending replies with no pending out-calls",
                       true);

            return !recvd.is_reply_error();
        }

        
        
        size_t stackDepth = RPCStackDepth();
        {
            MonitorAutoUnlock unlock(*mMonitor);

            CxxStackFrame frame(*this, IN_MESSAGE, &recvd);
            DispatchRPCMessage(recvd, stackDepth);
        }
        if (!Connected()) {
            ReportConnectionError("MessageChannel::DispatchRPCMessage");
            return false;
        }
    }

    return true;
}

bool
MessageChannel::RPCEventOccurred()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();
    IPC_ASSERT(RPCStackDepth() > 0, "not in wait loop");

    return (!Connected() ||
            !mPending.empty() ||
            mPendingUrgentRequest ||
            (!mOutOfTurnReplies.empty() &&
             mOutOfTurnReplies.find(mRPCStack.top().seqno()) !=
             mOutOfTurnReplies.end()));
}

bool
MessageChannel::OnMaybeDequeueOne()
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

    Message recvd;
    do {
        MonitorAutoLock lock(*mMonitor);

        if (!Connected()) {
            ReportConnectionError("OnMaybeDequeueOne");
            return false;
        }

        if (mPendingUrgentRequest) {
            recvd = *mPendingUrgentRequest;
            mPendingUrgentRequest = nullptr;
            break;
        }

        if (!mDeferred.empty())
            MaybeUndeferIncall();

        if (mPending.empty())
            return false;

        recvd = mPending.front();
        mPending.pop_front();
    } while (0);

    if (IsOnCxxStack() && recvd.is_rpc() && recvd.is_reply()) {
        
        
        mOutOfTurnReplies[recvd.seqno()] = recvd;
        return false;
    }

    CxxStackFrame frame(*this, IN_MESSAGE, &recvd);
    DispatchMessage(recvd);
    return true;
}

void
MessageChannel::DispatchMessage(const Message &aMsg)
{
    if (aMsg.is_sync())
        DispatchSyncMessage(aMsg);
    else if (aMsg.is_urgent())
        DispatchUrgentMessage(aMsg);
    else if (aMsg.is_rpc())
        DispatchRPCMessage(aMsg, 0);
    else
        DispatchAsyncMessage(aMsg);
}

void
MessageChannel::DispatchSyncMessage(const Message& aMsg)
{
    AssertWorkerThread();

    Message *reply = nullptr;

    mDispatchingSyncMessage = true;
    Result rv = mListener->OnMessageReceived(aMsg, reply);
    mDispatchingSyncMessage = false;

    if (!MaybeHandleError(rv, "DispatchSyncMessage")) {
        delete reply;
        reply = new Message();
        reply->set_sync();
        reply->set_reply();
        reply->set_reply_error();
    }
    reply->set_seqno(aMsg.seqno());

    MonitorAutoLock lock(*mMonitor);
    if (ChannelConnected == mChannelState)
        mLink->SendMessage(reply);
}

void
MessageChannel::DispatchUrgentMessage(const Message& aMsg)
{
    AssertWorkerThread();
    MOZ_ASSERT(aMsg.is_urgent());

    Message *reply = nullptr;

    if (!MaybeHandleError(mListener->OnCallReceived(aMsg, reply), "DispatchUrgentMessage")) {
        delete reply;
        reply = new Message();
        reply->set_urgent();
        reply->set_reply();
        reply->set_reply_error();
    }
    reply->set_seqno(aMsg.seqno());

    MonitorAutoLock lock(*mMonitor);
    if (ChannelConnected == mChannelState)
        mLink->SendMessage(reply);
}

void
MessageChannel::DispatchAsyncMessage(const Message& aMsg)
{
    AssertWorkerThread();
    MOZ_ASSERT(!aMsg.is_rpc() && !aMsg.is_sync() && !aMsg.is_urgent());

    if (aMsg.routing_id() == MSG_ROUTING_NONE) {
        NS_RUNTIMEABORT("unhandled special message!");
    }

    MaybeHandleError(mListener->OnMessageReceived(aMsg), "DispatchAsyncMessage");
}

void
MessageChannel::DispatchRPCMessage(const Message& aMsg, size_t stackDepth)
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

    IPC_ASSERT(aMsg.is_rpc() && !aMsg.is_reply(), "wrong message type");

    
    
    
    if (aMsg.rpc_remote_stack_depth_guess() != RemoteViewOfStackDepth(stackDepth)) {
        
        
        bool defer;
        const char* winner;
        switch (mListener->MediateRPCRace((mSide == ChildSide) ? aMsg : mRPCStack.top(),
                                          (mSide != ChildSide) ? mRPCStack.top() : aMsg))
        {
          case RRPChildWins:
            winner = "child";
            defer = (mSide == ChildSide);
            break;
          case RRPParentWins:
            winner = "parent";
            defer = (mSide != ChildSide);
            break;
          case RRPError:
            NS_RUNTIMEABORT("NYI: 'Error' RPC race policy");
            return;
          default:
            NS_RUNTIMEABORT("not reached");
            return;
        }

        if (LoggingEnabled()) {
            printf_stderr("  (%s: %s won, so we're%sdeferring)\n",
                          (mSide == ChildSide) ? "child" : "parent",
                          winner,
                          defer ? " " : " not ");
        }

        if (defer) {
            
            
            ++mRemoteStackDepthGuess; 
            mDeferred.push(aMsg);
            return;
        }

        
        
        
    }

#ifdef OS_WIN
    SyncStackFrame frame(this, true);
#endif

    Message* reply = nullptr;

    ++mRemoteStackDepthGuess;
    Result rv = mListener->OnCallReceived(aMsg, reply);
    --mRemoteStackDepthGuess;

    if (!MaybeHandleError(rv, "DispatchRPCMessage")) {
        delete reply;
        reply = new Message();
        reply->set_rpc();
        reply->set_reply();
        reply->set_reply_error();
    }
    reply->set_seqno(aMsg.seqno());

    MonitorAutoLock lock(*mMonitor);
    if (ChannelConnected == mChannelState)
        mLink->SendMessage(reply);
}

void
MessageChannel::MaybeUndeferIncall()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();

    if (mDeferred.empty())
        return;

    size_t stackDepth = RPCStackDepth();

    
    IPC_ASSERT(mDeferred.top().rpc_remote_stack_depth_guess() <= stackDepth,
               "fatal logic error");

    if (mDeferred.top().rpc_remote_stack_depth_guess() < RemoteViewOfStackDepth(stackDepth))
        return;

    
    Message call = mDeferred.top();
    mDeferred.pop();

    
    IPC_ASSERT(0 < mRemoteStackDepthGuess, "fatal logic error");
    --mRemoteStackDepthGuess;

    mPending.push_back(call);
}

void
MessageChannel::FlushPendingRPCQueue()
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

    {
        MonitorAutoLock lock(*mMonitor);

        if (mDeferred.empty()) {
            if (mPending.empty())
                return;

            const Message& last = mPending.back();
            if (!last.is_rpc() || last.is_reply())
                return;
        }
    }

    while (OnMaybeDequeueOne());
}

void
MessageChannel::ExitedCxxStack()
{
    mListener->OnExitedCxxStack();
    if (mSawRPCOutMsg) {
        MonitorAutoLock lock(*mMonitor);
        
        EnqueuePendingMessages();
        mSawRPCOutMsg = false;
    }
}

void
MessageChannel::EnqueuePendingMessages()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();

    MaybeUndeferIncall();

    for (size_t i = 0; i < mDeferred.size(); ++i) {
        mWorkerLoop->PostTask(FROM_HERE, new DequeueTask(mDequeueOneTask));
    }

    
    

    for (size_t i = 0; i < mPending.size(); ++i) {
        mWorkerLoop->PostTask(FROM_HERE, new DequeueTask(mDequeueOneTask));
    }
}

static inline bool
IsTimeoutExpired(PRIntervalTime aStart, PRIntervalTime aTimeout)
{
    return (aTimeout != PR_INTERVAL_NO_TIMEOUT) &&
           (aTimeout <= (PR_IntervalNow() - aStart));
}

bool
MessageChannel::WaitResponse(bool aWaitTimedOut)
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
MessageChannel::WaitForSyncNotify()
{
    PRIntervalTime timeout = (kNoTimeout == mTimeoutMs) ?
                             PR_INTERVAL_NO_TIMEOUT :
                             PR_MillisecondsToInterval(mTimeoutMs);
    
    PRIntervalTime waitStart = PR_IntervalNow();

    mMonitor->Wait(timeout);

    
    
    return WaitResponse(IsTimeoutExpired(waitStart, timeout));
}

bool
MessageChannel::WaitForRPCNotify()
{
    return WaitForSyncNotify();
}

void
MessageChannel::NotifyWorkerThread()
{
    mMonitor->Notify();
}
#endif

bool
MessageChannel::ShouldContinueFromTimeout()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();

    bool cont;
    {
        MonitorAutoUnlock unlock(*mMonitor);
        cont = mListener->OnReplyTimeout();
    }

    static enum { UNKNOWN, NOT_DEBUGGING, DEBUGGING } sDebuggingChildren = UNKNOWN;

    if (sDebuggingChildren == UNKNOWN) {
        sDebuggingChildren = getenv("MOZ_DEBUG_CHILD_PROCESS") ? DEBUGGING : NOT_DEBUGGING;
    }
    if (sDebuggingChildren == DEBUGGING) {
        return true;
    }

    if (!cont) {
        
        
        
        
        
        
        
        
        
        
        SynchronouslyClose();
        mChannelState = ChannelTimeout;
    }

    return cont;
}

void
MessageChannel::SetReplyTimeoutMs(int32_t aTimeoutMs)
{
    
    
    AssertWorkerThread();
    mTimeoutMs = (aTimeoutMs <= 0)
                 ? kNoTimeout
                 : (int32_t)ceil((double)aTimeoutMs / 2.0);
}

void
MessageChannel::OnChannelConnected(int32_t peer_id)
{
    mWorkerLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this,
                          &MessageChannel::DispatchOnChannelConnected,
                          peer_id));
}

void
MessageChannel::DispatchOnChannelConnected(int32_t peer_pid)
{
    AssertWorkerThread();
    if (mListener)
        mListener->OnChannelConnected(peer_pid);
}


static void
PrintErrorMessage(Side side, const char* channelName, const char* msg)
{
    const char *from = (side == ChildSide)
                       ? "Child"
                       : ((side == ParentSide) ? "Parent" : "Unknown");
    printf_stderr("\n###!!! [%s][%s] Error: %s\n\n", from, channelName, msg);
}

void
MessageChannel::ReportConnectionError(const char* aChannelName) const
{
    const char* errorMsg = nullptr;
    switch (mChannelState) {
      case ChannelClosed:
        errorMsg = "Closed channel: cannot send/recv";
        break;
      case ChannelOpening:
        errorMsg = "Opening channel: not yet ready for send/recv";
        break;
      case ChannelTimeout:
        errorMsg = "Channel timeout: cannot send/recv";
        break;
      case ChannelClosing:
        errorMsg = "Channel closing: too late to send/recv, messages will be lost";
        break;
      case ChannelError:
        errorMsg = "Channel error: cannot send/recv";
        break;

      default:
        NS_RUNTIMEABORT("unreached");
    }

    PrintErrorMessage(mSide, aChannelName, errorMsg);
    mListener->OnProcessingError(MsgDropped);
}

bool
MessageChannel::MaybeHandleError(Result code, const char* channelName)
{
    if (MsgProcessed == code)
        return true;

    const char* errorMsg = nullptr;
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
      case MsgProcessingError:
        errorMsg = "Processing error: message was deserialized, but the handler returned false (indicating failure)";
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

    PrintErrorMessage(mSide, channelName, errorMsg);

    mListener->OnProcessingError(code);

    return false;
}

void
MessageChannel::OnChannelErrorFromLink()
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (RPCStackDepth() > 0)
        NotifyWorkerThread();

    if (AwaitingSyncReply())
        NotifyWorkerThread();

    if (ChannelClosing != mChannelState) {
        mChannelState = ChannelError;
        mMonitor->Notify();
    }

    PostErrorNotifyTask();
}

void
MessageChannel::NotifyMaybeChannelError()
{
    mMonitor->AssertNotCurrentThreadOwns();

    
    if (ChannelClosing == mChannelState) {
        
        
        mChannelState = ChannelClosed;
        NotifyChannelClosed();
        return;
    }

    
    mChannelState = ChannelError;
    mListener->OnChannelError();
    Clear();
}

void
MessageChannel::OnNotifyMaybeChannelError()
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

    mChannelErrorTask = nullptr;

    
    
    
    
    {
        MonitorAutoLock lock(*mMonitor);
        
    }

    if (IsOnCxxStack()) {
        mChannelErrorTask =
            NewRunnableMethod(this, &MessageChannel::OnNotifyMaybeChannelError);
        
        mWorkerLoop->PostDelayedTask(FROM_HERE, mChannelErrorTask, 10);
        return;
    }

    NotifyMaybeChannelError();
}

void
MessageChannel::PostErrorNotifyTask()
{
    mMonitor->AssertCurrentThreadOwns();

    if (mChannelErrorTask)
        return;

    
    mChannelErrorTask =
        NewRunnableMethod(this, &MessageChannel::OnNotifyMaybeChannelError);
    mWorkerLoop->PostTask(FROM_HERE, mChannelErrorTask);
}


class GoodbyeMessage : public IPC::Message
{
public:
    GoodbyeMessage() :
        IPC::Message(MSG_ROUTING_NONE, GOODBYE_MESSAGE_TYPE, PRIORITY_NORMAL)
    {
    }
    static bool Read(const Message* msg) {
        return true;
    }
    void Log(const std::string& aPrefix, FILE* aOutf) const {
        fputs("(special `Goodbye' message)", aOutf);
    }
};

void
MessageChannel::SynchronouslyClose()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();
    mLink->SendClose();
    while (ChannelClosed != mChannelState)
        mMonitor->Wait();
}

void
MessageChannel::CloseWithError()
{
    AssertWorkerThread();

    MonitorAutoLock lock(*mMonitor);
    if (ChannelConnected != mChannelState) {
        return;
    }
    SynchronouslyClose();
    mChannelState = ChannelError;
    PostErrorNotifyTask();
}

void
MessageChannel::Close()
{
    AssertWorkerThread();

    {
        MonitorAutoLock lock(*mMonitor);

        if (ChannelError == mChannelState || ChannelTimeout == mChannelState) {
            
            
            
            
            
            if (mListener) {
                MonitorAutoUnlock unlock(*mMonitor);
                NotifyMaybeChannelError();
            }
            return;
        }

        if (ChannelConnected != mChannelState) {
            
            
            NS_RUNTIMEABORT("Close() called on closed channel!");
        }

        AssertWorkerThread();

        
        mLink->SendMessage(new GoodbyeMessage());
        SynchronouslyClose();
    }

    NotifyChannelClosed();
}

void
MessageChannel::NotifyChannelClosed()
{
    mMonitor->AssertNotCurrentThreadOwns();

    if (ChannelClosed != mChannelState)
        NS_RUNTIMEABORT("channel should have been closed!");

    
    
    mListener->OnChannelClose();

    Clear();
}

void
MessageChannel::DebugAbort(const char* file, int line, const char* cond,
                           const char* why,
                           bool reply) const
{
    printf_stderr("###!!! [RPCChannel][%s][%s:%d] "
                  "Assertion (%s) failed.  %s %s\n",
                  mSide == ChildSide ? "Child" : "Parent",
                  file, line, cond,
                  why,
                  reply ? "(reply)" : "");
    
    DumpRPCStack("  ");
    printf_stderr("  remote RPC stack guess: %lu\n",
                  mRemoteStackDepthGuess);
    printf_stderr("  deferred stack size: %lu\n",
                  mDeferred.size());
    printf_stderr("  out-of-turn RPC replies stack size: %lu\n",
                  mOutOfTurnReplies.size());
    printf_stderr("  Pending queue size: %lu, front to back:\n",
                  mPending.size());

    MessageQueue pending = mPending;
    while (!pending.empty()) {
        printf_stderr("    [ %s%s ]\n",
                      pending.front().is_rpc() ? "rpc" :
                      (pending.front().is_sync() ? "sync" : "async"),
                      pending.front().is_reply() ? "reply" : "");
        pending.pop_front();
    }

    NS_RUNTIMEABORT(why);
}

void
MessageChannel::DumpRPCStack(const char* const pfx) const
{
    NS_WARN_IF_FALSE(MessageLoop::current() != mWorkerLoop,
                     "The worker thread had better be paused in a debugger!");

    printf_stderr("%sRPCChannel 'backtrace':\n", pfx);

    
    for (uint32_t i = 0; i < mCxxStackFrames.size(); ++i) {
        int32_t id;
        const char* dir, *sems, *name;
        mCxxStackFrames[i].Describe(&id, &dir, &sems, &name);

        printf_stderr("%s[(%u) %s %s %s(actor=%d) ]\n", pfx,
                      i, dir, sems, name, id);
    }
}

} 
} 

