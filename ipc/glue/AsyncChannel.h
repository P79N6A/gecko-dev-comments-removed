






































#ifndef ipc_glue_AsyncChannel_h
#define ipc_glue_AsyncChannel_h 1

#include "base/basictypes.h"
#include "base/message_loop.h"

#include "mozilla/Monitor.h"
#include "mozilla/ipc/Transport.h"



namespace mozilla {
namespace ipc {

struct HasResultCodes
{
    enum Result {
        MsgProcessed,
        MsgDropped,
        MsgNotKnown,
        MsgNotAllowed,
        MsgPayloadError,
        MsgProcessingError,
        MsgRouteError,
        MsgValueError,
    };
};


class RefCountedMonitor : public Monitor
{
public:
    RefCountedMonitor() 
        : Monitor("mozilla.ipc.AsyncChannel.mMonitor")
    {}

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RefCountedMonitor)
};

class AsyncChannel : protected HasResultCodes
{
protected:
    typedef mozilla::Monitor Monitor;

    enum ChannelState {
        ChannelClosed,
        ChannelOpening,
        ChannelConnected,
        ChannelTimeout,
        ChannelClosing,
        ChannelError
    };

public:
    typedef IPC::Message Message;
    typedef mozilla::ipc::Transport Transport;

    class  AsyncListener: protected HasResultCodes
    {
    public:
        virtual ~AsyncListener() { }

        virtual void OnChannelClose() = 0;
        virtual void OnChannelError() = 0;
        virtual Result OnMessageReceived(const Message& aMessage) = 0;
        virtual void OnProcessingError(Result aError) = 0;
        virtual void OnChannelConnected(int32 peer_pid) {};
    };

    enum Side { Parent, Child, Unknown };

public:
    
    
    
    AsyncChannel(AsyncListener* aListener);
    virtual ~AsyncChannel();

    
    
    
    
    
    bool Open(Transport* aTransport, MessageLoop* aIOLoop=0, Side aSide=Unknown);
    
    
    
    
    
    
    
    
    
    bool Open(AsyncChannel *aTargetChan, MessageLoop *aTargetLoop, Side aSide);

    
    void Close();

    
    virtual bool Send(Message* msg);

    
    
    virtual bool Echo(Message* msg);

    
    void DispatchOnChannelConnected(int32 peer_pid);

    
    
    
    
    
    
    
    
    

    class Link {
    protected:
        AsyncChannel *mChan;

    public:
        Link(AsyncChannel *aChan);
        virtual ~Link();

        
        
        virtual void EchoMessage(Message *msg) = 0;
        virtual void SendMessage(Message *msg) = 0;
        virtual void SendClose() = 0;
    };

    class ProcessLink : public Link, public Transport::Listener {
    protected:
        Transport* mTransport;
        MessageLoop* mIOLoop;       
        Transport::Listener* mExistingListener; 
    
        void OnCloseChannel();
        void OnChannelOpened();
        void OnEchoMessage(Message* msg);

        void AssertIOThread() const
        {
            NS_ABORT_IF_FALSE(mIOLoop == MessageLoop::current(),
                              "not on I/O thread!");
        }

    public:
        ProcessLink(AsyncChannel *chan);
        virtual ~ProcessLink();
        void Open(Transport* aTransport, MessageLoop *aIOLoop, Side aSide);
        
        
        
        
        
        NS_OVERRIDE virtual void OnMessageReceived(const Message& msg);
        NS_OVERRIDE virtual void OnChannelConnected(int32 peer_pid);
        NS_OVERRIDE virtual void OnChannelError();

        NS_OVERRIDE virtual void EchoMessage(Message *msg);
        NS_OVERRIDE virtual void SendMessage(Message *msg);
        NS_OVERRIDE virtual void SendClose();
    };
    
    class ThreadLink : public Link {
    protected:
        AsyncChannel* mTargetChan;
    
    public:
        ThreadLink(AsyncChannel *aChan, AsyncChannel *aTargetChan);
        virtual ~ThreadLink();

        NS_OVERRIDE virtual void EchoMessage(Message *msg);
        NS_OVERRIDE virtual void SendMessage(Message *msg);
        NS_OVERRIDE virtual void SendClose();
    };

protected:
    
    
    
    void AssertLinkThread() const
    {
        NS_ABORT_IF_FALSE(mWorkerLoop != MessageLoop::current(),
                          "on worker thread but should not be!");
    }

    
    void AssertWorkerThread() const
    {
        NS_ABORT_IF_FALSE(mWorkerLoop == MessageLoop::current(),
                          "not on worker thread!");
    }

    bool Connected() const {
        mMonitor->AssertCurrentThreadOwns();
        return ChannelConnected == mChannelState;
    }

    
    
    virtual bool MaybeInterceptSpecialIOMessage(const Message& msg);
    void ProcessGoodbyeMessage();

    
    
    
    
    
    
    
    virtual void OnMessageReceivedFromLink(const Message& msg);
    virtual void OnChannelErrorFromLink();
    void PostErrorNotifyTask();

    
    void OnDispatchMessage(const Message& aMsg);
    virtual bool OnSpecialMessage(uint16 id, const Message& msg);
    void SendSpecialMessage(Message* msg) const;

    
    void SynchronouslyClose();

    bool MaybeHandleError(Result code, const char* channelName);
    void ReportConnectionError(const char* channelName) const;

    

    void OnNotifyMaybeChannelError();
    virtual bool ShouldDeferNotifyMaybeError() const {
        return false;
    }
    void NotifyChannelClosed();
    void NotifyMaybeChannelError();
    void OnOpenAsSlave(AsyncChannel *aTargetChan, Side aSide);
    void CommonThreadOpenInit(AsyncChannel *aTargetChan, Side aSide);

    virtual void Clear();

    AsyncListener* mListener;
    ChannelState mChannelState;
    nsRefPtr<RefCountedMonitor> mMonitor;
    MessageLoop* mWorkerLoop;   
    bool mChild;                
    CancelableTask* mChannelErrorTask; 
    Link *mLink;                
};

} 
} 
#endif
