






#include "mozilla/ipc/MessageChannel.h"
#include "mozilla/ipc/ProtocolUtils.h"

#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Move.h"
#include "nsDebug.h"
#include "nsISupportsImpl.h"
#include "nsContentUtils.h"

#include "prprf.h"


#undef compress

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

static uintptr_t gDispatchingUrgentMessageCount;

namespace mozilla {
namespace ipc {

const int32_t MessageChannel::kNoTimeout = INT32_MIN;


bool MessageChannel::sIsPumpingMessages = false;

enum Direction
{
    IN_MESSAGE,
    OUT_MESSAGE
};


class MessageChannel::InterruptFrame
{
private:
    enum Semantics
    {
        INTR_SEMS,
        SYNC_SEMS,
        ASYNC_SEMS
    };

public:
    InterruptFrame(Direction direction, const Message* msg)
      : mMessageName(strdup(msg->name())),
        mMessageRoutingId(msg->routing_id()),
        mMesageSemantics(msg->is_interrupt() ? INTR_SEMS :
                          msg->is_sync() ? SYNC_SEMS :
                          ASYNC_SEMS),
        mDirection(direction),
        mMoved(false)
    {
        MOZ_ASSERT(mMessageName);
    }

    InterruptFrame(InterruptFrame&& aOther)
    {
        MOZ_ASSERT(aOther.mMessageName);
        mMessageName = aOther.mMessageName;
        aOther.mMessageName = nullptr;
        aOther.mMoved = true;

        mMessageRoutingId = aOther.mMessageRoutingId;
        mMesageSemantics = aOther.mMesageSemantics;
        mDirection = aOther.mDirection;
    }

    ~InterruptFrame()
    {
        MOZ_ASSERT_IF(!mMessageName, mMoved);

        if (mMessageName)
            free(const_cast<char*>(mMessageName));
    }

    InterruptFrame& operator=(InterruptFrame&& aOther)
    {
        MOZ_ASSERT(&aOther != this);
        this->~InterruptFrame();
        new (this) InterruptFrame(mozilla::Move(aOther));
        return *this;
    }

    bool IsInterruptIncall() const
    {
        return INTR_SEMS == mMesageSemantics && IN_MESSAGE == mDirection;
    }

    bool IsInterruptOutcall() const
    {
        return INTR_SEMS == mMesageSemantics && OUT_MESSAGE == mDirection;
    }

    void Describe(int32_t* id, const char** dir, const char** sems,
                  const char** name) const
    {
        *id = mMessageRoutingId;
        *dir = (IN_MESSAGE == mDirection) ? "in" : "out";
        *sems = (INTR_SEMS == mMesageSemantics) ? "intr" :
                (SYNC_SEMS == mMesageSemantics) ? "sync" :
                "async";
        *name = mMessageName;
    }

private:
    const char* mMessageName;
    int32_t mMessageRoutingId;
    Semantics mMesageSemantics;
    Direction mDirection;
    DebugOnly<bool> mMoved;

    
    InterruptFrame(const InterruptFrame& aOther) MOZ_DELETE;
    InterruptFrame& operator=(const InterruptFrame&) MOZ_DELETE;
};

class MOZ_STACK_CLASS MessageChannel::CxxStackFrame
{
public:
    CxxStackFrame(MessageChannel& that, Direction direction, const Message* msg)
      : mThat(that)
    {
        mThat.AssertWorkerThread();

        if (mThat.mCxxStackFrames.empty())
            mThat.EnteredCxxStack();

        mThat.mCxxStackFrames.append(InterruptFrame(direction, msg));

        const InterruptFrame& frame = mThat.mCxxStackFrames.back();

        if (frame.IsInterruptIncall())
            mThat.EnteredCall();

        mThat.mSawInterruptOutMsg |= frame.IsInterruptOutcall();
    }

    ~CxxStackFrame() {
        mThat.AssertWorkerThread();

        MOZ_ASSERT(!mThat.mCxxStackFrames.empty());

        bool exitingCall = mThat.mCxxStackFrames.back().IsInterruptIncall();
        mThat.mCxxStackFrames.shrinkBy(1);

        bool exitingStack = mThat.mCxxStackFrames.empty();

        
        
        if (!mThat.mListener)
            return;

        if (exitingCall)
            mThat.ExitedCall();

        if (exitingStack)
            mThat.ExitedCxxStack();
    }
private:
    MessageChannel& mThat;

    
    CxxStackFrame() MOZ_DELETE;
    CxxStackFrame(const CxxStackFrame&) MOZ_DELETE;
    CxxStackFrame& operator=(const CxxStackFrame&) MOZ_DELETE;
};

namespace {

class MOZ_STACK_CLASS MaybeScriptBlocker {
public:
    explicit MaybeScriptBlocker(MessageChannel *aChannel
                                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : mBlocked(aChannel->ShouldBlockScripts())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        if (mBlocked) {
            nsContentUtils::AddScriptBlocker();
        }
    }
    ~MaybeScriptBlocker() {
        if (mBlocked) {
            nsContentUtils::RemoveScriptBlocker();
        }
    }
private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    bool mBlocked;
};

} 

MessageChannel::MessageChannel(MessageListener *aListener)
  : mListener(aListener),
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
    mPendingRPCReplies(0),
    mCurrentRPCTransaction(0),
    mDispatchingSyncMessage(false),
    mDispatchingUrgentMessageCount(0),
    mRemoteStackDepthGuess(false),
    mSawInterruptOutMsg(false),
    mAbortOnError(false),
    mBlockScripts(false),
    mFlags(REQUIRE_DEFAULT),
    mPeerPidSet(false),
    mPeerPid(-1)
{
    MOZ_COUNT_CTOR(ipc::MessageChannel);

#ifdef OS_WIN
    mTopFrame = nullptr;
    mIsSyncWaitingOnNonMainThread = false;
#endif

    mDequeueOneTask = new RefCountedTask(NewRunnableMethod(
                                                 this,
                                                 &MessageChannel::OnMaybeDequeueOne));

    mOnChannelConnectedTask = new RefCountedTask(NewRunnableMethod(
        this,
        &MessageChannel::DispatchOnChannelConnected));

#ifdef OS_WIN
    mEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    NS_ASSERTION(mEvent, "CreateEvent failed! Nothing is going to work!");
#endif
}

MessageChannel::~MessageChannel()
{
    MOZ_COUNT_DTOR(ipc::MessageChannel);
    IPC_ASSERT(mCxxStackFrames.empty(), "mismatched CxxStackFrame ctor/dtors");
#ifdef OS_WIN
    DebugOnly<BOOL> ok = CloseHandle(mEvent);
    MOZ_ASSERT(ok);
#endif
    Clear();
}

static void
PrintErrorMessage(Side side, const char* channelName, const char* msg)
{
    const char *from = (side == ChildSide)
                       ? "Child"
                       : ((side == ParentSide) ? "Parent" : "Unknown");
    printf_stderr("\n###!!! [%s][%s] Error: %s\n\n", from, channelName, msg);
}

bool
MessageChannel::Connected() const
{
    mMonitor->AssertCurrentThreadOwns();

    
    
    return (ChannelOpening == mChannelState || ChannelConnected == mChannelState);
}

bool
MessageChannel::CanSend() const
{
    MonitorAutoLock lock(*mMonitor);
    return Connected();
}

void
MessageChannel::Clear()
{
    
    
    
    
    
    
    
    
    

    mDequeueOneTask->Cancel();

    mWorkerLoop = nullptr;
    delete mLink;
    mLink = nullptr;

    mOnChannelConnectedTask->Cancel();

    if (mChannelErrorTask) {
        mChannelErrorTask->Cancel();
        mChannelErrorTask = nullptr;
    }

    
    mPending.clear();
    mPendingUrgentRequest = nullptr;
    mPendingRPCCall = nullptr;
    mOutOfTurnReplies.clear();
    while (!mDeferred.empty()) {
        mDeferred.pop();
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
    if (MSG_ROUTING_NONE == msg->routing_id()) {
        ReportMessageRouteError("MessageChannel::Echo");
        return false;
    }

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
    CxxStackFrame frame(*this, OUT_MESSAGE, aMsg);

    nsAutoPtr<Message> msg(aMsg);
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();
    if (MSG_ROUTING_NONE == msg->routing_id()) {
        ReportMessageRouteError("MessageChannel::Send");
        return false;
    }

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
        if (LoggingEnabled()) {
            printf("NOTE: %s process received `Goodbye', closing down\n",
                   (mSide == ChildSide) ? "child" : "parent");
        }
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
        (AwaitingUrgentReply() && aMsg.is_urgent()) ||
        (AwaitingRPCReply() && aMsg.is_rpc()))
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

    bool shouldWakeUp = AwaitingInterruptReply() ||
                        
                        (AwaitingUrgentReply() && aMsg.is_rpc()) ||
                        
                        ((AwaitingSyncReply() || AwaitingRPCReply()) && aMsg.is_urgent());

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (shouldWakeUp && (AwaitingUrgentReply() && aMsg.is_rpc())) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (aMsg.transaction_id() != mCurrentRPCTransaction)
            shouldWakeUp = false;
    }

    if (aMsg.is_urgent()) {
        MOZ_ASSERT(!mPendingUrgentRequest);
        mPendingUrgentRequest = new Message(aMsg);
    } else if (aMsg.is_rpc() && shouldWakeUp) {
        
        
        MOZ_ASSERT(!mPendingRPCCall);
        mPendingRPCCall = new Message(aMsg);
    } else {
        mPending.push_back(aMsg);
    }

    if (shouldWakeUp) {
        
        
        
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
    
    MaybeScriptBlocker scriptBlocker(this);

    
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

#ifdef OS_WIN
    SyncStackFrame frame(this, false);
#endif

    CxxStackFrame f(*this, OUT_MESSAGE, aMsg);

    MonitorAutoLock lock(*mMonitor);

    IPC_ASSERT(aMsg->is_sync(), "can only Send() sync messages here");
    IPC_ASSERT(!DispatchingSyncMessage(), "violation of sync handler invariant");
    IPC_ASSERT(!DispatchingUrgentMessage(), "sync messages forbidden while handling urgent message");
    IPC_ASSERT(!AwaitingSyncReply(), "nested sync messages are not supported");

    AutoEnterPendingReply replies(mPendingSyncReplies);
    if (!SendAndWait(aMsg, aReply))
        return false;

    NS_ABORT_IF_FALSE(aReply->is_sync(), "reply is not sync");
    return true;
}

bool
MessageChannel::UrgentCall(Message* aMsg, Message* aReply)
{
    
    MaybeScriptBlocker scriptBlocker(this);

    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();
    IPC_ASSERT(mSide == ParentSide, "cannot send urgent requests from child");

#ifdef OS_WIN
    SyncStackFrame frame(this, false);
#endif

    CxxStackFrame f(*this, OUT_MESSAGE, aMsg);

    MonitorAutoLock lock(*mMonitor);

    IPC_ASSERT(!AwaitingInterruptReply(), "urgent calls cannot be issued within Interrupt calls");
    IPC_ASSERT(!AwaitingSyncReply(), "urgent calls cannot be issued within sync sends");

    AutoEnterRPCTransaction transact(this);
    aMsg->set_transaction_id(mCurrentRPCTransaction);

    AutoEnterPendingReply replies(mPendingUrgentReplies);
    if (!SendAndWait(aMsg, aReply))
        return false;

    NS_ABORT_IF_FALSE(aReply->is_urgent(), "reply is not urgent");
    return true;
}

bool
MessageChannel::RPCCall(Message* aMsg, Message* aReply)
{
    
    MaybeScriptBlocker scriptBlocker(this);

    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();
    IPC_ASSERT(mSide == ChildSide, "cannot send rpc messages from parent");

#ifdef OS_WIN
    SyncStackFrame frame(this, false);
#endif

    CxxStackFrame f(*this, OUT_MESSAGE, aMsg);

    MonitorAutoLock lock(*mMonitor);

    AutoEnterRPCTransaction transact(this);
    aMsg->set_transaction_id(mCurrentRPCTransaction);

    AutoEnterPendingReply replies(mPendingRPCReplies);
    if (!SendAndWait(aMsg, aReply))
        return false;

    NS_ABORT_IF_FALSE(aReply->is_rpc(), "expected rpc reply");
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
            if (mRecvd || mPendingUrgentRequest || mPendingRPCCall)
                break;

            bool maybeTimedOut = !WaitForSyncNotify();

            if (!Connected()) {
                ReportConnectionError("MessageChannel::SendAndWait");
                return false;
            }

            if (maybeTimedOut && !ShouldContinueFromTimeout())
                return false;
        }

        if (mPendingUrgentRequest && !ProcessPendingUrgentRequest())
            return false;

        if (mPendingRPCCall && !ProcessPendingRPCCall())
            return false;

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
    if (aMsg->is_rpc())
        return RPCCall(aMsg, aReply);
    return InterruptCall(aMsg, aReply);
}

bool
MessageChannel::InterruptCall(Message* aMsg, Message* aReply)
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

#ifdef OS_WIN
    SyncStackFrame frame(this, true);
#endif

    
    
    CxxStackFrame cxxframe(*this, OUT_MESSAGE, aMsg);

    MonitorAutoLock lock(*mMonitor);
    if (!Connected()) {
        ReportConnectionError("MessageChannel::Call");
        return false;
    }

    
    IPC_ASSERT(!AwaitingSyncReply() && !AwaitingUrgentReply(),
               "cannot issue Interrupt call whiel blocked on sync or urgent");
    IPC_ASSERT(!DispatchingSyncMessage() || aMsg->priority() == IPC::Message::PRIORITY_HIGH,
               "violation of sync handler invariant");
    IPC_ASSERT(aMsg->is_interrupt(), "can only Call() Interrupt messages here");


    nsAutoPtr<Message> msg(aMsg);

    msg->set_seqno(NextSeqno());
    msg->set_interrupt_remote_stack_depth_guess(mRemoteStackDepthGuess);
    msg->set_interrupt_local_stack_depth(1 + InterruptStackDepth());
    mInterruptStack.push(*msg);
    mLink->SendMessage(msg.forget());

    while (true) {
        
        
        
        
        
        if (!Connected()) {
            ReportConnectionError("MessageChannel::InterruptCall");
            return false;
        }

        
        
        MaybeUndeferIncall();

        
        while (!InterruptEventOccurred()) {
            bool maybeTimedOut = !WaitForInterruptNotify();

            
            
            if (InterruptEventOccurred() ||
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
        } else if (mPendingRPCCall) {
            recvd = *mPendingRPCCall;
            mPendingRPCCall = nullptr;
        } else if ((it = mOutOfTurnReplies.find(mInterruptStack.top().seqno()))
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

        
        if (!recvd.is_interrupt()) {
            
            IPC_ASSERT(!recvd.is_sync() || mPending.empty(), "other side should be blocked");

            {
                AutoEnterRPCTransaction transaction(this, &recvd);
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
            IPC_ASSERT(!mInterruptStack.empty(), "invalid Interrupt stack");

            
            
            {
                const Message &outcall = mInterruptStack.top();

                
                
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

            
            
            mInterruptStack.pop();

            if (!recvd.is_reply_error()) {
                *aReply = recvd;
            }

            
            
            IPC_ASSERT(!mInterruptStack.empty() || mOutOfTurnReplies.empty(),
                       "still have pending replies with no pending out-calls",
                       true);

            return !recvd.is_reply_error();
        }

        
        
        size_t stackDepth = InterruptStackDepth();
        {
            MonitorAutoUnlock unlock(*mMonitor);

            CxxStackFrame frame(*this, IN_MESSAGE, &recvd);
            DispatchInterruptMessage(recvd, stackDepth);
        }
        if (!Connected()) {
            ReportConnectionError("MessageChannel::DispatchInterruptMessage");
            return false;
        }
    }

    return true;
}

bool
MessageChannel::InterruptEventOccurred()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();
    IPC_ASSERT(InterruptStackDepth() > 0, "not in wait loop");

    return (!Connected() ||
            !mPending.empty() ||
            mPendingUrgentRequest ||
            mPendingRPCCall ||
            (!mOutOfTurnReplies.empty() &&
             mOutOfTurnReplies.find(mInterruptStack.top().seqno()) !=
             mOutOfTurnReplies.end()));
}

bool
MessageChannel::ProcessPendingUrgentRequest()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();

    
    
    
    
    
    
    
    
    nsAutoPtr<Message> savedReply(mRecvd.forget());

    
    IPC_ASSERT(!mPendingRPCCall, "unexpected RPC call");

    nsAutoPtr<Message> recvd(mPendingUrgentRequest.forget());
    {
        
        
        AutoEnterRPCTransaction transaction(this, recvd);

        MonitorAutoUnlock unlock(*mMonitor);
        DispatchUrgentMessage(*recvd);
    }
    if (!Connected()) {
        ReportConnectionError("MessageChannel::DispatchUrgentMessage");
        return false;
    }

    
    
    
    
    IPC_ASSERT(!mRecvd || !savedReply, "unknown reply");
    if (!mRecvd)
        mRecvd = savedReply.forget();
    return true;
}

bool
MessageChannel::ProcessPendingRPCCall()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();

    
    nsAutoPtr<Message> savedReply(mRecvd.forget());

    IPC_ASSERT(!mPendingUrgentRequest, "unexpected urgent message");

    nsAutoPtr<Message> recvd(mPendingRPCCall.forget());
    {
        
        
        
        
        AutoEnterRPCTransaction transaction(this, recvd);

        MonitorAutoUnlock unlock(*mMonitor);
        DispatchRPCMessage(*recvd);
    }
    if (!Connected()) {
        ReportConnectionError("MessageChannel::DispatchRPCMessage");
        return false;
    }

    
    
    
    
    IPC_ASSERT(!mRecvd || !savedReply, "unknown reply");
    if (!mRecvd)
        mRecvd = savedReply.forget();
    return true;
}

bool
MessageChannel::DequeueOne(Message *recvd)
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();

    if (!Connected()) {
        ReportConnectionError("OnMaybeDequeueOne");
        return false;
    }

    if (mPendingUrgentRequest) {
        *recvd = *mPendingUrgentRequest;
        mPendingUrgentRequest = nullptr;
        return true;
    }

    if (mPendingRPCCall) {
        *recvd = *mPendingRPCCall;
        mPendingRPCCall = nullptr;
        return true;
    }

    if (!mDeferred.empty())
        MaybeUndeferIncall();

    if (mPending.empty())
        return false;

    *recvd = mPending.front();
    mPending.pop_front();
    return true;
}

bool
MessageChannel::OnMaybeDequeueOne()
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

    Message recvd;

    MonitorAutoLock lock(*mMonitor);
    if (!DequeueOne(&recvd))
        return false;

    if (IsOnCxxStack() && recvd.is_interrupt() && recvd.is_reply()) {
        
        
        mOutOfTurnReplies[recvd.seqno()] = recvd;
        return false;
    }

    {
        
        MOZ_ASSERT(mCurrentRPCTransaction == 0);
        AutoEnterRPCTransaction transaction(this, &recvd);

        MonitorAutoUnlock unlock(*mMonitor);

        CxxStackFrame frame(*this, IN_MESSAGE, &recvd);
        DispatchMessage(recvd);
    }
    return true;
}

void
MessageChannel::DispatchMessage(const Message &aMsg)
{
    if (aMsg.is_sync())
        DispatchSyncMessage(aMsg);
    else if (aMsg.is_urgent())
        DispatchUrgentMessage(aMsg);
    else if (aMsg.is_interrupt())
        DispatchInterruptMessage(aMsg, 0);
    else if (aMsg.is_rpc())
        DispatchRPCMessage(aMsg);
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

    if (!MaybeHandleError(rv, aMsg, "DispatchSyncMessage")) {
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

    MOZ_ASSERT(NS_IsMainThread());

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    MaybeScriptBlocker scriptBlocker(this);

    gDispatchingUrgentMessageCount++;
    mDispatchingUrgentMessageCount++;
    Result rv = mListener->OnCallReceived(aMsg, reply);
    mDispatchingUrgentMessageCount--;
    gDispatchingUrgentMessageCount--;

    if (!MaybeHandleError(rv, aMsg, "DispatchUrgentMessage")) {
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
MessageChannel::DispatchRPCMessage(const Message& aMsg)
{
    AssertWorkerThread();
    MOZ_ASSERT(aMsg.is_rpc());

    Message *reply = nullptr;

    if (!MaybeHandleError(mListener->OnCallReceived(aMsg, reply), aMsg, "DispatchRPCMessage")) {
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
MessageChannel::DispatchAsyncMessage(const Message& aMsg)
{
    AssertWorkerThread();
    MOZ_ASSERT(!aMsg.is_interrupt() && !aMsg.is_sync() && !aMsg.is_urgent());

    if (aMsg.routing_id() == MSG_ROUTING_NONE) {
        NS_RUNTIMEABORT("unhandled special message!");
    }

    MaybeHandleError(mListener->OnMessageReceived(aMsg), aMsg, "DispatchAsyncMessage");
}

void
MessageChannel::DispatchInterruptMessage(const Message& aMsg, size_t stackDepth)
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

    IPC_ASSERT(aMsg.is_interrupt() && !aMsg.is_reply(), "wrong message type");

    
    
    
    if (aMsg.interrupt_remote_stack_depth_guess() != RemoteViewOfStackDepth(stackDepth)) {
        
        
        bool defer;
        const char* winner;
        switch (mListener->MediateInterruptRace((mSide == ChildSide) ? aMsg : mInterruptStack.top(),
                                          (mSide != ChildSide) ? mInterruptStack.top() : aMsg))
        {
          case RIPChildWins:
            winner = "child";
            defer = (mSide == ChildSide);
            break;
          case RIPParentWins:
            winner = "parent";
            defer = (mSide != ChildSide);
            break;
          case RIPError:
            NS_RUNTIMEABORT("NYI: 'Error' Interrupt race policy");
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

    if (!MaybeHandleError(rv, aMsg, "DispatchInterruptMessage")) {
        delete reply;
        reply = new Message();
        reply->set_interrupt();
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

    size_t stackDepth = InterruptStackDepth();

    
    IPC_ASSERT(mDeferred.top().interrupt_remote_stack_depth_guess() <= stackDepth,
               "fatal logic error");

    if (mDeferred.top().interrupt_remote_stack_depth_guess() < RemoteViewOfStackDepth(stackDepth))
        return;

    
    Message call = mDeferred.top();
    mDeferred.pop();

    
    IPC_ASSERT(0 < mRemoteStackDepthGuess, "fatal logic error");
    --mRemoteStackDepthGuess;

    mPending.push_back(call);
}

void
MessageChannel::FlushPendingInterruptQueue()
{
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

    {
        MonitorAutoLock lock(*mMonitor);

        if (mDeferred.empty()) {
            if (mPending.empty())
                return;

            const Message& last = mPending.back();
            if (!last.is_interrupt() || last.is_reply())
                return;
        }
    }

    while (OnMaybeDequeueOne());
}

void
MessageChannel::ExitedCxxStack()
{
    mListener->OnExitedCxxStack();
    if (mSawInterruptOutMsg) {
        MonitorAutoLock lock(*mMonitor);
        
        EnqueuePendingMessages();
        mSawInterruptOutMsg = false;
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
MessageChannel::WaitForInterruptNotify()
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
    MOZ_ASSERT(!mPeerPidSet);
    mPeerPidSet = true;
    mPeerPid = peer_id;
    mWorkerLoop->PostTask(FROM_HERE, new DequeueTask(mOnChannelConnectedTask));
}

void
MessageChannel::DispatchOnChannelConnected()
{
    AssertWorkerThread();
    MOZ_ASSERT(mPeerPidSet);
    if (mListener)
        mListener->OnChannelConnected(mPeerPid);
}

void
MessageChannel::ReportMessageRouteError(const char* channelName) const
{
    PrintErrorMessage(mSide, channelName, "Need a route");
    mListener->OnProcessingError(MsgRouteError);
}

void
MessageChannel::ReportConnectionError(const char* aChannelName) const
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();

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

    MonitorAutoUnlock unlock(*mMonitor);
    mListener->OnProcessingError(MsgDropped);
}

bool
MessageChannel::MaybeHandleError(Result code, const Message& aMsg, const char* channelName)
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

    char printedMsg[512];
    PR_snprintf(printedMsg, sizeof(printedMsg),
                "(msgtype=0x%lX,name=%s) %s",
                aMsg.type(), aMsg.name(), errorMsg);

    PrintErrorMessage(mSide, channelName, printedMsg);

    mListener->OnProcessingError(code);

    return false;
}

void
MessageChannel::OnChannelErrorFromLink()
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (InterruptStackDepth() > 0)
        NotifyWorkerThread();

    if (AwaitingSyncReply() || AwaitingRPCReply() || AwaitingUrgentReply())
        NotifyWorkerThread();

    if (ChannelClosing != mChannelState) {
        if (mAbortOnError) {
            NS_RUNTIMEABORT("Aborting on channel error.");
        }
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
MessageChannel::BlockScripts()
{
    MOZ_ASSERT(NS_IsMainThread());
    mBlockScripts = true;
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

        if (ChannelOpening == mChannelState) {
            
            
            SynchronouslyClose();
            mChannelState = ChannelError;
            NotifyMaybeChannelError();
            return;
        }

        if (ChannelConnected != mChannelState) {
            
            
            NS_RUNTIMEABORT("Close() called on closed channel!");
        }

        
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
    printf_stderr("###!!! [MessageChannel][%s][%s:%d] "
                  "Assertion (%s) failed.  %s %s\n",
                  mSide == ChildSide ? "Child" : "Parent",
                  file, line, cond,
                  why,
                  reply ? "(reply)" : "");
    
    DumpInterruptStack("  ");
    printf_stderr("  remote Interrupt stack guess: %" PRIuSIZE "\n",
                  mRemoteStackDepthGuess);
    printf_stderr("  deferred stack size: %" PRIuSIZE "\n",
                  mDeferred.size());
    printf_stderr("  out-of-turn Interrupt replies stack size: %" PRIuSIZE "\n",
                  mOutOfTurnReplies.size());
    printf_stderr("  Pending queue size: %" PRIuSIZE ", front to back:\n",
                  mPending.size());

    MessageQueue pending = mPending;
    while (!pending.empty()) {
        printf_stderr("    [ %s%s ]\n",
                      pending.front().is_interrupt() ? "intr" :
                      (pending.front().is_sync() ? "sync" : "async"),
                      pending.front().is_reply() ? "reply" : "");
        pending.pop_front();
    }

    NS_RUNTIMEABORT(why);
}

void
MessageChannel::DumpInterruptStack(const char* const pfx) const
{
    NS_WARN_IF_FALSE(MessageLoop::current() != mWorkerLoop,
                     "The worker thread had better be paused in a debugger!");

    printf_stderr("%sMessageChannel 'backtrace':\n", pfx);

    
    for (uint32_t i = 0; i < mCxxStackFrames.length(); ++i) {
        int32_t id;
        const char* dir;
        const char* sems;
        const char* name;
        mCxxStackFrames[i].Describe(&id, &dir, &sems, &name);

        printf_stderr("%s[(%u) %s %s %s(actor=%d) ]\n", pfx,
                      i, dir, sems, name, id);
    }
}

bool
ProcessingUrgentMessages()
{
    return gDispatchingUrgentMessageCount > 0;
}

} 
} 
