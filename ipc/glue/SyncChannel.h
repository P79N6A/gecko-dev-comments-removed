






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
    static const int32_t kNoTimeout;

    class  SyncListener : 
        public AsyncChannel::AsyncListener
    {
    public:
        virtual ~SyncListener() { }

        virtual void OnChannelClose() = 0;
        virtual void OnChannelError() = 0;
        virtual Result OnMessageReceived(const Message& aMessage) = 0;
        virtual void OnProcessingError(Result aError) = 0;
        virtual int32_t GetProtocolTypeId() = 0;
        virtual bool OnReplyTimeout() = 0;
        virtual Result OnMessageReceived(const Message& aMessage,
                                         Message*& aReply) = 0;
        virtual void OnChannelConnected(int32_t peer_pid) {}
    };

    SyncChannel(SyncListener* aListener);
    virtual ~SyncChannel();

    virtual bool Send(Message* msg) MOZ_OVERRIDE {
        return AsyncChannel::Send(msg);
    }

    
    virtual bool Send(Message* msg, Message* reply);

    
    
    void SetReplyTimeoutMs(int32_t aTimeoutMs) {
        AssertWorkerThread();
        mTimeoutMs = (aTimeoutMs <= 0) ? kNoTimeout :
          
          (int32_t)ceil((double)aTimeoutMs/2.0);
    }

    static bool IsPumpingMessages() {
        return sIsPumpingMessages;
    }
    static void SetIsPumpingMessages(bool aIsPumping) {
        sIsPumpingMessages = aIsPumping;
    }

#ifdef OS_WIN
public:
    struct NS_STACK_CLASS SyncStackFrame
    {
        SyncStackFrame(SyncChannel* channel, bool rpc);
        ~SyncStackFrame();

        bool mRPC;
        bool mSpinNestedEvents;
        bool mListenerNotified;
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
    
    
    virtual void OnMessageReceivedFromLink(const Message& msg) MOZ_OVERRIDE;
    virtual void OnChannelErrorFromLink() MOZ_OVERRIDE;

    
    bool ProcessingSyncMessage() const {
        return mProcessingSyncMessage;
    }

    void OnDispatchMessage(const Message& aMsg);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool WaitForNotify();

    bool ShouldContinueFromTimeout();

    
    void NotifyWorkerThread();

    
    bool AwaitingSyncReply() const {
        mMonitor->AssertCurrentThreadOwns();
        return mPendingReply != 0;
    }

    int32_t NextSeqno() {
        AssertWorkerThread();
        return mChild ? --mNextSeqno : ++mNextSeqno;
    }

    msgid_t mPendingReply;
    bool mProcessingSyncMessage;
    Message mRecvd;
    
    
    int32_t mNextSeqno;

    static bool sIsPumpingMessages;

    
    
    
    
    bool WaitResponse(bool aWaitTimedOut);
    bool mInTimeoutSecondHalf;
    int32_t mTimeoutMs;

#ifdef OS_WIN
    HANDLE mEvent;
#endif

private:
    bool EventOccurred();
};


} 
} 
#endif  
