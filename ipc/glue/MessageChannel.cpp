






#include "mozilla/ipc/MessageChannel.h"
#include "mozilla/ipc/ProtocolUtils.h"

#include "mozilla/dom/ScriptSettings.h"

#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Move.h"
#include "mozilla/SizePrintfMacros.h"
#include "nsDebug.h"
#include "nsISupportsImpl.h"
#include "nsContentUtils.h"

#include "prprf.h"


#undef compress

























































using namespace mozilla;
using namespace std;

using mozilla::dom::AutoNoJSAPI;
using mozilla::dom::ScriptSettingsInitialized;
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

static bool gParentIsBlocked;

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

    bool IsOutgoingSync() const {
        return (mMesageSemantics == INTR_SEMS || mMesageSemantics == SYNC_SEMS) &&
               mDirection == OUT_MESSAGE;
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

    int32_t GetRoutingId() const
    {
        return mMessageRoutingId;
    }

private:
    const char* mMessageName;
    int32_t mMessageRoutingId;
    Semantics mMesageSemantics;
    Direction mDirection;
    DebugOnly<bool> mMoved;

    
    InterruptFrame(const InterruptFrame& aOther) = delete;
    InterruptFrame& operator=(const InterruptFrame&) = delete;
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

        if (frame.IsOutgoingSync())
            mThat.EnteredSyncSend();

        mThat.mSawInterruptOutMsg |= frame.IsInterruptOutcall();
    }

    ~CxxStackFrame() {
        mThat.AssertWorkerThread();

        MOZ_ASSERT(!mThat.mCxxStackFrames.empty());

        const InterruptFrame& frame = mThat.mCxxStackFrames.back();
        bool exitingSync = frame.IsOutgoingSync();
        bool exitingCall = frame.IsInterruptIncall();
        mThat.mCxxStackFrames.shrinkBy(1);

        bool exitingStack = mThat.mCxxStackFrames.empty();

        
        
        if (!mThat.mListener)
            return;

        if (exitingCall)
            mThat.ExitedCall();

        if (exitingSync)
            mThat.ExitedSyncSend();

        if (exitingStack)
            mThat.ExitedCxxStack();
    }
private:
    MessageChannel& mThat;

    
    CxxStackFrame() = delete;
    CxxStackFrame(const CxxStackFrame&) = delete;
    CxxStackFrame& operator=(const CxxStackFrame&) = delete;
};

namespace {

class MOZ_STACK_CLASS MaybeScriptBlocker {
public:
    explicit MaybeScriptBlocker(MessageChannel *aChannel, bool aBlock
                                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : mBlocked(aChannel->ShouldBlockScripts() && aBlock)
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
    mAwaitingSyncReply(false),
    mAwaitingSyncReplyPriority(0),
    mDispatchingSyncMessage(false),
    mDispatchingSyncMessagePriority(0),
    mDispatchingAsyncMessage(false),
    mDispatchingAsyncMessagePriority(0),
    mCurrentTransaction(0),
    mTimedOutMessageSeqno(0),
    mRecvdErrors(0),
    mRemoteStackDepthGuess(false),
    mSawInterruptOutMsg(false),
    mIsWaitingForIncoming(false),
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
    if (!mMonitor) {
        return false;
    }
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
    mRecvd = nullptr;
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

bool
MessageChannel::ShouldDeferMessage(const Message& aMsg)
{
    
    
    
    if (aMsg.priority() == IPC::Message::PRIORITY_URGENT)
        return false;

    
    if (!aMsg.is_sync()) {
        MOZ_ASSERT(aMsg.priority() == IPC::Message::PRIORITY_NORMAL);
        return true;
    }

    int msgPrio = aMsg.priority();
    int waitingPrio = AwaitingSyncReplyPriority();

    
    
    if (msgPrio < waitingPrio)
        return true;

    
    if (msgPrio > waitingPrio)
        return false;

    
    
    
    
    
    
    
    
    
    return mSide == ParentSide && aMsg.transaction_id() != mCurrentTransaction;
}

void
MessageChannel::OnMessageReceivedFromLink(const Message& aMsg)
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (MaybeInterceptSpecialIOMessage(aMsg))
        return;

    
    
    if (aMsg.is_sync() && aMsg.is_reply()) {
        if (aMsg.seqno() == mTimedOutMessageSeqno) {
            
            mTimedOutMessageSeqno = 0;
            return;
        }

        MOZ_ASSERT(AwaitingSyncReply());
        MOZ_ASSERT(!mRecvd);

        
        
        
        
        
        
        
        
        if (aMsg.is_reply_error()) {
            mRecvdErrors++;
            NotifyWorkerThread();
            return;
        }

        mRecvd = new Message(aMsg);
        NotifyWorkerThread();
        return;
    }

    
    MOZ_ASSERT(!aMsg.compress() || aMsg.priority() == IPC::Message::PRIORITY_NORMAL);

    bool compress = (aMsg.compress() && !mPending.empty() &&
                     mPending.back().type() == aMsg.type() &&
                     mPending.back().routing_id() == aMsg.routing_id());
    if (compress) {
        
        
        
        MOZ_ASSERT(mPending.back().compress());
        mPending.pop_back();
    }

    bool shouldWakeUp = AwaitingInterruptReply() ||
                        (AwaitingSyncReply() && !ShouldDeferMessage(aMsg)) ||
                        AwaitingIncomingMessage();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    mPending.push_back(aMsg);

    if (shouldWakeUp) {
        NotifyWorkerThread();
    } else {
        
        
        
        if (!compress) {
            
            
            mWorkerLoop->PostTask(FROM_HERE, new DequeueTask(mDequeueOneTask));
        }
    }
}

void
MessageChannel::ProcessPendingRequests()
{
    
    for (;;) {
        mozilla::Vector<Message> toProcess;

        for (MessageQueue::iterator it = mPending.begin(); it != mPending.end(); ) {
            Message &msg = *it;
            if (!ShouldDeferMessage(msg)) {
                toProcess.append(Move(msg));
                it = mPending.erase(it);
                continue;
            }
            it++;
        }

        if (toProcess.empty())
            break;

        
        
        for (auto it = toProcess.begin(); it != toProcess.end(); it++)
            ProcessPendingRequest(*it);
    }
}

bool
MessageChannel::Send(Message* aMsg, Message* aReply)
{
    
    MaybeScriptBlocker scriptBlocker(this, true);

    
    AssertWorkerThread();
    mMonitor->AssertNotCurrentThreadOwns();

    if (mCurrentTransaction == 0)
        mListener->OnBeginSyncTransaction();

#ifdef OS_WIN
    SyncStackFrame frame(this, false);
#endif

    CxxStackFrame f(*this, OUT_MESSAGE, aMsg);

    MonitorAutoLock lock(*mMonitor);

    if (mTimedOutMessageSeqno) {
        
        
        
        
        return false;
    }

    IPC_ASSERT(aMsg->is_sync(), "can only Send() sync messages here");
    IPC_ASSERT(aMsg->priority() >= DispatchingSyncMessagePriority(),
               "can't send sync message of a lesser priority than what's being dispatched");
    IPC_ASSERT(mAwaitingSyncReplyPriority <= aMsg->priority(),
               "nested sync message sends must be of increasing priority");

    IPC_ASSERT(DispatchingSyncMessagePriority() != IPC::Message::PRIORITY_URGENT,
               "not allowed to send messages while dispatching urgent messages");
    IPC_ASSERT(DispatchingAsyncMessagePriority() != IPC::Message::PRIORITY_URGENT,
               "not allowed to send messages while dispatching urgent messages");

    nsAutoPtr<Message> msg(aMsg);

    if (!Connected()) {
        ReportConnectionError("MessageChannel::SendAndWait");
        return false;
    }

    msg->set_seqno(NextSeqno());

    int32_t seqno = msg->seqno();
    DebugOnly<msgid_t> replyType = msg->type() + 1;

    AutoSetValue<bool> replies(mAwaitingSyncReply, true);
    AutoSetValue<int> prio(mAwaitingSyncReplyPriority, msg->priority());
    AutoEnterTransaction transact(this, seqno);

    int32_t transaction = mCurrentTransaction;
    msg->set_transaction_id(transaction);

    ProcessPendingRequests();

    mLink->SendMessage(msg.forget());

    while (true) {
        ProcessPendingRequests();

        
        if (mRecvdErrors) {
            mRecvdErrors--;
            return false;
        }

        if (mRecvd) {
            break;
        }

        MOZ_ASSERT(!mTimedOutMessageSeqno);

        bool maybeTimedOut = !WaitForSyncNotify();

        if (!Connected()) {
            ReportConnectionError("MessageChannel::SendAndWait");
            return false;
        }

        
        
        bool canTimeOut = transaction == seqno;
        if (maybeTimedOut && canTimeOut && !ShouldContinueFromTimeout()) {
            
            
            
            
            if (mRecvdErrors) {
                mRecvdErrors--;
                return false;
            }
            if (mRecvd) {
                break;
            }

            mTimedOutMessageSeqno = seqno;
            return false;
        }
    }

    MOZ_ASSERT(mRecvd);
    MOZ_ASSERT(mRecvd->is_reply(), "expected reply");
    MOZ_ASSERT(!mRecvd->is_reply_error());
    MOZ_ASSERT(mRecvd->type() == replyType, "wrong reply type");
    MOZ_ASSERT(mRecvd->seqno() == seqno);
    MOZ_ASSERT(mRecvd->is_sync());

    *aReply = Move(*mRecvd);
    mRecvd = nullptr;
    return true;
}

bool
MessageChannel::Call(Message* aMsg, Message* aReply)
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

    
    IPC_ASSERT(!AwaitingSyncReply(),
               "cannot issue Interrupt call while blocked on sync request");
    IPC_ASSERT(!DispatchingSyncMessage(),
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
            ReportConnectionError("MessageChannel::Call");
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

        if ((it = mOutOfTurnReplies.find(mInterruptStack.top().seqno()))
            != mOutOfTurnReplies.end())
        {
            recvd = Move(it->second);
            mOutOfTurnReplies.erase(it);
        } else if (!mPending.empty()) {
            recvd = Move(mPending.front());
            mPending.pop_front();
        } else {
            
            
            
            
            
            continue;
        }

        
        if (!recvd.is_interrupt()) {
            {
                AutoEnterTransaction transaction(this, recvd);
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
                    mOutOfTurnReplies[recvd.seqno()] = Move(recvd);
                    continue;
                }

                IPC_ASSERT(recvd.is_reply_error() ||
                           (recvd.type() == (outcall.type() + 1) &&
                            recvd.seqno() == outcall.seqno()),
                           "somebody's misbehavin'", true);
            }

            
            
            mInterruptStack.pop();

            bool is_reply_error = recvd.is_reply_error();
            if (!is_reply_error) {
                *aReply = Move(recvd);
            }

            
            
            IPC_ASSERT(!mInterruptStack.empty() || mOutOfTurnReplies.empty(),
                       "still have pending replies with no pending out-calls",
                       true);

            return !is_reply_error;
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
MessageChannel::WaitForIncomingMessage()
{
#ifdef OS_WIN
    SyncStackFrame frame(this, true);
#endif

    { 
        MonitorAutoLock lock(*mMonitor);
        AutoEnterWaitForIncoming waitingForIncoming(*this);
        if (mChannelState != ChannelConnected) {
            return false;
        }
        if (!HasPendingEvents()) {
            return WaitForInterruptNotify();
        }
    }

    return OnMaybeDequeueOne();
}

bool
MessageChannel::HasPendingEvents()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();
    return Connected() && !mPending.empty();
}

bool
MessageChannel::InterruptEventOccurred()
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();
    IPC_ASSERT(InterruptStackDepth() > 0, "not in wait loop");

    return (!Connected() ||
            !mPending.empty() ||
            (!mOutOfTurnReplies.empty() &&
             mOutOfTurnReplies.find(mInterruptStack.top().seqno()) !=
             mOutOfTurnReplies.end()));
}

bool
MessageChannel::ProcessPendingRequest(const Message &aUrgent)
{
    AssertWorkerThread();
    mMonitor->AssertCurrentThreadOwns();

    
    
    
    
    
    
    
    
    nsAutoPtr<Message> savedReply(mRecvd.forget());

    {
        
        
        AutoEnterTransaction transaction(this, aUrgent);

        MonitorAutoUnlock unlock(*mMonitor);
        DispatchMessage(aUrgent);
    }
    if (!Connected()) {
        ReportConnectionError("MessageChannel::ProcessPendingRequest");
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

    if (!mDeferred.empty())
        MaybeUndeferIncall();

    if (mPending.empty())
        return false;

    *recvd = Move(mPending.front());
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
        
        
        mOutOfTurnReplies[recvd.seqno()] = Move(recvd);
        return false;
    }

    {
        
        MOZ_ASSERT(mCurrentTransaction == 0);
        AutoEnterTransaction transaction(this, recvd);

        MonitorAutoUnlock unlock(*mMonitor);

        CxxStackFrame frame(*this, IN_MESSAGE, &recvd);
        DispatchMessage(recvd);
    }
    return true;
}

void
MessageChannel::DispatchMessage(const Message &aMsg)
{
    Maybe<AutoNoJSAPI> nojsapi;
    if (ScriptSettingsInitialized() && NS_IsMainThread())
        nojsapi.emplace();
    if (aMsg.is_sync())
        DispatchSyncMessage(aMsg);
    else if (aMsg.is_interrupt())
        DispatchInterruptMessage(aMsg, 0);
    else
        DispatchAsyncMessage(aMsg);
}

void
MessageChannel::DispatchSyncMessage(const Message& aMsg)
{
    AssertWorkerThread();

    nsAutoPtr<Message> reply;

    int prio = aMsg.priority();

    
    
    
    
    MOZ_ASSERT_IF(prio > IPC::Message::PRIORITY_NORMAL, NS_IsMainThread());
    MaybeScriptBlocker scriptBlocker(this, prio > IPC::Message::PRIORITY_NORMAL);

    IPC_ASSERT(prio >= mDispatchingSyncMessagePriority,
               "priority inversion while dispatching sync message");
    IPC_ASSERT(prio >= mAwaitingSyncReplyPriority,
               "dispatching a message of lower priority while waiting for a response");

    bool dummy;
    bool& blockingVar = ShouldBlockScripts() ? gParentIsBlocked : dummy;

    Result rv;
    if (mTimedOutMessageSeqno) {
        
        
        
        
        
        rv = MsgNotAllowed;
    } else {
        AutoSetValue<bool> blocked(blockingVar, true);
        AutoSetValue<bool> sync(mDispatchingSyncMessage, true);
        AutoSetValue<int> prioSet(mDispatchingSyncMessagePriority, prio);
        rv = mListener->OnMessageReceived(aMsg, *getter_Transfers(reply));
    }

    if (!MaybeHandleError(rv, aMsg, "DispatchSyncMessage")) {
        reply = new Message();
        reply->set_sync();
        reply->set_priority(aMsg.priority());
        reply->set_reply();
        reply->set_reply_error();
    }
    reply->set_seqno(aMsg.seqno());
    reply->set_transaction_id(aMsg.transaction_id());

    MonitorAutoLock lock(*mMonitor);
    if (ChannelConnected == mChannelState) {
        mLink->SendMessage(reply.forget());
    }
}

void
MessageChannel::DispatchAsyncMessage(const Message& aMsg)
{
    AssertWorkerThread();
    MOZ_ASSERT(!aMsg.is_interrupt() && !aMsg.is_sync());

    if (aMsg.routing_id() == MSG_ROUTING_NONE) {
        NS_RUNTIMEABORT("unhandled special message!");
    }

    Result rv;
    {
        int prio = aMsg.priority();
        AutoSetValue<bool> async(mDispatchingAsyncMessage, true);
        AutoSetValue<int> prioSet(mDispatchingAsyncMessagePriority, prio);
        rv = mListener->OnMessageReceived(aMsg);
    }
    MaybeHandleError(rv, aMsg, "DispatchAsyncMessage");
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

    nsAutoPtr<Message> reply;

    ++mRemoteStackDepthGuess;
    Result rv = mListener->OnCallReceived(aMsg, *getter_Transfers(reply));
    --mRemoteStackDepthGuess;

    if (!MaybeHandleError(rv, aMsg, "DispatchInterruptMessage")) {
        reply = new Message();
        reply->set_interrupt();
        reply->set_reply();
        reply->set_reply_error();
    }
    reply->set_seqno(aMsg.seqno());

    MonitorAutoLock lock(*mMonitor);
    if (ChannelConnected == mChannelState) {
        mLink->SendMessage(reply.forget());
    }
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

    MOZ_ASSERT(call.priority() == IPC::Message::PRIORITY_NORMAL);
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
    mListener->OnProcessingError(MsgRouteError, "MsgRouteError");
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
    mListener->OnProcessingError(MsgDropped, errorMsg);
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

    char reason[512];
    PR_snprintf(reason, sizeof(reason),
                "(msgtype=0x%lX,name=%s) %s",
                aMsg.type(), aMsg.name(), errorMsg);

    PrintErrorMessage(mSide, channelName, reason);

    mListener->OnProcessingError(code, reason);

    return false;
}

void
MessageChannel::OnChannelErrorFromLink()
{
    AssertLinkThread();
    mMonitor->AssertCurrentThreadOwns();

    if (InterruptStackDepth() > 0)
        NotifyWorkerThread();

    if (AwaitingSyncReply() || AwaitingIncomingMessage())
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
MessageChannel::CloseWithTimeout()
{
    AssertWorkerThread();

    MonitorAutoLock lock(*mMonitor);
    if (ChannelConnected != mChannelState) {
        return;
    }
    SynchronouslyClose();
    mChannelState = ChannelTimeout;
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

int32_t
MessageChannel::GetTopmostMessageRoutingId() const
{
    MOZ_ASSERT(MessageLoop::current() == mWorkerLoop);
    if (mCxxStackFrames.empty()) {
        return MSG_ROUTING_NONE;
    }
    const InterruptFrame& frame = mCxxStackFrames.back();
    return frame.GetRoutingId();
}

bool
ParentProcessIsBlocked()
{
    return gParentIsBlocked;
}

} 
} 
