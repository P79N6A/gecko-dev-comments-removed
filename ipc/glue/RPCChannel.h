





































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

        virtual void OnChannelClose() = 0;
        virtual void OnChannelError() = 0;
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

    RPCChannel(RPCListener* aListener, RacyRPCPolicy aPolicy=RRPChildWins);

    virtual ~RPCChannel();

    
    bool Call(Message* msg, Message* reply);

    
    
    NS_OVERRIDE virtual void OnMessageReceived(const Message& msg);
    NS_OVERRIDE virtual void OnChannelError();

private:
    

    void MaybeProcessDeferredIncall();
    void EnqueuePendingMessages();

    void OnMaybeDequeueOne();
    void Incall(const Message& call, size_t stackDepth);
    void DispatchIncall(const Message& call);

    
    size_t StackDepth() {
        mMutex.AssertCurrentThreadOwns();
        return mStack.size();
    }

    void DebugAbort(const char* file, int line, const char* cond,
                    const char* why,
                    const char* type="rpc", bool reply=false);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    std::queue<Message> mPending;

    
    
    
    
    std::stack<Message> mStack;

    
    
    
    
    std::stack<Message> mDeferred;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    size_t mRemoteStackDepthGuess;
    RacyRPCPolicy mRacePolicy;
};


} 
} 
#endif  
