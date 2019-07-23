






































#ifndef ipc_glue_SyncChannel_h
#define ipc_glue_SyncChannel_h 1

#include <queue>

#include "mozilla/ipc/AsyncChannel.h"

namespace mozilla {
namespace ipc {


class SyncChannel : public AsyncChannel
{
protected:
    typedef uint16 MessageId;

public:
    class  SyncListener : 
        public AsyncChannel::AsyncListener
    {
    public:
        virtual ~SyncListener() { }
        virtual Result OnMessageReceived(const Message& aMessage) = 0;
        virtual Result OnMessageReceived(const Message& aMessage,
                                         Message*& aReply) = 0;
    };

    SyncChannel(SyncListener* aListener) :
        AsyncChannel(aListener),
        mPendingReply(0),
        mProcessingSyncMessage(false)
    {
    }

    virtual ~SyncChannel()
    {
        
    }

    bool Send(Message* msg) {
        return AsyncChannel::Send(msg);
    }

    
    bool Send(Message* msg, Message* reply);

    
    NS_OVERRIDE virtual void OnMessageReceived(const Message& msg);
    NS_OVERRIDE virtual void OnChannelError();

protected:
    
    bool ProcessingSyncMessage() {
        return mProcessingSyncMessage;
    }

    void OnDispatchMessage(const Message& aMsg);

    
    void OnSendReply(Message* msg);

    
    bool AwaitingSyncReply() {
        mMutex.AssertCurrentThreadOwns();
        return mPendingReply != 0;
    }

    MessageId mPendingReply;
    bool mProcessingSyncMessage;
    Message mRecvd;
};


} 
} 
#endif  
