






#ifndef ipc_glue_MessageChannel_h
#define ipc_glue_MessageChannel_h 1

#include "base/basictypes.h"
#include "base/message_loop.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/Monitor.h"
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

  private:
    ~RefCountedMonitor() {}
};

class MessageChannel : HasResultCodes
{
    friend class ProcessLink;
    friend class ThreadLink;

    class CxxStackFrame;
    class InterruptFrame;

    typedef mozilla::Monitor Monitor;

  public:
    static const int32_t kNoTimeout;

    typedef IPC::Message Message;
    typedef mozilla::ipc::Transport Transport;

    explicit MessageChannel(MessageListener *aListener);
    ~MessageChannel();

    
    
    
    
    
    bool Open(Transport* aTransport, MessageLoop* aIOLoop=0, Side aSide=UnknownSide);

    
    
    
    
    
    
    
    
    bool Open(MessageChannel *aTargetChan, MessageLoop *aTargetLoop, Side aSide);

    
    void Close();

    
    
    void CloseWithError();

    void CloseWithTimeout();

    void SetAbortOnError(bool abort)
    {
        mAbortOnError = abort;
    }

    
    enum ChannelFlags {
      REQUIRE_DEFAULT                         = 0,
      
      
      
      
      
      REQUIRE_DEFERRED_MESSAGE_PROTECTION     = 1 << 0
    };
    void SetChannelFlags(ChannelFlags aFlags) { mFlags = aFlags; }
    ChannelFlags GetChannelFlags() { return mFlags; }

    void BlockScripts();

    bool ShouldBlockScripts() const
    {
        return mBlockScripts;
    }

    
    bool Send(Message* aMsg);

    
    
    bool Echo(Message* aMsg);

    
    bool Send(Message* aMsg, Message* aReply);

    
    bool Call(Message* aMsg, Message* aReply);

    
    bool WaitForIncomingMessage();

    bool CanSend() const;

    void SetReplyTimeoutMs(int32_t aTimeoutMs);

    bool IsOnCxxStack() const {
        return !mCxxStackFrames.empty();
    }

    





    int32_t GetTopmostMessageRoutingId() const;

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

    bool mIsSyncWaitingOnNonMainThread;

    
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
    bool MaybeHandleError(Result code, const Message& aMsg, const char* channelName);

    void Clear();

    
    void DispatchOnChannelConnected();

    bool InterruptEventOccurred();
    bool HasPendingEvents();

    void ProcessPendingRequests();
    bool ProcessPendingRequest(const Message &aUrgent);

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

    void EnteredSyncSend() {
        mListener->OnEnteredSyncSend();
    }

    void ExitedSyncSend() {
        mListener->OnExitedSyncSend();
    }

    MessageListener *Listener() const {
        return mListener.get();
    }

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
        return mAwaitingSyncReply;
    }
    int AwaitingSyncReplyPriority() const {
        mMonitor->AssertCurrentThreadOwns();
        return mAwaitingSyncReplyPriority;
    }
    bool AwaitingInterruptReply() const {
        mMonitor->AssertCurrentThreadOwns();
        return !mInterruptStack.empty();
    }
    bool AwaitingIncomingMessage() const {
        mMonitor->AssertCurrentThreadOwns();
        return mIsWaitingForIncoming;
    }

    class MOZ_STACK_CLASS AutoEnterWaitForIncoming
    {
    public:
        explicit AutoEnterWaitForIncoming(MessageChannel& aChannel)
            : mChannel(aChannel)
        {
            aChannel.mMonitor->AssertCurrentThreadOwns();
            aChannel.mIsWaitingForIncoming = true;
        }

        ~AutoEnterWaitForIncoming()
        {
            mChannel.mIsWaitingForIncoming = false;
        }

    private:
        MessageChannel& mChannel;
    };
    friend class AutoEnterWaitForIncoming;

    
    bool DispatchingSyncMessage() const {
        AssertWorkerThread();
        return mDispatchingSyncMessage;
    }

    int DispatchingSyncMessagePriority() const {
        AssertWorkerThread();
        return mDispatchingSyncMessagePriority;
    }

    bool DispatchingAsyncMessage() const {
        AssertWorkerThread();
        return mDispatchingAsyncMessage;
    }

    int DispatchingAsyncMessagePriority() const {
        AssertWorkerThread();
        return mDispatchingAsyncMessagePriority;
    }

    bool Connected() const;

  private:
    
    void NotifyWorkerThread();

    
    
    bool MaybeInterceptSpecialIOMessage(const Message& aMsg);

    void OnChannelConnected(int32_t peer_id);

    
    void SynchronouslyClose();

    bool ShouldDeferMessage(const Message& aMsg);
    void OnMessageReceivedFromLink(const Message& aMsg);
    void OnChannelErrorFromLink();

  private:
    
    void NotifyChannelClosed();
    void NotifyMaybeChannelError();

  private:
    
    void AssertWorkerThread() const
    {
        MOZ_RELEASE_ASSERT(mWorkerLoopID == MessageLoop::current()->id(),
                           "not on worker thread!");
    }

    
    
    
    void AssertLinkThread() const
    {
        MOZ_RELEASE_ASSERT(mWorkerLoopID != MessageLoop::current()->id(),
                           "on worker thread but should not be!");
    }

  private:
    typedef IPC::Message::msgid_t msgid_t;
    typedef std::deque<Message> MessageQueue;
    typedef std::map<size_t, Message> MessageMap;

    
    
    class RefCountedTask
    {
      public:
        explicit RefCountedTask(CancelableTask* aTask)
          : mTask(aTask)
        { }
      private:
        ~RefCountedTask() { delete mTask; }
      public:
        void Run() { mTask->Run(); }
        void Cancel() { mTask->Cancel(); }

        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RefCountedTask)

      private:
        CancelableTask* mTask;
    };

    
    
    class DequeueTask : public Task
    {
      public:
        explicit DequeueTask(RefCountedTask* aTask)
          : mTask(aTask)
        { }
        void Run() override { mTask->Run(); }

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

    template<class T>
    class AutoSetValue {
      public:
        explicit AutoSetValue(T &var, const T &newValue)
          : mVar(var), mPrev(var)
        {
            mVar = newValue;
        }
        ~AutoSetValue() {
            mVar = mPrev;
        }
      private:
        T& mVar;
        T mPrev;
    };

    
    bool mAwaitingSyncReply;
    int mAwaitingSyncReplyPriority;

    
    
    bool mDispatchingSyncMessage;
    int mDispatchingSyncMessagePriority;

    bool mDispatchingAsyncMessage;
    int mDispatchingAsyncMessagePriority;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    int32_t mCurrentTransaction;

    class AutoEnterTransaction
    {
      public:
       explicit AutoEnterTransaction(MessageChannel *aChan, int32_t aMsgSeqno)
        : mChan(aChan),
          mOldTransaction(mChan->mCurrentTransaction)
       {
           mChan->mMonitor->AssertCurrentThreadOwns();
           if (mChan->mCurrentTransaction == 0)
               mChan->mCurrentTransaction = aMsgSeqno;
       }
       explicit AutoEnterTransaction(MessageChannel *aChan, const Message &aMessage)
        : mChan(aChan),
          mOldTransaction(mChan->mCurrentTransaction)
       {
           mChan->mMonitor->AssertCurrentThreadOwns();

           if (!aMessage.is_sync())
               return;

           MOZ_ASSERT_IF(mChan->mSide == ParentSide && mOldTransaction != aMessage.transaction_id(),
                         !mOldTransaction || aMessage.priority() > mChan->AwaitingSyncReplyPriority());
           mChan->mCurrentTransaction = aMessage.transaction_id();
       }
       ~AutoEnterTransaction() {
           mChan->mMonitor->AssertCurrentThreadOwns();
           mChan->mCurrentTransaction = mOldTransaction;
       }

      private:
       MessageChannel *mChan;
       int32_t mOldTransaction;
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t mTimedOutMessageSeqno;
    int mTimedOutMessagePriority;

    
    
    nsAutoPtr<Message> mRecvd;

    
    
    size_t mRecvdErrors;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    MessageQueue mPending;

    
    
    
    
    std::stack<Message> mInterruptStack;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    size_t mRemoteStackDepthGuess;

    
    
    
    
    
    
    
    
    mozilla::Vector<InterruptFrame> mCxxStackFrames;

    
    
    bool mSawInterruptOutMsg;

    
    
    
    bool mIsWaitingForIncoming;

    
    
    
    MessageMap mOutOfTurnReplies;

    
    
    std::stack<Message> mDeferred;

#ifdef OS_WIN
    HANDLE mEvent;
#endif

    
    
    bool mAbortOnError;

    
    bool mBlockScripts;

    
    ChannelFlags mFlags;

    
    
    
    nsRefPtr<RefCountedTask> mOnChannelConnectedTask;
    DebugOnly<bool> mPeerPidSet;
    int32_t mPeerPid;
};

bool
ParentProcessIsBlocked();

} 
} 

#endif  
