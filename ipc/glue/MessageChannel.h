






#ifndef ipc_glue_MessageChannel_h
#define ipc_glue_MessageChannel_h 1

#include "base/basictypes.h"
#include "base/message_loop.h"

#include "mozilla/WeakPtr.h"
#include "mozilla/Monitor.h"
#include "mozilla/ipc/Transport.h"
#include "MessageLink.h"
#include "nsAutoPtr.h"
#include "mozilla/DebugOnly.h"

#include <deque>
#include <stack>
#include <vector>
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

    void FlushPendingRPCQueue();

    
    
    
    
    
    
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
        SyncStackFrame(MessageChannel* channel, bool rpc);
        ~SyncStackFrame();

        bool mRPC;
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
    void ProcessNativeEventsInRPCCall();
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
    bool MaybeHandleError(Result code, const char* channelName);

    void Clear();

    
    void DispatchOnChannelConnected(int32_t peer_pid);

    
    
    
    
    
    
    
    
    
    
    
    
    bool SendAndWait(Message* aMsg, Message* aReply);

    bool RPCCall(Message* aMsg, Message* aReply);
    bool UrgentCall(Message* aMsg, Message* aReply);

    bool RPCEventOccurred();

    void MaybeUndeferIncall();
    void EnqueuePendingMessages();

    
    bool OnMaybeDequeueOne();

    
    void DispatchMessage(const Message &aMsg);

    
    
    void DispatchSyncMessage(const Message &aMsg);
    void DispatchUrgentMessage(const Message &aMsg);
    void DispatchAsyncMessage(const Message &aMsg);
    void DispatchRPCMessage(const Message &aMsg, size_t aStackDepth);

    
    
    
    
    
    
    
    
    
    
    bool WaitForSyncNotify();
    bool WaitForRPCNotify();

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
    struct RPCFrame {
        RPCFrame(Direction direction, const Message* msg)
          : mDirection(direction), mMsg(msg)
        { }

        bool IsRPCIncall() const {
            return mMsg->is_rpc() && IN_MESSAGE == mDirection;
        }
        bool IsRPCOutcall() const {
            return mMsg->is_rpc() && OUT_MESSAGE == mDirection;
        }

        void Describe(int32_t* id, const char** dir, const char** sems,
                      const char** name) const
        {
            *id = mMsg->routing_id();
            *dir = (IN_MESSAGE == mDirection) ? "in" : "out";
            *sems = mMsg->is_rpc() ? "rpc" : mMsg->is_sync() ? "sync" : "async";
            *name = mMsg->name();
        }

        Direction mDirection;
        const Message* mMsg;
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

            mThat.mCxxStackFrames.push_back(RPCFrame(direction, msg));
            const RPCFrame& frame = mThat.mCxxStackFrames.back();

            if (frame.IsRPCIncall())
                mThat.EnteredCall();

            mThat.mSawRPCOutMsg |= frame.IsRPCOutcall();
        }

        ~CxxStackFrame() {
            bool exitingCall = mThat.mCxxStackFrames.back().IsRPCIncall();
            mThat.mCxxStackFrames.pop_back();
            bool exitingStack = mThat.mCxxStackFrames.empty();

            
            
            if (!mThat.mListener)
                return;

            mThat.AssertWorkerThread();
            if (exitingCall)
                mThat.ExitedCall();

            if (exitingStack)
                mThat.ExitedCxxStack();
        }
      private:
        MessageChannel& mThat;

        
        CxxStackFrame();
        CxxStackFrame(const CxxStackFrame&);
        CxxStackFrame& operator=(const CxxStackFrame&);
    };

    void DebugAbort(const char* file, int line, const char* cond,
                    const char* why,
                    bool reply=false) const;

    
    
    void DumpRPCStack(const char* const pfx="") const;

  private:
    
    size_t RPCStackDepth() const {
        mMonitor->AssertCurrentThreadOwns();
        return mRPCStack.size();
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
        return !mRPCStack.empty();
    }

    
    bool DispatchingSyncMessage() const {
        return mDispatchingSyncMessage;
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

    
    
    nsAutoPtr<Message> mRecvd;

    
    bool mDispatchingSyncMessage;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    MessageQueue mPending;
    nsAutoPtr<Message> mPendingUrgentRequest;

    
    
    
    
    std::stack<Message> mRPCStack;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    size_t mRemoteStackDepthGuess;

    
    
    
    
    
    
    
    
    std::vector<RPCFrame> mCxxStackFrames;

    
    
    bool mSawRPCOutMsg;

    
    
    
    MessageMap mOutOfTurnReplies;

    
    
    std::stack<Message> mDeferred;

#ifdef OS_WIN
    HANDLE mEvent;
#endif
};

} 
} 

#endif  
