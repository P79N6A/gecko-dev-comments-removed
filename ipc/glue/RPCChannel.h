





































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

    RPCChannel(RPCListener* aListener) :
        SyncChannel(aListener),
        mPending(),
        mRemoteStackDepth(0)
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

    
    
    
    
    std::stack<Message> mStack;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    std::queue<Message> mPending;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    size_t mRemoteStackDepth;
};


} 
} 
#endif  
