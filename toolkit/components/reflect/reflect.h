





































#ifndef COMPONENTS_REFLECT_H
#define COMPONENTS_REFLECT_H

#include "nsIXPCScriptable.h"

namespace mozilla {
namespace reflect {

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
