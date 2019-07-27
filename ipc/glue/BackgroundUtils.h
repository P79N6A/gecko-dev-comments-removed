



#ifndef mozilla_ipc_backgroundutils_h__
#define mozilla_ipc_backgroundutils_h__

#include "ipc/IPCMessageUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/BasePrincipal.h"
#include "nsCOMPtr.h"
#include "nscore.h"

class nsIPrincipal;

namespace IPC {

template<>
struct ParamTraits<mozilla::OriginAttributes>
{
  typedef mozilla::OriginAttributes paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    nsAutoCString suffix;
    aParam.CreateSuffix(suffix);
    WriteParam(aMsg, suffix);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    nsAutoCString suffix;
    return ReadParam(aMsg, aIter, &suffix) &&
           aResult->PopulateFromSuffix(suffix);
  }
};

} 

namespace mozilla {
namespace ipc {

class PrincipalInfo;






already_AddRefed<nsIPrincipal>
PrincipalInfoToPrincipal(const PrincipalInfo& aPrincipalInfo,
                         nsresult* aOptionalResult = nullptr);






nsresult
PrincipalToPrincipalInfo(nsIPrincipal* aPrincipal,
                         PrincipalInfo* aPrincipalInfo);

} 
} 

#endif 
