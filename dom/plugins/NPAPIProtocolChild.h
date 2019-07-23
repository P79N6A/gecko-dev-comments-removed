





































#ifndef dom_plugins_NPAPIProtocolChildImpl_h
#define dom_plugins_NPAPIProtocolChildImpl_h 1

#include "chrome/common/ipc_channel.h"
#include "chrome/common/ipc_message.h"

#include "mozilla/ipc/RPCChannel.h"
#include "mozilla/plugins/NPAPIProtocol.h"


#include "mozilla/plugins/NPPProtocolChild.h"


#ifdef DEBUG
#  undef _MSG_LOG0
#  undef _MSG_LOG

#  define _MSG_LOG0(fmt, ...)                                       \
    do {                                                            \
        printf("[NPAPIProtocolChild] %s "fmt"\n", __VA_ARGS__);     \
    } while(0)

#  define _MSG_LOG(fmt, ...)                    \
    _MSG_LOG0(fmt, __FUNCTION__,## __VA_ARGS__)
#else
#  define _MSG_LOG(fmt, ...)
#endif

namespace mozilla {
namespace plugins {







class NS_FINAL_CLASS NPAPIProtocolChild : 
        public NPAPIProtocol::Parent,
        public mozilla::ipc::RPCChannel::Listener
{
private:
    typedef IPC::Message Message;
    typedef mozilla::ipc::RPCChannel RPCChannel;

public:
    NPAPIProtocolChild(NPAPIProtocol::Child* aChild) :
        mChild(aChild),
        mRpc(this)
    {
    }

    ~NPAPIProtocolChild()
    {
    }

    bool Open(IPC::Channel* aChannel, MessageLoop* aIOLoop)
    {
        return mRpc.Open(aChannel, aIOLoop);
    }

    void Close()
    {
        mRpc.Close();
    }

    void NextState(NPAPIProtocol::State aNextState)
    {
        
    }

    
    virtual void NPN_GetValue()
    {
        _MSG_LOG("outcall NPN_GetValue()");
        
    }
    

    
    virtual Result OnCallReceived(const Message& msg, Message** reply)
    {
        switch(msg.type()) {
        case NPAPI_ParentToChildMsg_NP_Initialize__ID: {
            _MSG_LOG("incall NP_Initialize()");

            NPError val0 = mChild->NP_Initialize();
            *reply = new NPAPI_ChildToParentMsg_Reply_NP_Initialize(val0);
            (*reply)->set_reply();
            return MsgProcessed;
        }

        case NPAPI_ParentToChildMsg_NPP_New__ID: {
            mozilla::ipc::String aMimeType;
            int aHandle;
            uint16_t aMode;
            mozilla::ipc::StringArray aNames;
            mozilla::ipc::StringArray aValues;
            
            NPAPI_ParentToChildMsg_NPP_New::Param p;
            NS_ASSERTION(NPAPI_ParentToChildMsg_NPP_New::Read(&msg, &p),
                         "bad types in NPP_New() msg");
            aMimeType = p.a;
            aHandle = p.b;
            aMode = p.c;
            aNames = p.d;
            aValues = p.e;

            _MSG_LOG("incall NPP_New(%s, %d, %d, <vec>, <vec>)",
                     aMimeType.c_str(), aHandle, aMode);

            NPError val0 = mChild->NPP_New(aMimeType,
                                           aHandle,
                                           aMode,
                                           aNames,
                                           aValues);
            *reply = new NPAPI_ChildToParentMsg_Reply_NPP_New(val0);
            (*reply)->set_reply();
            return MsgProcessed;
        }

        default:
            
            return HACK_npp->OnCallReceived(msg, reply);
            
            

        }
    }

    
    RPCChannel* HACK_getchannel_please() { return &mRpc; }
    NPPProtocolChild* HACK_npp;
    


private:
    NPAPIProtocol::Child* mChild;
    mozilla::ipc::RPCChannel mRpc;
};


} 
} 

#endif  
