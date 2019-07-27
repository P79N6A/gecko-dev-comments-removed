






#include "nsDebug.h"

#include "mozilla/ipc/IOThreadChild.h"
#include "mozilla/ipc/ProcessChild.h"

namespace mozilla {
namespace ipc {

ProcessChild* ProcessChild::gProcessChild;

ProcessChild::ProcessChild(ProcessHandle parentHandle)
  : ChildProcess(new IOThreadChild())
  , mUILoop(MessageLoop::current())
  , mParentHandle(parentHandle)
{
  MOZ_ASSERT(mUILoop, "UILoop should be created by now");
  MOZ_ASSERT(!gProcessChild, "should only be one ProcessChild");
  gProcessChild = this;
}

ProcessChild::~ProcessChild()
{
  gProcessChild = nullptr;
}

} 
} 
