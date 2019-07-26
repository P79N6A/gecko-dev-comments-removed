






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
  NS_ABORT_IF_FALSE(mUILoop, "UILoop should be created by now");
  NS_ABORT_IF_FALSE(!gProcessChild, "should only be one ProcessChild");
  gProcessChild = this;
}

ProcessChild::~ProcessChild()
{
  gProcessChild = nullptr;
}

} 
} 
