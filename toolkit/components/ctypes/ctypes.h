





































#ifndef COMPONENTS_CTYPES_H
#define COMPONENTS_CTYPES_H

#include "nsIXPCScriptable.h"

namespace mozilla {
namespace ctypes {

class Module : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

  Module();

private:
  ~Module();
};

}
}

#endif
