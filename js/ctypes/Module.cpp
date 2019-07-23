






































#include "nsIGenericFactory.h"
#include "Library.h"

namespace mozilla {
namespace ctypes {

NS_GENERIC_FACTORY_CONSTRUCTOR(Library)

}
}

static nsModuleComponentInfo components[] =
{
  {
    "jsctypes",
    FOREIGNLIBRARY_CID,
    FOREIGNLIBRARY_CONTRACTID,
    mozilla::ctypes::LibraryConstructor,
  }
};

NS_IMPL_NSGETMODULE(jsctypes, components)

