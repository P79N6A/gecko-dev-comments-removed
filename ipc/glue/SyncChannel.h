






































#ifndef ipc_glue_SyncChannel_h
#define ipc_glue_SyncChannel_h 1

#include <queue>

#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "mozilla/ipc/AsyncChannel.h"

namespace mozilla {
namespace ipc {


class SyncChannel : public AsyncChannel
{
protected:
    typedef mozilla::CondVar CondVar;
    typedef mozilla::Mutex Mutex;
    typedef uint16 MessageId;
    typedef std::queue<Message> MessageQueue;

public:
    class  Listener :
        public AsyncChannel::Listener
    {
    public:
        virtual ~Listener() { }
        virtual Result OnMessageReceived(const Message& aMessage) = 0;
        virtual Result OnMessageReceived(const Message& aMessage,
                                         Message*& aReply) = 0;
    };

    SyncChannel(Listener* aListener) :
        AsyncChannel(aListener),
        mMutex("mozilla.ipc.SyncChannel.mMutex"),
        mCvar(mMutex, "mozilla.ipc.SyncChannel.mCvar")
    {
    }

    virtual ~SyncChannel()
    {
        
    }

    
    bool Send(Message* msg) {
        return AsyncChannel::Send(msg);
    }

    
    bool Send(Message* msg, Message* reply);

    
    virtual void OnMessageReceived(const Message& msg);

protected:
    
    virtual bool WaitingForReply() {
        mMutex.AssertCurrentThreadOwns();
        return mPendingReply != 0;
    }

    void OnDispatchMessage(const Message& aMsg);

    
    void OnSendReply(Message* msg);

    Mutex mMutex;
    CondVar mCvar;
    MessageId mPendingReply;
    Message mRecvd;
};


} 
} 
#endif  
