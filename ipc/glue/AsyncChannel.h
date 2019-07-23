






































#ifndef ipc_glue_AsyncChannel_h
#define ipc_glue_AsyncChannel_h 1

#include "base/basictypes.h"
#include "base/message_loop.h"
#include "chrome/common/ipc_channel.h"



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
    enum ChannelState {
        ChannelClosed,
        ChannelOpening,
        ChannelIdle,            
        ChannelWaiting,         
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
    typedef AsyncListener Listener;

    AsyncChannel(Listener* aListener) :
        mTransport(0),
        mListener(aListener),
        mChannelState(ChannelClosed),
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

    
    virtual void OnMessageReceived(const Message& msg);
    virtual void OnChannelConnected(int32 peer_pid);
    virtual void OnChannelError();

protected:
    
    void OnDispatchMessage(const Message& aMsg);

    
    void OnChannelOpened();
    void OnSend(Message* aMsg);

    Transport* mTransport;
    Listener* mListener;
    ChannelState mChannelState;
    MessageLoop* mIOLoop;       
    MessageLoop* mWorkerLoop;   
};


} 
} 
#endif
