





#include "GMPProcessParent.h"

#include "base/string_util.h"
#include "base/process_util.h"

using std::vector;
using std::string;

using mozilla::gmp::GMPProcessParent;
using mozilla::ipc::GeckoChildProcessHost;
using base::ProcessArchitecture;

template<>
struct RunnableMethodTraits<GMPProcessParent>
{
  static void RetainCallee(GMPProcessParent* obj) { }
  static void ReleaseCallee(GMPProcessParent* obj) { }
};

namespace mozilla {
namespace gmp {

GMPProcessParent::GMPProcessParent(const std::string& aGMPPath)
: GeckoChildProcessHost(GeckoProcessType_GMPlugin),
  mGMPPath(aGMPPath)
{
}

GMPProcessParent::~GMPProcessParent()
{
}

bool
GMPProcessParent::Launch(int32_t aTimeoutMs)
{
  vector<string> args;
  args.push_back(mGMPPath);
  return SyncLaunch(args, aTimeoutMs, base::GetCurrentProcessArchitecture());
}

void
GMPProcessParent::Delete()
{
  MessageLoop* currentLoop = MessageLoop::current();
  MessageLoop* ioLoop = XRE_GetIOMessageLoop();

  if (currentLoop == ioLoop) {
    delete this;
    return;
  }

  ioLoop->PostTask(FROM_HERE, NewRunnableMethod(this, &GMPProcessParent::Delete));
}

} 
} 
