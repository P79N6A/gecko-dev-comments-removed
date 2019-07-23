





































#ifndef dom_plugins_NPPProtocolChild_h
#define dom_plugins_NPPProtocolChild_h 1

#include "npapi.h"

#include "nsDebug.h"

#include "IPC/IPCMessageUtils.h"
#include "mozilla/ipc/RPCChannel.h"
#include "mozilla/plugins/NPPProtocol.h"

#ifdef DEBUG
#  undef _MSG_LOG0
#  undef _MSG_LOG

#  define _MSG_LOG0(fmt, ...)                                       \
    do {                                                            \
        printf("[NPAPIProtocolChild] %s "fmt"\n", __VA_ARGS__);    \
    } while(0)

#  define _MSG_LOG(fmt, ...)                    \
    _MSG_LOG0(fmt, __FUNCTION__,## __VA_ARGS__)
#else
#  define _MSG_LOG(fmt, ...)
#endif

namespace mozilla {
namespace plugins {


class NS_FINAL_CLASS NPPProtocolChild :
        public NPPProtocol::Parent,
        public mozilla::ipc::RPCChannel::Listener
{
private:
    typedef IPC::Message Message;
    typedef mozilla::ipc::RPCChannel RPCChannel;

public:
    NPPProtocolChild(NPPProtocol::Child* aChild) :
        mChild(aChild)
    {

    }

    virtual ~NPPProtocolChild()
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

    
    virtual void NPN_GetValue()
    {
        _MSG_LOG("outcall NPN_GetValue()");

        Message reply;

        mRpc->Call(new NPP_ChildToParentMsg_NPN_GetValue(),
                   &reply);
        NS_ASSERTION(NPP_ParentToChildMsg_Reply_NPN_GetValue__ID
                     == reply.type(),
                     "wrong reply msg to NPN_GetValue()");
        
        return;
    }

    virtual Result OnCallReceived(const Message& msg, Message** reply)
    {
        switch(msg.type()) {
        case NPP_ParentToChildMsg_NPP_SetWindow__ID: {
            XID aWindow;
            int32_t aWidth;
            int32_t aHeight;

            NPP_ParentToChildMsg_NPP_SetWindow::Param p;
            NS_ASSERTION(NPP_ParentToChildMsg_NPP_SetWindow::Read(&msg, &p),
                         "bad types in NPP_SetWindow() msg");
            aWindow = p.a;
            aWidth = p.b;
            aHeight = p.c;

            _MSG_LOG("incall NPP_SetWindow(%lx, %d, %d)\n",
                     aWindow, aWidth, aHeight);

            NPError val0 = mChild->NPP_SetWindow(aWindow, aWidth, aHeight);
            *reply = new NPP_ChildToParentMsg_Reply_NPP_SetWindow(val0);
            (*reply)->set_reply();
            return MsgProcessed;
        }

        default:
            return MsgNotKnown;
        }
    }

private:
    NPPProtocol::Child* mChild;
    NPPProtocol::State mState;
    RPCChannel* mRpc;
};


} 
} 

#endif 
