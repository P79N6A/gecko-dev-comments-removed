





































#ifndef ipc_glue_RPCChannel_h
#define ipc_glue_RPCChannel_h 1

#include <stdio.h>


#include <queue>
#include <stack>
#include <vector>

#include "base/basictypes.h"

#include "nsAtomicRefcnt.h"

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
        virtual void OnProcessingError(Result aError) = 0;
        virtual bool OnReplyTimeout() = 0;
        virtual Result OnMessageReceived(const Message& aMessage,
                                         Message*& aReply) = 0;
        virtual Result OnCallReceived(const Message& aMessage,
                                      Message*& aReply) = 0;
        virtual void OnChannelConnected(int32 peer_pid) {};

        virtual void OnEnteredCxxStack()
        {
            NS_RUNTIMEABORT("default impl shouldn't be invoked");
        }

        virtual void OnExitedCxxStack()
        {
            NS_RUNTIMEABORT("default impl shouldn't be invoked");
        }

        virtual void OnEnteredCall()
        {
            NS_RUNTIMEABORT("default impl shouldn't be invoked");
        }

        virtual void OnExitedCall()
        {
            NS_RUNTIMEABORT("default impl shouldn't be invoked");
        }

        virtual RacyRPCPolicy MediateRPCRace(const Message& parent,
                                             const Message& child)
        {
            return RRPChildWins;
        }
        virtual void ProcessRemoteNativeEventsInRPCCall() {};
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

    





    void FlushPendingRPCQueue();

#ifdef OS_WIN
    void ProcessNativeEventsInRPCCall();
    static void NotifyGeckoEventDispatch();

protected:
    bool WaitForNotify();
    void SpinInternalEventLoop();
#endif

  private:
    

    RPCListener* Listener() const {
        return static_cast<RPCListener*>(mListener);
    }

    NS_OVERRIDE
    virtual bool ShouldDeferNotifyMaybeError() const {
        return IsOnCxxStack();
    }

    bool EventOccurred() const;

    void MaybeUndeferIncall();
    void EnqueuePendingMessages();

    



    bool OnMaybeDequeueOne();

    









    size_t RemoteViewOfStackDepth(size_t stackDepth) const;

    void Incall(const Message& call, size_t stackDepth);
    void DispatchIncall(const Message& call);

    void BlockOnParent();
    void UnblockFromParent();

    
    
    
    
    void EnteredCxxStack()
    {
        Listener()->OnEnteredCxxStack();
    }

    void ExitedCxxStack();

    void EnteredCall()
    {
        Listener()->OnEnteredCall();
    }

    void ExitedCall()
    {
        Listener()->OnExitedCall();
    }

    enum Direction { IN_MESSAGE, OUT_MESSAGE };
    struct RPCFrame {
        RPCFrame(Direction direction, const Message* msg) :
            mDirection(direction), mMsg(msg)
        { }

        bool IsRPCIncall() const
        {
            return mMsg->is_rpc() && IN_MESSAGE == mDirection;
        }

        bool IsRPCOutcall() const
        {
            return mMsg->is_rpc() && OUT_MESSAGE == mDirection;
        }

        void Describe(int32* id, const char** dir, const char** sems,
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

    class NS_STACK_CLASS CxxStackFrame
    {
    public:

        CxxStackFrame(RPCChannel& that, Direction direction,
                      const Message* msg) : mThat(that) {
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
        RPCChannel& mThat;

        
        CxxStackFrame();
        CxxStackFrame(const CxxStackFrame&);
        CxxStackFrame& operator=(const CxxStackFrame&);
    };

    
    size_t StackDepth() const {
        mMonitor.AssertCurrentThreadOwns();
        return mStack.size();
    }

    void DebugAbort(const char* file, int line, const char* cond,
                    const char* why,
                    const char* type="rpc", bool reply=false) const;

    
    
    void DumpRPCStack(FILE* outfile=NULL, const char* const pfx="") const;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    typedef std::queue<Message> MessageQueue;
    MessageQueue mPending;

    
    
    
    
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
            NS_AtomicIncrementRefcnt(mRefCnt);
        }
        void Release() {
            if (NS_AtomicDecrementRefcnt(mRefCnt) == 0)
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
