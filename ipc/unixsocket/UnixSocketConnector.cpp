





#include "UnixSocketConnector.h"
#include "nsISupportsImpl.h" 

namespace mozilla {
namespace ipc {

UnixSocketConnector::UnixSocketConnector()
{
  MOZ_COUNT_CTOR(UnixSocketConnector);
}

UnixSocketConnector::~UnixSocketConnector()
{
  MOZ_COUNT_DTOR(UnixSocketConnector);
}

}
}
