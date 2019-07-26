





#ifndef ipc_glue_RPCChannel_h
#define ipc_glue_RPCChannel_h 1

#include <stdio.h>

#include <deque>
#include <stack>
#include <vector>

#include "base/basictypes.h"

#include "nsISupportsImpl.h"

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
        virtual int32_t GetProtocolTypeId() = 0;
        virtual bool OnReplyTimeout() = 0;
        virtual Result OnMessageReceived(const Message& aMessage,
                                         Message*& aReply) = 0;
        virtual Result OnCallReceived(const Message& aMessage,
                                      Message*& aReply) = 0;
        virtual void OnChannelConnected(int32_t peer_pid) {}

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

    void Clear() MOZ_OVERRIDE;

    
    bool Call(Message* msg, Message* reply);

    
    
    virtual bool Send(Message* msg) MOZ_OVERRIDE;
    virtual bool Send(Message* msg, Message* reply) MOZ_OVERRIDE;

    
    bool IsOnCxxStack() const {
        return !mCxxStackFrames.empty();
    }

    





    void FlushPendingRPCQueue();

#ifdef OS_WIN
    void ProcessNativeEventsInRPCCall();
    static void NotifyGeckoEventDispatch();

protected:
    bool WaitForNotify();
    void SpinInternalEventLoop();
#endif

protected:
    virtual void OnMessageReceivedFromLink(const Message& msg) MOZ_OVERRIDE;
    virtual void OnChannelErrorFromLink() MOZ_OVERRIDE;

private:
    

    RPCListener* Listener() const {
        return static_cast<RPCListener*>(mListener.get());
    }

    virtual bool ShouldDeferNotifyMaybeError() const MOZ_OVERRIDE {
        return IsOnCxxStack();
    }

    bool EventOccurred() const;

    void MaybeUndeferIncall();
    void EnqueuePendingMessages();

    



    bool OnMaybeDequeueOne();

    









    size_t RemoteViewOfStackDepth(size_t stackDepth) const;

    void Incall(const Message& call, size_t stackDepth);
    void DispatchIncall(const Message& call);

    
    
    
    
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
        mMonitor->AssertCurrentThreadOwns();
        return mStack.size();
    }

    void DebugAbort(const char* file, int line, const char* cond,
                    const char* why,
                    const char* type="rpc", bool reply=false) const;

    
    
    void DumpRPCStack(const char* const pfx="") const;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    typedef std::deque<Message> MessageQueue;
    MessageQueue mPending;

    
    
    
    MessageQueue mNonUrgentDeferred;

    
    
    
    
    std::stack<Message> mStack;

    
    
    
    
    
    typedef std::map<size_t, Message> MessageMap;
    MessageMap mOutOfTurnReplies;

    
    
    
    
    std::stack<Message> mDeferred;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    size_t mRemoteStackDepthGuess;

    
    
    
    
    
    
    
    
    std::vector<RPCFrame> mCxxStackFrames;

    
    
    
    bool mSawRPCOutMsg;

private:

    
    
    
    
    class RefCountedTask
    {
      public:
        RefCountedTask(CancelableTask* aTask)
        : mTask(aTask) {}
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
