





































#ifndef ipc_glue_RPCChannel_h
#define ipc_glue_RPCChannel_h 1

#include <stdio.h>


#include <queue>
#include <stack>
#include <vector>

#include "base/basictypes.h"

#include "pratom.h"

#include "mozilla/ipc/SyncChannel.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace ipc {


class RPCChannel : public SyncChannel
{
    friend class CxxStackFrame;

public:
    
    enum RacyRPCPolicy {
        RRPError,
        RRPChildWins,
        RRPParentWins
    };

    class  RPCListener :
        public SyncChannel::SyncListener
    {
    public:
        virtual ~RPCListener() { }

        virtual void OnChannelClose() = 0;
        virtual void OnChannelError() = 0;
        virtual Result OnMessageReceived(const Message& aMessage) = 0;
        virtual bool OnReplyTimeout() = 0;
        virtual Result OnMessageReceived(const Message& aMessage,
                                         Message*& aReply) = 0;
        virtual Result OnCallReceived(const Message& aMessage,
                                      Message*& aReply) = 0;

        virtual void OnEnteredCxxStack()
        {
            NS_RUNTIMEABORT("default impl shouldn't be invoked");
        }

        virtual void OnExitedCxxStack()
        {
            NS_RUNTIMEABORT("default impl shouldn't be invoked");
        }

        virtual RacyRPCPolicy MediateRPCRace(const Message& parent,
                                             const Message& child)
        {
            return RRPChildWins;
        }
    };

    RPCChannel(RPCListener* aListener);

    virtual ~RPCChannel();

    NS_OVERRIDE
    void Clear();

    
    bool Call(Message* msg, Message* reply);

    
    
    NS_OVERRIDE
    virtual bool Send(Message* msg);
    NS_OVERRIDE
    virtual bool Send(Message* msg, Message* reply);

    
    
    
    
    
    
    
    
    
    
    
    
    bool BlockChild();

    
    
    
    
    
    
    
    bool UnblockChild();

    
    bool IsOnCxxStack() const {
        return !mCxxStackFrames.empty();
    }

    NS_OVERRIDE
    virtual bool OnSpecialMessage(uint16 id, const Message& msg);

    
    
    NS_OVERRIDE
    virtual void OnMessageReceived(const Message& msg);
    NS_OVERRIDE
    virtual void OnChannelError();

#ifdef OS_WIN
    static bool IsSpinLoopActive() {
        return (sModalEventCount > 0);
    }
protected:
    bool WaitForNotify();
    void SpinInternalEventLoop();
    static bool WaitNeedsSpinLoop() {
        return (IsSpinLoopActive() && 
                (sModalEventCount > sInnerEventLoopDepth));
    }
    static void EnterSpinLoop() {
        sInnerEventLoopDepth++;
    }
    static void ExitSpinLoop() {
        sInnerEventLoopDepth--;
        NS_ASSERTION(sInnerEventLoopDepth >= 0,
            "sInnerEventLoopDepth dropped below zero!");
    }
    static void IncModalLoopCnt() {
        sModalEventCount++;
    }
    static void DecModalLoopCnt() {
        sModalEventCount--;
        NS_ASSERTION(sModalEventCount >= 0,
            "sModalEventCount dropped below zero!");
    }

    static int sInnerEventLoopDepth;
    static int sModalEventCount;
#endif

  private:
    

    RPCListener* Listener() const {
        return static_cast<RPCListener*>(mListener);
    }

    NS_OVERRIDE
    virtual bool ShouldDeferNotifyMaybeError() {
        return IsOnCxxStack();
    }

    bool EventOccurred();

    void MaybeProcessDeferredIncall();
    void EnqueuePendingMessages();

    void OnMaybeDequeueOne();
    void Incall(const Message& call, size_t stackDepth);
    void DispatchIncall(const Message& call);

    void BlockOnParent();
    void UnblockFromParent();

    
    
    
    
    void EnteredCxxStack()
    {
        Listener()->OnEnteredCxxStack();
    }

    void ExitedCxxStack();

    enum Direction { IN_MESSAGE, OUT_MESSAGE };
    struct RPCFrame {
        RPCFrame(Direction direction, const Message* msg) :
            mDirection(direction), mMsg(msg)
        { }

        void Describe(int32* id, const char** dir, const char** sems,
                      const char** name)
            const
        {
            *id = mMsg->routing_id();
            *dir = (IN_MESSAGE == mDirection) ? "in" : "out";
            *sems = mMsg->is_rpc() ? "rpc" : mMsg->is_sync() ? "sync" : "async";
            *name = mMsg->name();
        }

        Direction mDirection;
        const Message* mMsg;
    };

    class NS_STACK_CLASS CxxStackFrame
    {
    public:

        CxxStackFrame(RPCChannel& that, Direction direction,
                      const Message* msg) : mThat(that) {
            mThat.AssertWorkerThread();

            if (mThat.mCxxStackFrames.empty())
                mThat.EnteredCxxStack();

            mThat.mCxxStackFrames.push_back(RPCFrame(direction, msg));
            mThat.mSawRPCOutMsg |= (direction == OUT_MESSAGE) &&
                                   (msg->is_rpc());
        }

        ~CxxStackFrame() {
            mThat.mCxxStackFrames.pop_back();
            bool exitingStack = mThat.mCxxStackFrames.empty();

            
            
            if (!mThat.mListener)
                return;

            mThat.AssertWorkerThread();
            if (exitingStack)
                mThat.ExitedCxxStack();
        }
    private:
        RPCChannel& mThat;

        
        CxxStackFrame();
        CxxStackFrame(const CxxStackFrame&);
        CxxStackFrame& operator=(const CxxStackFrame&);
    };

    
    size_t StackDepth() {
        mMutex.AssertCurrentThreadOwns();
        return mStack.size();
    }

    void DebugAbort(const char* file, int line, const char* cond,
                    const char* why,
                    const char* type="rpc", bool reply=false);

    
    
    void DumpRPCStack(FILE* outfile=NULL, const char* const pfx="");

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    std::queue<Message> mPending;

    
    
    
    
    std::stack<Message> mStack;

    
    
    
    
    
    typedef std::map<size_t, Message> MessageMap;
    MessageMap mOutOfTurnReplies;

    
    
    
    
    std::stack<Message> mDeferred;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    size_t mRemoteStackDepthGuess;

    
    bool mBlockedOnParent;

    
    
    
    
    
    
    
    
    std::vector<RPCFrame> mCxxStackFrames;

    
    
    
    bool mSawRPCOutMsg;

private:

    
    
    
    
    class RefCountedTask
    {
      public:
        RefCountedTask(CancelableTask* aTask)
        : mTask(aTask)
        , mRefCnt(0) {}
        ~RefCountedTask() { delete mTask; }
        void Run() { mTask->Run(); }
        void Cancel() { mTask->Cancel(); }
        void AddRef() {
            PR_AtomicIncrement(reinterpret_cast<PRInt32*>(&mRefCnt));
        }
        void Release() {
            nsrefcnt count =
                PR_AtomicDecrement(reinterpret_cast<PRInt32*>(&mRefCnt));
            if (0 == count)
                delete this;
        }

      private:
        CancelableTask* mTask;
        nsrefcnt mRefCnt;
    };

    
    
    
    
    class DequeueTask : public Task
    {
      public:
        DequeueTask(RefCountedTask* aTask) : mTask(aTask) {}
        void Run() { mTask->Run(); }
        
      private:
        nsRefPtr<RefCountedTask> mTask;
    };

    
    nsRefPtr<RefCountedTask> mDequeueOneTask;
};


} 
} 
#endif  
