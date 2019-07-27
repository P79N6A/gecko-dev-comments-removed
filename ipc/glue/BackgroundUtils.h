



#ifndef mozilla_ipc_backgroundutils_h__
#define mozilla_ipc_backgroundutils_h__

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nscore.h"

class nsIPrincipal;

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
