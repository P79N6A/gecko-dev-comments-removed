




#ifndef COMPONENTS_REFLECT_H
#define COMPONENTS_REFLECT_H

#include "nsIXPCScriptable.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace reflect {

class Module MOZ_FINAL : public nsIXPCScriptable
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
