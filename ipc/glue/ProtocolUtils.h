






































#ifndef mozilla_ipc_ProtocolUtils_h
#define mozilla_ipc_ProtocolUtils_h 1

#include "base/process.h"
#include "base/process_util.h"
#include "chrome/common/ipc_message_utils.h"

#include "prenv.h"

#include "mozilla/ipc/Shmem.h"



namespace {
enum {
    SHMEM_CREATED_MESSAGE_TYPE = kuint16max - 2,
    GOODBYE_MESSAGE_TYPE       = kuint16max - 1,
};
}

namespace mozilla {
namespace ipc {





struct ActorHandle
{
    int mId;
};

template<class ListenerT>
class  IProtocolManager
{
public:
    enum ActorDestroyReason {
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
    
    virtual ProcessHandle OtherProcess() const = 0;
};




class __internal__ipdl__ShmemCreated : public IPC::Message
{
private:
    typedef Shmem::id_t id_t;
    typedef Shmem::SharedMemoryHandle SharedMemoryHandle;

public:
    enum { ID = SHMEM_CREATED_MESSAGE_TYPE };

    __internal__ipdl__ShmemCreated(
        int32 routingId,
        const SharedMemoryHandle& aHandle,
        const id_t& aIPDLId,
        const size_t& aSize) :
        IPC::Message(routingId, ID, PRIORITY_NORMAL)
    {
        IPC::WriteParam(this, aHandle);
        IPC::WriteParam(this, aIPDLId);
        IPC::WriteParam(this, aSize);
    }

    static bool Read(const Message* msg,
                     SharedMemoryHandle* aHandle,
                     id_t* aIPDLId,
                     size_t* aSize)
    {
        void* iter = 0;
        if (!IPC::ReadParam(msg, &iter, aHandle))
            return false;
        if (!IPC::ReadParam(msg, &iter, aIPDLId))
            return false;
        if (!IPC::ReadParam(msg, &iter, aSize))
            return false;
        msg->EndRead(iter);
        return true;
    }

    void Log(const std::string& aPrefix,
             FILE* aOutf) const
    {
        fputs("(special ShmemCreated msg)", aOutf);
    }
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
