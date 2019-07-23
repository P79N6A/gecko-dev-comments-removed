



































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


template<class ListenerT>
class  IProtocolManager
{
public:
    virtual int32 Register(ListenerT*) = 0;
    virtual ListenerT* Lookup(int32) = 0;
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
