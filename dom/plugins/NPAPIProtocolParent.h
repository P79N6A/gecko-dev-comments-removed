





































#ifndef dom_plugins_NPAPIProtocolParentImpl_h
#define dom_plugins_NPAPIProtocolParentImpl_h 1

#include "chrome/common/ipc_channel.h"
#include "chrome/common/ipc_message.h"

#include "nsDebug.h"

#include "mozilla/ipc/RPCChannel.h"
#include "mozilla/plugins/NPAPIProtocol.h"


#include "mozilla/plugins/NPPProtocolParent.h"

#ifdef DEBUG
#  undef _MSG_LOG0
#  undef _MSG_LOG

#  define _MSG_LOG0(fmt, ...)                                       \
    do {                                                            \
        printf("[NPAPIProtocolParent] %s "fmt"\n", __VA_ARGS__);    \
    } while(0)

#  define _MSG_LOG(fmt, ...)                    \
    _MSG_LOG0(fmt, __FUNCTION__,## __VA_ARGS__)
#else
#  define _MSG_LOG(fmt, ...)
#endif

namespace mozilla {
namespace plugins {







class NS_FINAL_CLASS NPAPIProtocolParent :
        public NPAPIProtocol::Child,
        public mozilla::ipc::RPCChannel::Listener
{
private:
    typedef IPC::Message Message;
    typedef mozilla::ipc::RPCChannel RPCChannel;

public:
    NPAPIProtocolParent(NPAPIProtocol::Parent* aParent) :
        mParent(aParent),
        mRpc(this)
    {
    }

    virtual ~NPAPIProtocolParent()
    {
    }

    bool Open(IPC::Channel* aChannel)
    {
        return mRpc.Open(aChannel);
    }

    void Close()
    {
        mRpc.Close();
    }

    void NextState(NPAPIProtocol::State aNextState)
    {
        
    }

    
    virtual NPError NP_Initialize()
    {
        _MSG_LOG("outcall NP_Initialize()");

        Message reply;

        mRpc.Call(new NPAPI_ParentToChildMsg_NP_Initialize(),
                  &reply);
        NS_ASSERTION(NPAPI_ChildToParentMsg_Reply_NP_Initialize__ID
                     == reply.type(),
                     "wrong reply msg to NP_Initialize()");
        
        NPError ret0;
        NS_ASSERTION(NPAPI_ChildToParentMsg_Reply_NP_Initialize
                     ::Read(&reply, &ret0),
                     "bad types in reply msg to NP_Initialize()");

        return ret0;
    }

    virtual NPError NPP_New(const mozilla::ipc::String& aMimeType,
                             const int& aHandle,
                            const uint16_t& aMode,
                            const mozilla::ipc::StringArray& aNames,
                            const mozilla::ipc::StringArray& aValues)
    {
        _MSG_LOG("outcall NPP_New(%s, %hd, %hd, <vec>, <vec>)",
                 aMimeType.c_str(), aHandle, aMode);

        Message reply;

        mRpc.Call(new NPAPI_ParentToChildMsg_NPP_New(aMimeType,
                                                     aHandle,
                                                     aMode,
                                                     aNames,
                                                     aValues),
                  &reply);
        NS_ASSERTION(NPAPI_ChildToParentMsg_Reply_NPP_New__ID
                     == reply.type(),
                     "wrong reply msg to NPP_New()");

        NPError ret0;
        NS_ASSERTION(NPAPI_ChildToParentMsg_Reply_NPP_New
                     ::Read(&reply, &ret0),
                     "bad types in reply msg to NPP_New()");

        return ret0;
    }

    virtual void NPP_Destroy()
    {
        _MSG_LOG("outcall NPP_New()");
    }
    

    
    virtual Result OnCallReceived(const Message& msg, Message*& reply)
    {
        switch(msg.type()) {
        default:
            
            return HACK_npp->OnCallReceived(msg, reply);



        }
    }

    
    RPCChannel* HACK_getchannel_please() { return &mRpc; }
    NPPProtocolParent* HACK_npp;
    


private:
    NPAPIProtocol::Parent* mParent;
    RPCChannel mRpc;
    NPAPIProtocol::State mState;
};


} 
} 

#endif  
