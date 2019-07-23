





































#ifndef dom_plugins_NPPProtocolParent_h
#define dom_plugins_NPPProtocolParent_h 1

#include "mozilla/plugins/NPPProtocol.h"

#include "npapi.h"

#include "nsDebug.h"
#include "IPC/IPCMessageUtils.h"

#include "mozilla/ipc/RPCChannel.h"

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


class NS_FINAL_CLASS NPPProtocolParent :
        public NPPProtocol::Child,
        public mozilla::ipc::RPCChannel::Listener
{
private:
    typedef IPC::Message Message;
    typedef mozilla::ipc::RPCChannel RPCChannel;

public:
    NPPProtocolParent(NPPProtocol::Parent* aParent) :
        mParent(aParent)
    {

    }

    virtual ~NPPProtocolParent()
    {

    }

    
    

    void SetChannel(RPCChannel* aRpc)
    {
        
        NS_ASSERTION(aRpc, "need a valid RPC channel");
        mRpc = aRpc;
    }

    void NextState(NPPProtocol::State aNextState)
    {
        
    }

    
    virtual NPError NPP_SetWindow(XID aWindow,
                                  int32_t aWidth,
                                  int32_t aHeight)
    {
        _MSG_LOG("outcall NPP_SetWindow(%lx, %d, %d)",
                 aWindow, aWidth, aHeight);

        Message reply;

        mRpc->Call(new NPP_ParentToChildMsg_NPP_SetWindow(aWindow,
                                                          aWidth,
                                                          aHeight),
                   &reply);
        NS_ASSERTION(NPP_ChildToParentMsg_Reply_NPP_SetWindow__ID
                     == reply.type(),
                     "wrong reply msg to NPP_SetWindow()");

        NPError ret0;
        NS_ASSERTION(NPP_ChildToParentMsg_Reply_NPP_SetWindow
                     ::Read(&reply, &ret0),
                     "bad types in reply msg to NPP_SetWindow()");

        return ret0;
    }

    virtual Result OnCallReceived(const Message& msg, Message*& reply)
    {
        switch(msg.type()) {
        case NPP_ChildToParentMsg_NPN_GetValue__ID: {
            _MSG_LOG("incall NPN_GetValue()");

            
            mParent->NPN_GetValue();

            reply = new NPP_ParentToChildMsg_Reply_NPN_GetValue();
            return MsgProcessed;
        }

        default:
            return MsgNotKnown;
        }
    }

private:
    NPPProtocol::Parent* mParent;
    NPPProtocol::State mState;
    RPCChannel* mRpc;
};


} 
} 

#endif 
