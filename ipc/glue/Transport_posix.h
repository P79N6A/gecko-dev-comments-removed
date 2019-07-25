







































#ifndef mozilla_ipc_Transport_posix_h
#define mozilla_ipc_Transport_posix_h 1

#include "IPC/IPCMessageUtils.h"


namespace mozilla {
namespace ipc {

struct TransportDescriptor
{
  base::FileDescriptor mFd;
};

} 
} 


namespace IPC {

template<>
struct ParamTraits<mozilla::ipc::TransportDescriptor>
{
  typedef mozilla::ipc::TransportDescriptor paramType;
  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mFd);
  }
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return ReadParam(aMsg, aIter, &aResult->mFd);
  }
};

} 


#endif  
