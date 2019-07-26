






#ifndef ipc_glue_MessageChannel_h
#define ipc_glue_MessageChannel_h 1

#include "base/basictypes.h"
#include "base/message_loop.h"

#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Monitor.h"
#include "mozilla/Move.h"
#include "mozilla/Vector.h"
#include "mozilla/WeakPtr.h"
#include "mozilla/ipc/Transport.h"
#include "MessageLink.h"
#include "nsAutoPtr.h"

#include <deque>
#include <stack>
#include <math.h>

namespace mozilla {
namespace ipc {

class MessageChannel;

class RefCountedMonitor : public Monitor
{
  public:
    RefCountedMonitor()
        : Monitor("mozilla.ipc.MessageChannel.mMonitor")
    {}

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RefCountedMonitor)
};

class MessageChannel : HasResultCodes
{
    friend class ProcessLink;
    friend class ThreadLink;
    friend class AutoEnterRPCTransaction;

    typedef mozilla::Monitor Monitor;

  public:
    static const int32_t kNoTimeout;

    typedef IPC::Message Message;
    typedef mozilla::ipc::Transport Transport;

    MessageChannel(MessageListener *aListener);
    ~MessageChannel();

    
    
    
    
    
    bool Open(Transport* aTransport, MessageLoop* aIOLoop=0, Side aSide=UnknownSide);

    
    
    
    
    
    
    
    
    bool Open(MessageChannel *aTargetChan, MessageLoop *aTargetLoop, Side aSide);

    
    void Close();

    
    
    void CloseWithError();

    
    bool Send(Message* aMsg);

    
    
    bool Echo(Message* aMsg);

    
    bool Send(Message* aMsg, Message* aReply);

    
    bool Call(Message* aMsg, Message* aReply);

    void SetReplyTimeoutMs(int32_t aTimeoutMs);

    bool IsOnCxxStack() const {
        return !mCxxStackFrames.empty();
    }

    void FlushPendingInterruptQueue();

    
    
    
    
    
    
    bool Unsound_IsClosed() const {
        return mLink ? mLink->Unsound_IsClosed() : true;
    }
    uint32_t Unsound_NumQueuedMessages() const {
        return mLink ? mLink->Unsound_NumQueuedMessages() : 0;
    }

    static bool IsPumpingMessages() {
        return sIsPumpingMessages;
    }
    static void SetIsPumpingMessages(bool aIsPumping) {
        sIsPumpingMessages = aIsPumping;
    }

#ifdef OS_WIN
    struct MOZ_STACK_CLASS SyncStackFrame
    {
        SyncStackFrame(MessageChannel* channel, bool interrupt);
        ~SyncStackFrame();

        bool mInterrupt;
        bool mSpinNestedEvents;
        bool mListenerNotified;
        MessageChannel* mChannel;

        
        SyncStackFrame* mPrev;

        
        SyncStackFrame* mStaticPrev;
    };
    friend struct MessageChannel::SyncStackFrame;

    static bool IsSpinLoopActive() {
        for (SyncStackFrame* frame = sStaticTopFrame; frame; frame = frame->mPrev) {
            if (frame->mSpinNestedEvents)
                return true;
        }
        return false;
    }

  protected:
    
    SyncStackFrame* mTopFrame;

    
    static SyncStackFrame* sStaticTopFrame;

  public:
    void ProcessNativeEventsInInterruptCall();
    static void NotifyGeckoEventDispatch();

  private:
    void SpinInternalEventLoop();
#endif

  private:
    void CommonThreadOpenInit(MessageChannel *aTargetChan, Side aSide);
    void OnOpenAsSlave(MessageChannel *aTargetChan, Side aSide);

    void PostErrorNotifyTask();
    void OnNotifyMaybeChannelError();
    void ReportConnectionError(const char* aChannelName) const;
    void ReportMessageRouteError(const char* channelName) const;
    bool MaybeHandleError(Result code, const char* channelName);

    void Clear();

    
    void DispatchOnChannelConnected(int32_t peer_pid);

    
    
    
    
    
    
    
    
    
    
    
    
    bool SendAndWait(Message* aMsg, Message* aReply);

    bool RPCCall(Message* aMsg, Message* aReply);
    bool InterruptCall(Message* aMsg, Message* aReply);
    bool UrgentCall(Message* aMsg, Message* aReply);

    bool InterruptEventOccurred();

    bool ProcessPendingUrgentRequest();
    bool ProcessPendingRPCCall();

    void MaybeUndeferIncall();
    void EnqueuePendingMessages();

    
    bool OnMaybeDequeueOne();
    bool DequeueOne(Message *recvd);

    
    void DispatchMessage(const Message &aMsg);

    
    
    void DispatchSyncMessage(const Message &aMsg);
    void DispatchUrgentMessage(const Message &aMsg);
    void DispatchAsyncMessage(const Message &aMsg);
    void DispatchRPCMessage(const Message &aMsg);
    void DispatchInterruptMessage(const Message &aMsg, size_t aStackDepth);

    
    
    
    
    
    
    
    
    
    
    bool WaitForSyncNotify();
    bool WaitForInterruptNotify();

    bool WaitResponse(bool aWaitTimedOut);

    bool ShouldContinueFromTimeout();

    
    
    
    
    
    
    
    
    size_t RemoteViewOfStackDepth(size_t stackDepth) const {
        AssertWorkerThread();
        return stackDepth - mOutOfTurnReplies.size();
    }

    int32_t NextSeqno() {
        AssertWorkerThread();
        return (mSide == ChildSide) ? --mNextSeqno : ++mNextSeqno;
    }

    
    
    
    void EnteredCxxStack() {
       mListener->OnEnteredCxxStack();
    }

    void ExitedCxxStack();

    void EnteredCall() {
        mListener->OnEnteredCall();
    }

    void ExitedCall() {
        mListener->OnExitedCall();
    }

    MessageListener *Listener() const {
        return mListener.get();
    }

    enum Direction { IN_MESSAGE, OUT_MESSAGE };
    struct InterruptFrame {
        enum Semantics { INTR_SEMS, SYNC_SEMS, ASYNC_SEMS };

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

        const char* mMessageName;
        int32_t mMessageRoutingId;
        Semantics mMesageSemantics;
        Direction mDirection;
        DebugOnly<bool> mMoved;

    private:
        InterruptFrame(const InterruptFrame& aOther) MOZ_DELETE;
        InterruptFrame& operator=(const InterruptFrame&) MOZ_DELETE;
    };

    class MOZ_STACK_CLASS CxxStackFrame
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

    void DebugAbort(const char* file, int line, const char* cond,
                    const char* why,
                    bool reply=false) const;

    
    
    void DumpInterruptStack(const char* const pfx="") const;

  private:
    
    size_t InterruptStackDepth() const {
        mMonitor->AssertCurrentThreadOwns();
        return mInterruptStack.size();
    }

    
    bool AwaitingSyncReply() const {
        mMonitor->AssertCurrentThreadOwns();
        return mPendingSyncReplies > 0;
    }
    bool AwaitingUrgentReply() const {
        mMonitor->AssertCurrentThreadOwns();
        return mPendingUrgentReplies > 0;
    }
    bool AwaitingRPCReply() const {
        mMonitor->AssertCurrentThreadOwns();
        return mPendingRPCReplies > 0;
    }
    bool AwaitingInterruptReply() const {
        mMonitor->AssertCurrentThreadOwns();
        return !mInterruptStack.empty();
    }

    
    bool DispatchingSyncMessage() const {
        return mDispatchingSyncMessage;
    }

    
    bool DispatchingUrgentMessage() const {
        return mDispatchingUrgentMessageCount > 0;
    }

    bool Connected() const;

  private:
    
    void NotifyWorkerThread();

    
    
    bool MaybeInterceptSpecialIOMessage(const Message& aMsg);

    void OnChannelConnected(int32_t peer_id);

    
    void SynchronouslyClose();

    void OnMessageReceivedFromLink(const Message& aMsg);
    void OnChannelErrorFromLink();

  private:
    
    void NotifyChannelClosed();
    void NotifyMaybeChannelError();

  private:
    
    void AssertWorkerThread() const
    {
        NS_ABORT_IF_FALSE(mWorkerLoopID == MessageLoop::current()->id(),
                          "not on worker thread!");
    }

    
    
    
    void AssertLinkThread() const
    {
        NS_ABORT_IF_FALSE(mWorkerLoopID != MessageLoop::current()->id(),
                          "on worker thread but should not be!");
    }

  private:
    typedef IPC::Message::msgid_t msgid_t;
    typedef std::deque<Message> MessageQueue;
    typedef std::map<size_t, Message> MessageMap;

    
    
    class RefCountedTask
    {
      public:
        RefCountedTask(CancelableTask* aTask)
          : mTask(aTask)
        { }
        ~RefCountedTask() { delete mTask; }
        void Run() { mTask->Run(); }
        void Cancel() { mTask->Cancel(); }

        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RefCountedTask)

      private:
        CancelableTask* mTask;
    };

    
    
    class DequeueTask : public Task
    {
      public:
        DequeueTask(RefCountedTask* aTask)
          : mTask(aTask)
        { }
        void Run() { mTask->Run(); }

      private:
        nsRefPtr<RefCountedTask> mTask;
    };

  private:
    mozilla::WeakPtr<MessageListener> mListener;
    ChannelState mChannelState;
    nsRefPtr<RefCountedMonitor> mMonitor;
    Side mSide;
    MessageLink* mLink;
    MessageLoop* mWorkerLoop;           
    CancelableTask* mChannelErrorTask;  

    
    
    int mWorkerLoopID;

    
    nsRefPtr<RefCountedTask> mDequeueOneTask;

    
    
    
    
    int32_t mTimeoutMs;
    bool mInTimeoutSecondHalf;

    
    
    int32_t mNextSeqno;

    static bool sIsPumpingMessages;

    class AutoEnterPendingReply {
      public:
        AutoEnterPendingReply(size_t &replyVar)
          : mReplyVar(replyVar)
        {
            mReplyVar++;
        }
        ~AutoEnterPendingReply() {
            mReplyVar--;
        }
      private:
        size_t& mReplyVar;
    };

    
    
    size_t mPendingSyncReplies;

    
    
    
    size_t mPendingUrgentReplies;
    size_t mPendingRPCReplies;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    int32_t mCurrentRPCTransaction;

    class AutoEnterRPCTransaction
    {
      public:
       AutoEnterRPCTransaction(MessageChannel *aChan)
        : mChan(aChan),
          mOldTransaction(mChan->mCurrentRPCTransaction)
       {
           mChan->mMonitor->AssertCurrentThreadOwns();
           if (mChan->mCurrentRPCTransaction == 0)
               mChan->mCurrentRPCTransaction = mChan->NextSeqno();
       }
       AutoEnterRPCTransaction(MessageChannel *aChan, Message *message)
        : mChan(aChan),
          mOldTransaction(mChan->mCurrentRPCTransaction)
       {
           mChan->mMonitor->AssertCurrentThreadOwns();

           if (!message->is_rpc() && !message->is_urgent())
               return;

           MOZ_ASSERT_IF(mChan->mSide == ParentSide,
                         !mOldTransaction || mOldTransaction == message->transaction_id());
           mChan->mCurrentRPCTransaction = message->transaction_id();
       }
       ~AutoEnterRPCTransaction() {
           mChan->mMonitor->AssertCurrentThreadOwns();
           mChan->mCurrentRPCTransaction = mOldTransaction;
       }

      private:
       MessageChannel *mChan;
       int32_t mOldTransaction;
    };

    
    
    nsAutoPtr<Message> mRecvd;

    
    bool mDispatchingSyncMessage;

    
    size_t mDispatchingUrgentMessageCount;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    MessageQueue mPending;

    
    
    
    
    
    
    
    
    
    nsAutoPtr<Message> mPendingUrgentRequest;
    nsAutoPtr<Message> mPendingRPCCall;

    
    
    
    
    std::stack<Message> mInterruptStack;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    size_t mRemoteStackDepthGuess;

    
    
    
    
    
    
    
    
    mozilla::Vector<InterruptFrame> mCxxStackFrames;

    
    
    bool mSawInterruptOutMsg;

    
    
    
    MessageMap mOutOfTurnReplies;

    
    
    std::stack<Message> mDeferred;

#ifdef OS_WIN
    HANDLE mEvent;
#endif
};

} 
} 

#endif  
