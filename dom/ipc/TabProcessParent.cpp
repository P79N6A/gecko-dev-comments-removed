


#include "TabProcessParent.h"

using mozilla::ipc::GeckoChildProcessHost;

namespace mozilla {
namespace tabs {


TabProcessParent::TabProcessParent() :
    GeckoChildProcessHost(GeckoChildProcess_Tab)
{
}

TabProcessParent::~TabProcessParent()
{
}


} 
} 
