






































#ifndef ipc_glue_SyncChannel_h
#define ipc_glue_SyncChannel_h 1

#include "base/basictypes.h"

#include "prinrval.h"

#include "mozilla/ipc/AsyncChannel.h"

namespace mozilla {
namespace ipc {


class SyncChannel : public AsyncChannel
{
protected:
    typedef uint16 MessageId;

public:
    static const int32 kNoTimeout;

    class  SyncListener : 
        public AsyncChannel::AsyncListener
    {
    public:
        virtual ~SyncListener() { }

        virtual void OnChannelClose() = 0;
        virtual void OnChannelError() = 0;
        virtual Result OnMessageReceived(const Message& aMessage) = 0;
        virtual bool OnReplyTimeout() = 0;
        virtual Result OnMessageReceived(const Message& aMessage,
                                         Message*& aReply) = 0;
    };

    SyncChannel(SyncListener* aListener);
    virtual ~SyncChannel();

    bool Send(Message* msg) {
        return AsyncChannel::Send(msg);
    }

    
    bool Send(Message* msg, Message* reply);

    void SetReplyTimeoutMs(int32 aTimeoutMs) {
        AssertWorkerThread();
        mTimeoutMs = (aTimeoutMs <= 0) ? kNoTimeout : aTimeoutMs;
    }

    
    NS_OVERRIDE virtual void OnMessageReceived(const Message& msg);
    NS_OVERRIDE virtual void OnChannelError();

    static bool IsPumpingMessages() {
        return sIsPumpingMessages;
    }
    static void SetIsPumpingMessages(bool aIsPumping) {
        sIsPumpingMessages = aIsPumping;
    }

protected:
    
    bool ProcessingSyncMessage() {
        return mProcessingSyncMessage;
    }

    void OnDispatchMessage(const Message& aMsg);

    NS_OVERRIDE
    bool OnSpecialMessage(uint16 id, const Message& msg)
    {
        
        return AsyncChannel::OnSpecialMessage(id, msg);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool WaitForNotify();

    bool ShouldContinueFromTimeout();

    
    void OnSendReply(Message* msg);
    void NotifyWorkerThread();

    
    bool AwaitingSyncReply() {
        mMutex.AssertCurrentThreadOwns();
        return mPendingReply != 0;
    }

    int32 NextSeqno() {
        AssertWorkerThread();
        return mChild ? --mNextSeqno : ++mNextSeqno;
    }

    MessageId mPendingReply;
    bool mProcessingSyncMessage;
    Message mRecvd;
    
    
    int32 mNextSeqno;

    static bool sIsPumpingMessages;

private:
    bool EventOccurred();

    int32 mTimeoutMs;
};


} 
} 
#endif  
