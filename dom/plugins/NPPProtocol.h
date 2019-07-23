





































#ifndef dom_plugins_NPPProtocol_h
#define dom_plugins_NPPProtocol_h 1


#include "IPC/IPCMessageUtils.h"
#include "mozilla/ipc/MessageTypes.h"


#include "npapi.h"
#include "X11/X.h"

namespace mozilla {
namespace plugins {


class NS_FINAL_CLASS NPPProtocol
{
public:
     class Parent
    {
    public:
        virtual void NPN_GetValue() = 0;

    protected:
        Parent() { }
        virtual ~Parent() { }
        Parent(const Parent&);
        Parent& operator=(const Parent&);
    };

     class Child
    {
    public:
        virtual NPError NPP_SetWindow(XID aWindow,
                                      int32_t aWidth,
                                      int32_t aHeight) = 0;

    protected:
        Child() { }
        virtual ~Child() { }
        Child(const Child&);
        Child& operator=(const Child&);
    };

    enum State {
        StateStart = 0,


        
        StateLast
    };

private:
    NPPProtocol();
    virtual ~NPPProtocol() = 0;
    NPPProtocol& operator=(const NPPProtocol&);
    
};






enum NPP_ParentToChildMsgType {
    NPP_ParentToChildStart = NPP_ParentToChildMsgStart << 12,
    NPP_ParentToChildPreStart = (NPP_ParentToChildMsgStart << 12) - 1,

    NPP_ParentToChildMsg_NPP_SetWindow__ID,

    NPP_ParentToChildMsg_Reply_NPN_GetValue__ID,

    NPP_ParentToChildEnd
};

class NPP_ParentToChildMsg_NPP_SetWindow :
        public IPC::MessageWithTuple<
    Tuple3<XID, int32_t, int32_t> >
{
public:
    enum { ID = NPP_ParentToChildMsg_NPP_SetWindow__ID };
    NPP_ParentToChildMsg_NPP_SetWindow(const XID& window,
                                       const int32_t& width,
                                       const int32_t& height) :
        IPC::MessageWithTuple<
        Tuple3<XID, int32_t, int32_t> >(
            MSG_ROUTING_CONTROL, ID, MakeTuple(window, width, height))
    {}
};

class NPP_ParentToChildMsg_Reply_NPN_GetValue : public IPC::Message
{
public:
    enum { ID = NPP_ParentToChildMsg_Reply_NPN_GetValue__ID };
    NPP_ParentToChildMsg_Reply_NPN_GetValue() :
        IPC::Message(MSG_ROUTING_CONTROL, ID, PRIORITY_NORMAL)
    {}
};



enum NPP_ChildToParentMsgType {
    NPP_ChildToParentStart = NPP_ChildToParentMsgStart << 12,
    NPP_ChildToParentPreStart = (NPP_ChildToParentMsgStart << 12) - 1,

    NPP_ChildToParentMsg_NPN_GetValue__ID,

    NPP_ChildToParentMsg_Reply_NPP_SetWindow__ID,

    NPP_ChildToParentEnd
};

class NPP_ChildToParentMsg_NPN_GetValue : public IPC::Message
{
public:
    enum { ID = NPP_ChildToParentMsg_NPN_GetValue__ID };
    NPP_ChildToParentMsg_NPN_GetValue() :
        IPC::Message(MSG_ROUTING_CONTROL, ID, PRIORITY_NORMAL)
    {}
};

class NPP_ChildToParentMsg_Reply_NPP_SetWindow :
        public IPC::MessageWithTuple<NPError>
{
public:
    enum { ID = NPP_ChildToParentMsg_Reply_NPP_SetWindow__ID };
    NPP_ChildToParentMsg_Reply_NPP_SetWindow(const NPError& arg1) :
        IPC::MessageWithTuple<NPError>(MSG_ROUTING_CONTROL, ID, arg1)
    {}
};


} 
} 

#endif
