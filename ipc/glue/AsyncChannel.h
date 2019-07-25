






































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

class AsyncChannel : public Transport::Listener, protected HasResultCodes
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
    
    
    void Close();

    
    virtual bool Send(Message* msg);

    
    void DispatchOnChannelConnected(int32 peer_pid);

    
    
    

    
    NS_OVERRIDE virtual void OnMessageReceived(const Message& msg);
    NS_OVERRIDE virtual void OnChannelConnected(int32 peer_pid);
    NS_OVERRIDE virtual void OnChannelError();

protected:
    
    void AssertWorkerThread() const
    {
        NS_ABORT_IF_FALSE(mWorkerLoop == MessageLoop::current(),
                          "not on worker thread!");
    }

    void AssertIOThread() const
    {
        NS_ABORT_IF_FALSE(mIOLoop == MessageLoop::current(),
                          "not on IO thread!");
    }

    bool Connected() const {
        mMonitor.AssertCurrentThreadOwns();
        return ChannelConnected == mChannelState;
    }

    
    void OnDispatchMessage(const Message& aMsg);
    virtual bool OnSpecialMessage(uint16 id, const Message& msg);
    void SendSpecialMessage(Message* msg) const;

    
    void SynchronouslyClose();

    bool MaybeHandleError(Result code, const char* channelName);
    void ReportConnectionError(const char* channelName) const;

    

    void SendThroughTransport(Message* msg) const;

    void OnNotifyMaybeChannelError();
    virtual bool ShouldDeferNotifyMaybeError() const {
        return false;
    }
    void NotifyChannelClosed();
    void NotifyMaybeChannelError();

    virtual void Clear();

    

    void OnChannelOpened();
    void OnCloseChannel();
    void PostErrorNotifyTask();

    
    
    bool MaybeInterceptSpecialIOMessage(const Message& msg);
    void ProcessGoodbyeMessage();

    Transport* mTransport;
    AsyncListener* mListener;
    ChannelState mChannelState;
    Monitor mMonitor;
    MessageLoop* mIOLoop;       
    MessageLoop* mWorkerLoop;   
    bool mChild;                
    CancelableTask* mChannelErrorTask; 
    Transport::Listener* mExistingListener; 
};


} 
} 
#endif  
