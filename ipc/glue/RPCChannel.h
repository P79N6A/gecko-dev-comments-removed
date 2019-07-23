





































#ifndef ipc_glue_RPCChannel_h
#define ipc_glue_RPCChannel_h 1


#include <queue>
#include <stack>

#include "mozilla/ipc/SyncChannel.h"

namespace mozilla {
namespace ipc {


class RPCChannel : public SyncChannel
{
public:
    class  RPCListener :
        public SyncChannel::SyncListener
    {
    public:
        virtual ~RPCListener() { }
        virtual Result OnMessageReceived(const Message& aMessage) = 0;
        virtual Result OnMessageReceived(const Message& aMessage,
                                         Message*& aReply) = 0;
        virtual Result OnCallReceived(const Message& aMessage,
                                      Message*& aReply) = 0;
    };

    
    enum RacyRPCPolicy {
        RRPError,
        RRPChildWins,
        RRPParentWins
    };

    RPCChannel(RPCListener* aListener, RacyRPCPolicy aPolicy=RRPChildWins) :
        SyncChannel(aListener),
        mPending(),
        mRemoteStackDepthGuess(0),
        mRacePolicy(aPolicy)
    {
    }

    virtual ~RPCChannel()
    {
        
    }

    
    bool Call(Message* msg, Message* reply);

    
    NS_OVERRIDE virtual void OnMessageReceived(const Message& msg);
    NS_OVERRIDE virtual void OnChannelError();

protected:
    
    
    
    void OnDelegate(const Message& msg);

    
    
    
    
    void OnMaybeDequeueOne();

private:
    void OnIncall(const Message& msg);
    void ProcessIncall(const Message& call, size_t stackDepth);

    
    size_t StackDepth() {
        mMutex.AssertCurrentThreadOwns();
        return mStack.size();
    }

#define RPC_DEBUGABORT(...) \
    DebugAbort(__FILE__, __LINE__,## __VA_ARGS__)

    void DebugAbort(const char* file, int line,
                    const char* why,
                    const char* type="rpc", bool reply=false)
    {
        fprintf(stderr,
                "[RPCChannel][%s][%s:%d] Aborting: %s (triggered by %s%s)\n",
                mChild ? "Child" : "Parent",
                file, line,
                why,
                type, reply ? "reply" : "");
        
        fprintf(stderr, "  local RPC stack size: %zu\n",
                mStack.size());
        fprintf(stderr, "  remote RPC stack guess: %zd\n",
                mRemoteStackDepthGuess);
        fprintf(stderr, "  Pending queue size: %zu, front to back:\n",
                mPending.size());
        while (!mPending.empty()) {
            fprintf(stderr, "    [ %s%s ]\n",
                    mPending.front().is_rpc() ? "rpc" :
                        (mPending.front().is_sync() ? "sync" : "async"),
                    mPending.front().is_reply() ? "reply" : "");
            mPending.pop();
        }

        NS_RUNTIMEABORT(why);
    }

    
    
    
    
    std::stack<Message> mStack;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    std::queue<Message> mPending;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    size_t mRemoteStackDepthGuess;

    RacyRPCPolicy mRacePolicy;
};


} 
} 
#endif  
