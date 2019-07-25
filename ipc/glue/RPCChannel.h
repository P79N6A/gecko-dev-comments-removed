





































#ifndef ipc_glue_RPCChannel_h
#define ipc_glue_RPCChannel_h 1


#include <queue>
#include <stack>

#include "mozilla/ipc/SyncChannel.h"

namespace mozilla {
namespace ipc {


class RPCChannel : public SyncChannel
{
    friend class CxxStackFrame;

public:
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
    };

    
    enum RacyRPCPolicy {
        RRPError,
        RRPChildWins,
        RRPParentWins
    };

    RPCChannel(RPCListener* aListener, RacyRPCPolicy aPolicy=RRPChildWins);

    virtual ~RPCChannel();

    
    bool Call(Message* msg, Message* reply);

    
    
    NS_OVERRIDE
    virtual bool Send(Message* msg);
    NS_OVERRIDE
    virtual bool Send(Message* msg, Message* reply);

    
    
    
    
    
    
    
    
    
    
    
    
    bool BlockChild();

    
    
    
    
    
    
    
    bool UnblockChild();

    
    bool IsOnCxxStack() const {
        return 0 < mCxxStackFrames;
    }

    NS_OVERRIDE
    virtual bool OnSpecialMessage(uint16 id, const Message& msg);

    
    
    NS_OVERRIDE
    virtual void OnMessageReceived(const Message& msg);
    NS_OVERRIDE
    virtual void OnChannelError();

#ifdef OS_WIN
    static bool IsSpinLoopActive() {
        return (sInnerEventLoopDepth > 0);
    }

protected:
    bool WaitForNotify();
    bool IsMessagePending();
    bool SpinInternalEventLoop();
    static void EnterModalLoop() {
        sInnerEventLoopDepth++;
    }
    static void ExitModalLoop() {
        sInnerEventLoopDepth--;
        NS_ASSERTION(sInnerEventLoopDepth >= 0,
            "sInnerEventLoopDepth dropped below zero!");
    }

    static int sInnerEventLoopDepth;
#endif

  private:
    

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
        static_cast<RPCListener*>(mListener)->OnEnteredCxxStack();
    }

    void ExitedCxxStack()
    {
        static_cast<RPCListener*>(mListener)->OnExitedCxxStack();
    }

    class NS_STACK_CLASS CxxStackFrame
    {
    public:
        CxxStackFrame(RPCChannel& that) : mThat(that) {
            NS_ABORT_IF_FALSE(0 <= mThat.mCxxStackFrames,
                              "mismatched CxxStackFrame ctor/dtor");
            mThat.AssertWorkerThread();

            if (0 == mThat.mCxxStackFrames++)
                mThat.EnteredCxxStack();
        }

        ~CxxStackFrame() {
            bool exitingStack = (0 == --mThat.mCxxStackFrames);

            
            
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

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    std::queue<Message> mPending;

    
    
    
    
    std::stack<Message> mStack;

    
    
    
    
    
    typedef std::map<size_t, Message> MessageMap;
    MessageMap mOutOfTurnReplies;

    
    
    
    
    std::stack<Message> mDeferred;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    size_t mRemoteStackDepthGuess;
    RacyRPCPolicy mRacePolicy;

    
    bool mBlockedOnParent;

    
    
    
    
    
    
    
    
    int mCxxStackFrames;
};


} 
} 
#endif  
