






































#ifndef mozilla_ipc_ProtocolUtils_h
#define mozilla_ipc_ProtocolUtils_h 1

#include "base/process.h"
#include "base/process_util.h"
#include "chrome/common/ipc_message_utils.h"

#include "prenv.h"

#include "IPCMessageStart.h"
#include "mozilla/ipc/Shmem.h"
#include "mozilla/ipc/Transport.h"



namespace {






enum {
    CHANNEL_OPENED_MESSAGE_TYPE = kuint16max - 6,
    SHMEM_DESTROYED_MESSAGE_TYPE = kuint16max - 5,
    UNBLOCK_CHILD_MESSAGE_TYPE = kuint16max - 4,
    BLOCK_CHILD_MESSAGE_TYPE   = kuint16max - 3,
    SHMEM_CREATED_MESSAGE_TYPE = kuint16max - 2,
    GOODBYE_MESSAGE_TYPE       = kuint16max - 1,
};
}

namespace mozilla {
namespace ipc {

class AsyncChannel;




struct ActorHandle
{
    int mId;
};






struct Trigger
{
    enum Action { Send, Recv };

    Trigger(Action action, int32 msg) :
        mAction(action),
        mMsg(msg)
    {}

    Action mAction;
    int32 mMsg;
};

template<class ListenerT>
class  IProtocolManager
{
public:
    enum ActorDestroyReason {
        FailedConstructor,
        Deletion,
        AncestorDeletion,
        NormalShutdown,
        AbnormalShutdown
    };

    typedef base::ProcessHandle ProcessHandle;

    virtual int32 Register(ListenerT*) = 0;
    virtual int32 RegisterID(ListenerT*, int32) = 0;
    virtual ListenerT* Lookup(int32) = 0;
    virtual void Unregister(int32) = 0;
    virtual void RemoveManagee(int32, ListenerT*) = 0;

    virtual Shmem::SharedMemory* CreateSharedMemory(
        size_t, SharedMemory::SharedMemoryType, bool, int32*) = 0;
    virtual bool AdoptSharedMemory(Shmem::SharedMemory*, int32*) = 0;
    virtual Shmem::SharedMemory* LookupSharedMemory(int32) = 0;
    virtual bool IsTrackingSharedMemory(Shmem::SharedMemory*) = 0;
    virtual bool DestroySharedMemory(Shmem&) = 0;

    
    virtual ProcessHandle OtherProcess() const = 0;
    virtual AsyncChannel* GetIPCChannel() = 0;
};


inline bool
LoggingEnabled()
{
#if defined(DEBUG)
    return !!PR_GetEnv("MOZ_IPC_MESSAGE_LOG");
#else
    return false;
#endif
}


typedef IPCMessageStart ProtocolId;

struct PrivateIPDLInterface {};

bool
Bridge(const PrivateIPDLInterface&,
       AsyncChannel*, base::ProcessHandle, AsyncChannel*, base::ProcessHandle,
       ProtocolId);

bool
UnpackChannelOpened(const PrivateIPDLInterface&,
                    const IPC::Message&,
                    TransportDescriptor*, base::ProcessId*, ProtocolId*);

} 
} 


namespace IPC {

template <>
struct ParamTraits<mozilla::ipc::ActorHandle>
{
    typedef mozilla::ipc::ActorHandle paramType;

    static void Write(Message* aMsg, const paramType& aParam)
    {
        IPC::WriteParam(aMsg, aParam.mId);
    }

    static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
    {
        int id;
        if (IPC::ReadParam(aMsg, aIter, &id)) {
            aResult->mId = id;
            return true;
        }
        return false;
    }

    static void Log(const paramType& aParam, std::wstring* aLog)
    {
        aLog->append(StringPrintf(L"(%d)", aParam.mId));
    }
};

} 


#endif
