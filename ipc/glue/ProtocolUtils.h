






































#ifndef mozilla_ipc_ProtocolUtils_h
#define mozilla_ipc_ProtocolUtils_h 1

#include "base/process.h"
#include "chrome/common/ipc_message_utils.h"

#include "mozilla/ipc/RPCChannel.h"

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
    typedef base::ProcessHandle ProcessHandle;

    virtual int32 Register(ListenerT*) = 0;
    virtual int32 RegisterID(ListenerT*, int32) = 0;
    virtual ListenerT* Lookup(int32) = 0;
    virtual void Unregister(int32) = 0;
    
    virtual ProcessHandle OtherProcess() = 0;
};

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
