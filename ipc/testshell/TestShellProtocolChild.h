




#ifndef mozilla_ipc_TestShellProtocolChild_h
#define mozilla_ipc_TestShellProtocolChild_h

#include "mozilla/ipc/TestShellProtocol.h"
#include "mozilla/ipc/RPCChannel.h"

namespace mozilla {
namespace ipc {


class  TestShellProtocolChild :
    public mozilla::ipc::RPCChannel::Listener
{
protected:
    typedef mozilla::ipc::String String;
    typedef mozilla::ipc::StringArray StringArray;

    virtual nsresult AnswerSendCommand(const String& aCommand) = 0;

private:
    typedef IPC::Message Message;
    typedef mozilla::ipc::RPCChannel Channel;

public:
    TestShellProtocolChild() :
        mChannel(this)
    {
    }

    virtual ~TestShellProtocolChild()
    {
    }

    bool Open(
                IPC::Channel* aChannel,
                MessageLoop* aThread = 0)
    {
        return mChannel.Open(aChannel, aThread);
    }

    void Close()
    {
        mChannel.Close();
    }

    virtual Result OnMessageReceived(const Message& msg)
    {
        switch (msg.type()) {
        default:
            {
                return MsgNotKnown;
            }
        }
    }

    virtual Result OnMessageReceived(
                const Message& msg,
                Message*& reply)
    {
        switch (msg.type()) {
        default:
            {
                return MsgNotKnown;
            }
        }
    }

    virtual Result OnCallReceived(
                const Message& msg,
                Message*& reply)
    {
        switch (msg.type()) {
        case TestShellProtocol::Msg_SendCommand__ID:
            {
                String aCommand;

                if (!(TestShellProtocol::Msg_SendCommand::Read(&(msg), &(aCommand)))) {
                    return MsgPayloadError;
                }
                if (NS_FAILED(AnswerSendCommand(aCommand))) {
                    return MsgValueError;
                }

                reply = new TestShellProtocol::Reply_SendCommand();
                reply->set_reply();
                return MsgProcessed;
            }
        default:
            {
                return MsgNotKnown;
            }
        }
    }

private:
    Channel mChannel;
    int mId;
    int mPeerId;
    mozilla::ipc::IProtocolManager* mManager;
};


} 
} 
#if 0





class ActorImpl :
    public TestShellProtocolChild
{
    virtual nsresult AnswerSendCommand(const String& aCommand);
    ActorImpl();
    virtual ~ActorImpl();
};



nsresult ActorImpl::AnswerSendCommand(const String& aCommand)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

ActorImpl::ActorImpl()
{
}

ActorImpl::~ActorImpl()
{
}

#endif 

#endif 
