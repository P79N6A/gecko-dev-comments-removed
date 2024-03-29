#include "TestSyncHang.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "IPDLUnitTests.h"      

using std::vector;
using std::string;

namespace mozilla {
namespace _ipdltest {




mozilla::ipc::GeckoChildProcessHost* gSyncHangSubprocess;

TestSyncHangParent::TestSyncHangParent()
{
    MOZ_COUNT_CTOR(TestSyncHangParent);
}

TestSyncHangParent::~TestSyncHangParent()
{
    MOZ_COUNT_DTOR(TestSyncHangParent);
}

void
DeleteSyncHangSubprocess(MessageLoop* uiLoop)
{
  delete gSyncHangSubprocess;
}

void
DeferredSyncHangParentShutdown()
{
  
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(DeleteSyncHangSubprocess, MessageLoop::current()));
}

void
TestSyncHangParent::Main()
{
  vector<string> args;
  args.push_back("fake/path");
  gSyncHangSubprocess = new mozilla::ipc::GeckoChildProcessHost(GeckoProcessType_Plugin);
  bool launched = gSyncHangSubprocess->SyncLaunch(args, 2);
  if (launched)
    fail("Calling SyncLaunch with an invalid path should return false");

  MessageLoop::current()->PostTask(
  				   FROM_HERE, NewRunnableFunction(DeferredSyncHangParentShutdown));
  Close();
}




TestSyncHangChild::TestSyncHangChild()
{
    MOZ_COUNT_CTOR(TestSyncHangChild);
}

TestSyncHangChild::~TestSyncHangChild()
{
    MOZ_COUNT_DTOR(TestSyncHangChild);
}

} 
} 
