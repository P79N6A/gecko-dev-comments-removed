





































#ifndef MODULE_H
#define MODULE_H

#include "nsIXPCScriptable.h"

namespace mozilla {
namespace ctypes {




enum ABICode {
#define DEFINE_ABI(name) ABI_##name,
#define DEFINE_TYPE(name)
#include "types.h"
#undef DEFINE_ABI
#undef DEFINE_TYPE
  INVALID_ABI
};

enum TypeCode {
#define DEFINE_ABI(name)
#define DEFINE_TYPE(name) TYPE_##name,
#include "types.h"
#undef DEFINE_ABI
#undef DEFINE_TYPE
  INVALID_TYPE
};

class Module : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

  Module();

  
  bool Init(JSContext* aContext, JSObject* aGlobal);

  static ABICode GetABICode(JSContext* cx, jsval val);
  static TypeCode GetTypeCode(JSContext* cx, jsval val);

private:
  ~Module();
};

}
}

#endif
