






































#ifndef ipc_glue_AsyncChannel_h
#define ipc_glue_AsyncChannel_h 1

#include "base/basictypes.h"
#include "base/message_loop.h"
#include "chrome/common/ipc_channel.h"

#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"




namespace mozilla {
namespace ipc {

struct HasResultCodes
{
    enum Result {
        MsgProcessed,
        MsgNotKnown,
        MsgNotAllowed,
        MsgPayloadError,
        MsgRouteError,
        MsgValueError,
    };
};

class AsyncChannel : public IPC::Channel::Listener, protected HasResultCodes
{
protected:
    typedef mozilla::CondVar CondVar;
    typedef mozilla::Mutex Mutex;

    enum ChannelState {
        ChannelClosed,
        ChannelOpening,
        ChannelConnected,
        ChannelTimeout,
        ChannelClosing,
        ChannelError
    };

public:
    typedef IPC::Channel Transport;
    typedef IPC::Message Message;

    class  AsyncListener: protected HasResultCodes
    {
    public:
        virtual ~AsyncListener() { }

        virtual void OnChannelClose() = 0;
        virtual void OnChannelError() = 0;
        virtual Result OnMessageReceived(const Message& aMessage) = 0;
    };

public:
    
    
    
    AsyncChannel(AsyncListener* aListener);
    virtual ~AsyncChannel();

    
    
    
    
    
    bool Open(Transport* aTransport, MessageLoop* aIOLoop=0);
    
    
    void Close();

    
    virtual bool Send(Message* msg);

    
    
    

    
    NS_OVERRIDE virtual void OnMessageReceived(const Message& msg);
    NS_OVERRIDE virtual void OnChannelConnected(int32 peer_pid);
    NS_OVERRIDE virtual void OnChannelError();

protected:
    
    void AssertWorkerThread()
    {
        NS_ABORT_IF_FALSE(mWorkerLoop == MessageLoop::current(),
                          "not on worker thread!");
    }

    void AssertIOThread()
    {
        NS_ABORT_IF_FALSE(mIOLoop == MessageLoop::current(),
                          "not on IO thread!");
    }

    bool Connected() {
        mMutex.AssertCurrentThreadOwns();
        return ChannelConnected == mChannelState;
    }

    
    void OnDispatchMessage(const Message& aMsg);
    virtual bool OnSpecialMessage(uint16 id, const Message& msg);
    void SendSpecialMessage(Message* msg);

    
    void SynchronouslyClose();

    bool MaybeHandleError(Result code, const char* channelName);
    void ReportConnectionError(const char* channelName);

    void PrintErrorMessage(const char* channelName, const char* msg)
    {
        fprintf(stderr, "\n###!!! [%s][%s] Error: %s\n\n",
                mChild ? "Child" : "Parent", channelName, msg);
    }

    

    void OnNotifyMaybeChannelError();
    virtual bool ShouldDeferNotifyMaybeError() {
        return false;
    }
    void NotifyChannelClosed();
    void NotifyMaybeChannelError();

    virtual void Clear();

    

    void OnChannelOpened();
    void OnSend(Message* aMsg);
    void OnCloseChannel();
    void PostErrorNotifyTask();

    
    
    bool MaybeInterceptSpecialIOMessage(const Message& msg);
    void ProcessGoodbyeMessage();

    Transport* mTransport;
    AsyncListener* mListener;
    ChannelState mChannelState;
    Mutex mMutex;
    CondVar mCvar;
    MessageLoop* mIOLoop;       
    MessageLoop* mWorkerLoop;   
    bool mChild;                
    CancelableTask* mChannelErrorTask; 
    IPC::Channel::Listener* mExistingListener; 
};


} 
} 
#endif  
