






































#ifndef ipc_glue_AsyncChannel_h
#define ipc_glue_AsyncChannel_h 1

#include "base/basictypes.h"
#include "base/message_loop.h"
#include "chrome/common/ipc_channel.h"

#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"



namespace {
enum Result {
    MsgProcessed,
    MsgNotKnown,
    MsgNotAllowed,
    MsgPayloadError,
    MsgRouteError,
    MsgValueError,
};
} 

namespace mozilla {
namespace ipc {

class AsyncChannel : public IPC::Channel::Listener
{
protected:
    typedef mozilla::CondVar CondVar;
    typedef mozilla::Mutex Mutex;

    enum ChannelState {
        ChannelClosed,
        ChannelOpening,
        ChannelConnected,
        ChannelError
    };

public:
    typedef IPC::Channel Transport;
    typedef IPC::Message Message;

    class  AsyncListener
    {
    public:
        virtual ~AsyncListener() { }
        virtual Result OnMessageReceived(const Message& aMessage) = 0;
    };

    AsyncChannel(AsyncListener* aListener) :
        mTransport(0),
        mListener(aListener),
        mChannelState(ChannelClosed),
        mMutex("mozilla.ipc.AsyncChannel.mMutex"),
        mCvar(mMutex, "mozilla.ipc.AsyncChannel.mCvar"),
        mIOLoop(),
        mWorkerLoop()
    {
    }

    virtual ~AsyncChannel()
    {
        if (mTransport)
            Close();
        mTransport = 0;
    }

    
    
    
    
    
    bool Open(Transport* aTransport, MessageLoop* aIOLoop=0);
    
    
    
    void Close();

    
    bool Send(Message* msg);

    
    NS_OVERRIDE virtual void OnMessageReceived(const Message& msg);
    NS_OVERRIDE virtual void OnChannelConnected(int32 peer_pid);
    NS_OVERRIDE virtual void OnChannelError();

protected:
    
    void AssertWorkerThread()
    {
        if (mWorkerLoop != MessageLoop::current()) {
            NS_ERROR("not on worker thread!");
        }
    }

    void AssertIOThread()
    {
        if (mIOLoop != MessageLoop::current()) {
            NS_ERROR("not on IO thread!");
        }
    }

    bool Connected() {
        return ChannelConnected == mChannelState;
    }

    
    void OnDispatchMessage(const Message& aMsg);

    
    void OnChannelOpened();
    void OnSend(Message* aMsg);

    Transport* mTransport;
    AsyncListener* mListener;
    ChannelState mChannelState;
    Mutex mMutex;
    CondVar mCvar;
    MessageLoop* mIOLoop;       
    MessageLoop* mWorkerLoop;   
};


} 
} 
#endif  
