






#ifndef ipc_glue_MessageLink_h
#define ipc_glue_MessageLink_h 1

#include "base/basictypes.h"
#include "base/message_loop.h"

#include "mozilla/WeakPtr.h"
#include "mozilla/ipc/Transport.h"

namespace mozilla {
namespace ipc {

class MessageChannel;

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
        MsgValueError
    };
};

enum Side {
    ParentSide,
    ChildSide,
    UnknownSide
};

enum ChannelState {
    ChannelClosed,
    ChannelOpening,
    ChannelConnected,
    ChannelTimeout,
    ChannelClosing,
    ChannelError
};


enum RacyInterruptPolicy {
    RIPError,
    RIPChildWins,
    RIPParentWins
};

class MessageListener
  : protected HasResultCodes,
    public mozilla::SupportsWeakPtr<MessageListener>
{
  public:
    MOZ_DECLARE_WEAKREFERENCE_TYPENAME(MessageListener)
    typedef IPC::Message Message;

    virtual ~MessageListener() { }

    virtual void OnChannelClose() = 0;
    virtual void OnChannelError() = 0;
    virtual Result OnMessageReceived(const Message& aMessage) = 0;
    virtual Result OnMessageReceived(const Message& aMessage, Message *& aReply) = 0;
    virtual Result OnCallReceived(const Message& aMessage, Message *& aReply) = 0;
    virtual void OnProcessingError(Result aError, const char* aMsgName) = 0;
    virtual void OnChannelConnected(int32_t peer_pid) {}
    virtual bool OnReplyTimeout() {
        return false;
    }

    virtual void OnEnteredCxxStack() {
        NS_RUNTIMEABORT("default impl shouldn't be invoked");
    }
    virtual void OnExitedCxxStack() {
        NS_RUNTIMEABORT("default impl shouldn't be invoked");
    }
    virtual void OnEnteredCall() {
        NS_RUNTIMEABORT("default impl shouldn't be invoked");
    }
    virtual void OnExitedCall() {
        NS_RUNTIMEABORT("default impl shouldn't be invoked");
    }
    

    virtual void OnBeginSyncTransaction() {
    }
    virtual RacyInterruptPolicy MediateInterruptRace(const Message& parent,
                                                     const Message& child)
    {
        return RIPChildWins;
    }

    virtual void OnEnteredSyncSend() {
    }
    virtual void OnExitedSyncSend() {
    }

    virtual void ProcessRemoteNativeEventsInInterruptCall() {
    }

    
    
    virtual int32_t GetProtocolTypeId() = 0;
};

class MessageLink
{
  public:
    typedef IPC::Message Message;

    explicit MessageLink(MessageChannel *aChan);
    virtual ~MessageLink();

    
    
    virtual void EchoMessage(Message *msg) = 0;
    virtual void SendMessage(Message *msg) = 0;
    virtual void SendClose() = 0;

    virtual bool Unsound_IsClosed() const = 0;
    virtual uint32_t Unsound_NumQueuedMessages() const = 0;

  protected:
    MessageChannel *mChan;
};

class ProcessLink
  : public MessageLink,
    public Transport::Listener
{
    void OnCloseChannel();
    void OnChannelOpened();
    void OnTakeConnectedChannel();
    void OnEchoMessage(Message* msg);

    void AssertIOThread() const
    {
        MOZ_ASSERT(mIOLoop == MessageLoop::current(),
                   "not on I/O thread!");
    }

  public:
    explicit ProcessLink(MessageChannel *chan);
    virtual ~ProcessLink();

    
    
    
    
    
    
    
    void Open(Transport* aTransport, MessageLoop *aIOLoop, Side aSide);
    
    
    
    
    
    virtual void OnMessageReceived(const Message& msg) override;
    virtual void OnChannelConnected(int32_t peer_pid) override;
    virtual void OnChannelError() override;

    virtual void EchoMessage(Message *msg) override;
    virtual void SendMessage(Message *msg) override;
    virtual void SendClose() override;

    virtual bool Unsound_IsClosed() const override;
    virtual uint32_t Unsound_NumQueuedMessages() const override;

  protected:
    Transport* mTransport;
    MessageLoop* mIOLoop;       
    Transport::Listener* mExistingListener; 
#ifdef MOZ_NUWA_PROCESS
    bool mIsToNuwaProcess;
#endif
};

class ThreadLink : public MessageLink
{
  public:
    ThreadLink(MessageChannel *aChan, MessageChannel *aTargetChan);
    virtual ~ThreadLink();

    virtual void EchoMessage(Message *msg) override;
    virtual void SendMessage(Message *msg) override;
    virtual void SendClose() override;

    virtual bool Unsound_IsClosed() const override;
    virtual uint32_t Unsound_NumQueuedMessages() const override;

  protected:
    MessageChannel* mTargetChan;
};

} 
} 

#endif  

