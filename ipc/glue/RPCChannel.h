





































#ifndef ipc_glue_RPCChannel_h
#define ipc_glue_RPCChannel_h 1


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
        SyncChannel(aListener)
    {
    }

    virtual ~RPCChannel()
    {
        
    }

    
    bool Call(Message* msg, Message* reply);

    
    virtual void OnMessageReceived(const Message& msg);

private:
    
    virtual bool WaitingForReply() {
        mMutex.AssertCurrentThreadOwns();
        return mPending.size() > 0 || SyncChannel::WaitingForReply();
    }

    void OnDispatchMessage(const Message& msg);

    std::stack<Message> mPending;
};


} 
} 
#endif  
