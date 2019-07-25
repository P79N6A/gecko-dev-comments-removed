







































#ifndef mozilla_ipc_Transport_win_h
#define mozilla_ipc_Transport_win_h 1

#include <string>

#include "IPC/IPCMessageUtils.h"


namespace mozilla {
namespace ipc {

struct TransportDescriptor
{
  std::wstring mPipeName;
  HANDLE mServerPipe;
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
    WriteParam(aMsg, aParam.mPipeName);
    WriteParam(aMsg, aParam.mServerPipe);
  }
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return (ReadParam(aMsg, aIter, &aResult->mPipeName) &&
            ReadParam(aMsg, aIter, &aResult->mServerPipe));
  }
};

} 


#endif  
