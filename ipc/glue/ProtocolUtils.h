



































#ifndef mozilla_ipc_ProtocolUtils_h
#define mozilla_ipc_ProtocolUtils_h 1

#include "chrome/common/ipc_message_utils.h"

#include "mozilla/ipc/RPCChannel.h"

namespace mozilla {
namespace ipc {




struct ActorHandle
{
    int mParentId;
    int mChildId;
};


class  IProtocolManager
{
public:
    virtual int32 Register(RPCChannel::Listener*) = 0;
    RPCChannel::Listener* Lookup(int32) = 0;
    virtual void Unregister(int32) = 0;
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
    IPC::WriteParam(aMsg, aParam.mParentId);
    IPC::WriteParam(aMsg, aParam.mChildId);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    int parentId, childId;
    if (IPC::ReadParam(aMsg, aIter, &parentId)
        && ReadParam(aMsg, aIter, &childId)) {
      aResult->mParentId = parentId;
      aResult->mChildId = childId;
      return true;
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(StringPrintf(L"(%d,%d)", aParam.mParentId, aParam.mChildId));
  }
};

} 


#endif  
