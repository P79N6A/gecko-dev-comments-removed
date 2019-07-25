




































#include "mozilla/jetpack/JetpackProcessParent.h"

#include "mozilla/ipc/BrowserProcessSubThread.h"

namespace mozilla {
namespace jetpack {

JetpackProcessParent::JetpackProcessParent()
  : mozilla::ipc::GeckoChildProcessHost(GeckoProcessType_Jetpack)
{
}

JetpackProcessParent::~JetpackProcessParent()
{
}

void
JetpackProcessParent::Launch()
{
  AsyncLaunch();
}

} 
} 
