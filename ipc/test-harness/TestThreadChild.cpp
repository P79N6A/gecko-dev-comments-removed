




































#include "TestThreadChild.h"

using mozilla::test::TestThreadChild;
using mozilla::ipc::GeckoThread;

TestThreadChild::TestThreadChild(ProcessHandle aParentHandle) :
    GeckoThread(aParentHandle)
{
}

TestThreadChild::~TestThreadChild()
{
}

void
TestThreadChild::Init()
{
    GeckoThread::Init();
    mChild.Open(channel(), GetParentProcessHandle(), owner_loop());
}

void
TestThreadChild::CleanUp()
{
    GeckoThread::CleanUp();
    mChild.Close();
}
