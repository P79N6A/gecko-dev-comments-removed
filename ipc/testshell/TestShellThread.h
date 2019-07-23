



































#ifndef _IPC_TESTSHELL_TESTSHELLTHREAD_H_
#define _IPC_TESTSHELL_TESTSHELLTHREAD_H_

#include "mozilla/ipc/GeckoThread.h"
#include "TestShellChild.h"

namespace mozilla {
namespace ipc {

class XPCShellEnvironment;

class TestShellThread : public GeckoThread
{
public:
    TestShellThread();
    ~TestShellThread();

protected:
    virtual void Init();
    virtual void CleanUp();

private:
    XPCShellEnvironment* mXPCShell;
    TestShellChild mTestShellChild;
};

} 
} 

#endif 