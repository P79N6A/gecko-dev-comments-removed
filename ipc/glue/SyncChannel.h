






































#ifndef ipc_glue_SyncChannel_h
#define ipc_glue_SyncChannel_h 1

#include "mozilla/ipc/AsyncChannel.h"

namespace mozilla {
namespace ipc {


class SyncChannel : public AsyncChannel
{
protected:
    typedef IPC::Message::msgid_t msgid_t;

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
        virtual void OnProcessingError(Result aError) = 0;
        virtual bool OnReplyTimeout() = 0;
        virtual Result OnMessageReceived(const Message& aMessage,
                                         Message*& aReply) = 0;
        virtual void OnChannelConnected(int32 peer_pid) {};
    };

    SyncChannel(SyncListener* aListener);
    virtual ~SyncChannel();

    NS_OVERRIDE
    virtual bool Send(Message* msg) {
        return AsyncChannel::Send(msg);
    }

    
    virtual bool Send(Message* msg, Message* reply);

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

#ifdef OS_WIN
    struct NS_STACK_CLASS SyncStackFrame
    {
        SyncStackFrame(SyncChannel* channel, bool rpc);
        ~SyncStackFrame();

        bool mRPC;
        bool mSpinNestedEvents;
        SyncChannel* mChannel;

        
        SyncStackFrame* mPrev;

        
        SyncStackFrame* mStaticPrev;
    };
    friend struct SyncChannel::SyncStackFrame;

    static bool IsSpinLoopActive() {
        for (SyncStackFrame* frame = sStaticTopFrame;
             frame;
             frame = frame->mPrev) {
            if (frame->mSpinNestedEvents)
                return true;
        }
        return false;
    }

protected:
    
    SyncStackFrame* mTopFrame;

    
    static SyncStackFrame* sStaticTopFrame;
#endif 

protected:
    
    bool ProcessingSyncMessage() const {
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

    
    void NotifyWorkerThread();

    
    bool AwaitingSyncReply() const {
        mMonitor.AssertCurrentThreadOwns();
        return mPendingReply != 0;
    }

    int32 NextSeqno() {
        AssertWorkerThread();
        return mChild ? --mNextSeqno : ++mNextSeqno;
    }

    msgid_t mPendingReply;
    bool mProcessingSyncMessage;
    Message mRecvd;
    
    
    int32 mNextSeqno;

    static bool sIsPumpingMessages;

    int32 mTimeoutMs;

#ifdef OS_WIN
    HANDLE mEvent;
#endif

private:
    bool EventOccurred();
};


} 
} 
#endif  
