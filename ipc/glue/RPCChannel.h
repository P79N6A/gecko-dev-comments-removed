





































#ifndef ipc_glue_RPCChannel_h
#define ipc_glue_RPCChannel_h 1


#include <stack>

#include "base/basictypes.h"
#include "base/message_loop.h"
#include "chrome/common/ipc_channel.h"

#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"

namespace mozilla {
namespace ipc {


class RPCChannel : public IPC::Channel::Listener
{
private:
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

    class Listener
    {
    public:
        enum Result {
            MsgProcessed,
            MsgNotKnown,
            MsgNotAllowed,
            MsgPayloadError,
            MsgRouteError,
            MsgValueError,
        };

        virtual ~Listener() { }
        virtual Result OnCallReceived(const Message& aMessage,
                                      Message*& aReply) = 0;
    };

    






    RPCChannel(Listener* aListener) :
        mTransport(0),
        mListener(aListener),
        mChannelState(ChannelClosed),
        mMutex("mozilla.ipc.RPCChannel.mMutex"),
        mCvar(mMutex, "mozilla.ipc.RPCChannel.mCvar")
    {
    }

    virtual ~RPCChannel()
    {
        if (mTransport)
            Close();
        mTransport = 0;
    }

    
    
    bool Open(Transport* aTransport, MessageLoop* aIOLoop=0);
    
    
    
    void Close();

    
    virtual void OnMessageReceived(const Message& msg);
    virtual void OnChannelConnected(int32 peer_pid);
    virtual void OnChannelError();

    
    virtual bool Call(Message* msg, Message* reply);

private:
    
    
    void OnIncomingCall(Message msg);
    
    bool ProcessIncomingCall(Message msg);

    void OnChannelOpened();
    void SendCall(Message* aCall);
    void SendReply(Message* aReply);

    Transport* mTransport;
    Listener* mListener;
    ChannelState mChannelState;
    MessageLoop* mIOLoop;       
    MessageLoop* mWorkerLoop;   
    Mutex mMutex;
    CondVar mCvar;
    std::stack<Message> mPending;
};


} 
} 
#endif
