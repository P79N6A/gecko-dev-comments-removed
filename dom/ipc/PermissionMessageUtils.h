




#ifndef mozilla_dom_permission_message_utils_h__
#define mozilla_dom_permission_message_utils_h__

#include "ipc/IPCMessageUtils.h"
#include "nsCOMPtr.h"
#include "nsIPrincipal.h"

namespace IPC {

class Principal {
  friend struct ParamTraits<Principal>;

public:
  Principal() : mPrincipal(nullptr) {}
  explicit Principal(nsIPrincipal* aPrincipal) : mPrincipal(aPrincipal) {}
  operator nsIPrincipal*() const { return mPrincipal.get(); }

private:
  
  Principal& operator=(Principal&);
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

template <>
struct ParamTraits<Principal>
{
  typedef Principal paramType;
  static void Write(Message* aMsg, const paramType& aParam);
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult);
};

} 

#endif 

