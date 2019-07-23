





































#ifndef dom_plugins_NPAPIProtocol_h
#define dom_plugins_NPAPIProtocol_h 1

#include "npapi.h"

#include "IPC/IPCMessageUtils.h"
#include "mozilla/ipc/MessageTypes.h"

namespace mozilla {
namespace plugins {


class NS_FINAL_CLASS NPAPIProtocol
{
public:
    








     class Parent
    {
    protected:
        Parent() { }
        virtual ~Parent() { }
        Parent(const Parent&);
        Parent& operator=(const Parent&);
    };

    








     class Child
    {
    public:
        virtual NPError NP_Initialize() = 0;

        virtual NPError NPP_New(const mozilla::ipc::String& aMimeType,
                                 const int& aHandle,
                                const uint16_t& aMode,
                                const mozilla::ipc::StringArray& aNames,
                                const mozilla::ipc::StringArray& aValues) = 0;

        virtual void NPP_Destroy() = 0;

    protected:
        Child() { }
        virtual ~Child() { }
        Child(const Child&);
        Child& operator=(const Child&);
    };

    enum State {
        StateStart = 0,

        StateInitializing = StateStart,
        StateInitialized,
        StateNotScriptable,
        StateScriptable,

        
        StateLast
    };

    

private:
    NPAPIProtocol();
    virtual ~NPAPIProtocol() = 0;
    NPAPIProtocol& operator=(const NPAPIProtocol&);
    
};














enum NPAPI_ParentToChildMsgType {
    NPAPI_ParentToChildStart = NPAPI_ParentToChildMsgStart << 12,
    NPAPI_ParentToChildPreStart = (NPAPI_ParentToChildMsgStart << 12) - 1,

    NPAPI_ParentToChildMsg_NP_Initialize__ID,
    NPAPI_ParentToChildMsg_NPP_New__ID,
    NPAPI_ParentToChildMsg_NPP_Destroy__ID,

    NPAPI_ParentToChildEnd
};

class NPAPI_ParentToChildMsg_NP_Initialize : public IPC::Message
{
public:
    enum { ID = NPAPI_ParentToChildMsg_NP_Initialize__ID };
    NPAPI_ParentToChildMsg_NP_Initialize() :
        IPC::Message(MSG_ROUTING_CONTROL, ID, PRIORITY_NORMAL)
    {}
};

class NPAPI_ParentToChildMsg_NPP_New :
        public IPC::MessageWithTuple<
    Tuple5<mozilla::ipc::String, int, uint16_t, mozilla::ipc::StringArray, mozilla::ipc::StringArray> >
{
public:
    enum { ID = NPAPI_ParentToChildMsg_NPP_New__ID };
    NPAPI_ParentToChildMsg_NPP_New(const mozilla::ipc::String& aMimeType,
                                   
                                   const int& aHandle,
                                   const uint16_t& aMode,
                                   const mozilla::ipc::StringArray& aNames,
                                   const mozilla::ipc::StringArray& aValues) :
        IPC::MessageWithTuple<
        Tuple5<mozilla::ipc::String, int, uint16_t, mozilla::ipc::StringArray, mozilla::ipc::StringArray> >(
            MSG_ROUTING_CONTROL, ID, MakeTuple(aMimeType,
                                               aHandle,
                                               aMode,
                                               aNames,
                                               aValues))
    {}
};

class NPAPI_ParentToChildMsg_NPP_Destroy : public IPC::Message {
public:
    enum { ID = NPAPI_ParentToChildMsg_NPP_Destroy__ID };
    NPAPI_ParentToChildMsg_NPP_Destroy() :
        IPC::Message(MSG_ROUTING_CONTROL, ID, PRIORITY_NORMAL)
    {}
};



enum NPAPI_ChildToParentMsgType {
    NPAPI_ChildToParentStart = NPAPI_ChildToParentMsgStart << 12,
    NPAPI_ChildToParentPreStart = (NPAPI_ChildToParentMsgStart << 12) - 1,

    NPAPI_ChildToParentMsg_Reply_NP_Initialize__ID,

    NPAPI_ChildToParentMsg_Reply_NPP_New__ID,
    NPAPI_ChildToParentMsg_Reply_NPP_Destroy__ID,

    NPAPI_ChildToParentEnd
};

class NPAPI_ChildToParentMsg_Reply_NP_Initialize :
        public IPC::MessageWithTuple<NPError>
{
public:
    enum { ID = NPAPI_ChildToParentMsg_Reply_NP_Initialize__ID };
    NPAPI_ChildToParentMsg_Reply_NP_Initialize(const NPError& arg1) :
        IPC::MessageWithTuple<NPError>(MSG_ROUTING_CONTROL, ID, arg1)
    {}
};

class NPAPI_ChildToParentMsg_Reply_NPP_New :
        public IPC::MessageWithTuple<NPError>
{
public:
    enum { ID = NPAPI_ChildToParentMsg_Reply_NPP_New__ID };
    NPAPI_ChildToParentMsg_Reply_NPP_New(const NPError& arg1) :
        IPC::MessageWithTuple<NPError>(MSG_ROUTING_CONTROL, ID, arg1)
    {}
};

class NPAPI_ChildToParentMsg_Reply_NPP_Destroy : public IPC::Message
{
public:
    enum { ID = NPAPI_ChildToParentMsg_Reply_NPP_Destroy__ID };
    NPAPI_ChildToParentMsg_Reply_NPP_Destroy()
        : IPC::Message(MSG_ROUTING_CONTROL, ID, PRIORITY_NORMAL)
    {}
};


#if 0

class FOO_5PARAMS :
        public IPC::MessageWithTuple<
    Tuple5<NPError, int, bool, std::string, short> >
{
public:
    enum { ID = FOO_5PARAMS__ID };
    FOO_5PARAMS(const NPError& arg1,
                const int& arg2,
                const bool& arg3,
                const std::string& arg4,
                const short& arg5) :
        IPC::MessageWithTuple<
        Tuple5<NPError, int, bool, std::string, short> >(
            MSG_ROUTING_CONTROL, ID, MakeTuple(arg1, arg2, arg3, arg4, arg5))
    {}
};
#endif


} 
} 


#endif
