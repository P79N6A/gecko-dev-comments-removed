





































#ifndef MODULE_H
#define MODULE_H

#include "nsIXPCScriptable.h"

namespace mozilla {
namespace ctypes {

class Module : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

  Module();

  
  JSBool Init(JSContext* aContext, JSObject* aGlobal);

private:
  ~Module();
};

}
}

#endif
