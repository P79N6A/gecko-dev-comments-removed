



































#include "TestShellThread.h"

#include "XPCShellEnvironment.h"

#include "base/message_loop.h"

using mozilla::ipc::TestShellThread;
using mozilla::ipc::XPCShellEnvironment;

TestShellThread::TestShellThread()
: mXPCShell(nsnull)
{

}

TestShellThread::~TestShellThread()
{

}

void
TestShellThread::Init()
{
    GeckoThread::Init();
    mXPCShell = XPCShellEnvironment::CreateEnvironment();
    if (mXPCShell) {
        if (mTestShellChild.Open(channel(), owner_loop())) {
            mTestShellChild.SetXPCShell(mXPCShell);
        }
    }
}

void
TestShellThread::CleanUp()
{
    GeckoThread::CleanUp();
    if (mXPCShell) {
        XPCShellEnvironment::DestroyEnvironment(mXPCShell);
        mTestShellChild.Close();
    }
}
