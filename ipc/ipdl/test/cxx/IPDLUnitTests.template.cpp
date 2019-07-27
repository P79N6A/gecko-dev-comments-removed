



#include <stdlib.h>
#include <string.h>

#include "IPDLUnitTests.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "base/thread.h"

#include "nsRegion.h"

#include "IPDLUnitTestSubprocess.h"



${INCLUDES}


using namespace std;

using base::Thread;

namespace mozilla {
namespace _ipdltest {

void* gParentActor;
IPDLUnitTestSubprocess* gSubprocess;

void* gChildActor;



Thread* gChildThread;
MessageLoop *gParentMessageLoop;
bool gParentDone;
bool gChildDone;

void
DeleteChildActor();




char* gIPDLUnitTestName = nullptr;

const char* const
IPDLUnitTestName()
{
    if (!gIPDLUnitTestName) {
#if defined(OS_WIN)
        vector<wstring> args =
            CommandLine::ForCurrentProcess()->GetLooseValues();
        gIPDLUnitTestName = ::strdup(WideToUTF8(args[0]).c_str());
#elif defined(OS_POSIX)
        vector<string> argv = CommandLine::ForCurrentProcess()->argv();
        gIPDLUnitTestName = ::moz_xstrdup(argv[1].c_str());
#else
#  error Sorry
#endif
    }
    return gIPDLUnitTestName;
}

} 
} 


namespace {

enum IPDLUnitTestType {
    NoneTest = 0,



${ENUM_VALUES}
    
    LastTest = ${LAST_ENUM}

};


IPDLUnitTestType
IPDLUnitTestFromString(const char* const aString)
{
    if (!aString)
        return static_cast<IPDLUnitTestType>(0);


${STRING_TO_ENUMS}

    else
        return static_cast<IPDLUnitTestType>(0);
}


const char* const
IPDLUnitTestToString(IPDLUnitTestType aTest)
{
    switch (aTest) {


${ENUM_TO_STRINGS}


    default:
        return nullptr;
    }
}


IPDLUnitTestType
IPDLUnitTest()
{
    return IPDLUnitTestFromString(::mozilla::_ipdltest::IPDLUnitTestName());
}


} 





namespace mozilla {
namespace _ipdltest {

void
DeferredParentShutdown();

void
IPDLUnitTestThreadMain(char *testString);

void
IPDLUnitTestMain(void* aData)
{
    char* testString = reinterpret_cast<char*>(aData);

    
    const char *prefix = "thread:";
    const int prefixLen = strlen(prefix);
    if (!strncmp(testString, prefix, prefixLen)) {
        IPDLUnitTestThreadMain(testString + prefixLen);
        return;
    }

    IPDLUnitTestType test = IPDLUnitTestFromString(testString);
    if (!test) {
        
        fprintf(stderr, MOZ_IPDL_TESTFAIL_LABEL "| %s | unknown unit test %s\n",
                "<--->", testString);
        NS_RUNTIMEABORT("can't continue");
    }
    gIPDLUnitTestName = testString;

    
    switch (test) {


${PARENT_ENABLED_CASES_PROC}


    default:
        fail("not reached");
        return;                 
    }

    printf(MOZ_IPDL_TESTINFO_LABEL "| running test | %s\n", gIPDLUnitTestName);

    std::vector<std::string> testCaseArgs;
    testCaseArgs.push_back(testString);

    gSubprocess = new IPDLUnitTestSubprocess();
    if (!gSubprocess->SyncLaunch(testCaseArgs))
        fail("problem launching subprocess");

    IPC::Channel* transport = gSubprocess->GetChannel();
    if (!transport)
        fail("no transport");

    base::ProcessId child = base::GetProcId(gSubprocess->GetChildProcessHandle());

    switch (test) {


${PARENT_MAIN_CASES_PROC}


    default:
        fail("not reached");
        return;                 
    }
}

void
IPDLUnitTestThreadMain(char *testString)
{
    IPDLUnitTestType test = IPDLUnitTestFromString(testString);
    if (!test) {
        
        fprintf(stderr, MOZ_IPDL_TESTFAIL_LABEL "| %s | unknown unit test %s\n",
                "<--->", testString);
        NS_RUNTIMEABORT("can't continue");
    }
    gIPDLUnitTestName = testString;

    
    switch (test) {


${PARENT_ENABLED_CASES_THREAD}


    default:
        fail("not reached");
        return;                 
    }

    printf(MOZ_IPDL_TESTINFO_LABEL "| running test | %s\n", gIPDLUnitTestName);

    std::vector<std::string> testCaseArgs;
    testCaseArgs.push_back(testString);

    gChildThread = new Thread("ParentThread");
    if (!gChildThread->Start())
        fail("starting parent thread");

    gParentMessageLoop = MessageLoop::current();
    MessageLoop *childMessageLoop = gChildThread->message_loop();

    switch (test) {


${PARENT_MAIN_CASES_THREAD}


    default:
        fail("not reached");
        return;                 
    }
}

void
DeleteParentActor()
{
    if (!gParentActor)
        return;

    switch (IPDLUnitTest()) {


${PARENT_DELETE_CASES}

    default:  ::mozilla::_ipdltest::fail("???");
    }
}

void
QuitXPCOM()
{
  DeleteParentActor();

  static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);
  nsCOMPtr<nsIAppShell> appShell (do_GetService(kAppShellCID));
  appShell->Exit();
}

void
DeleteSubprocess(MessageLoop* uiLoop)
{
  
  delete gSubprocess;
  uiLoop->PostTask(FROM_HERE, NewRunnableFunction(QuitXPCOM));
}

void
DeferredParentShutdown()
{
    
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(DeleteSubprocess, MessageLoop::current()));
}

void 
TryThreadedShutdown()
{
    
    
    
    
    
    
    if (!gChildDone || !gParentDone || !gChildThread)
        return;

    delete gChildThread;
    gChildThread = 0;
    DeferredParentShutdown();
}

void 
ChildCompleted()
{
    
    gChildDone = true;
    TryThreadedShutdown();
}

void
QuitParent()
{
    if (gChildThread) {
        gParentDone = true;
        MessageLoop::current()->PostTask(
            FROM_HERE, NewRunnableFunction(TryThreadedShutdown));
    } else {
        
        
        MessageLoop::current()->PostTask(
            FROM_HERE, NewRunnableFunction(DeferredParentShutdown));
    }
}

static void
ChildDie()
{
    DeleteChildActor();
    XRE_ShutdownChildProcess();
}

void
QuitChild()
{
    if (gChildThread) { 
        gParentMessageLoop->PostTask(
            FROM_HERE, NewRunnableFunction(ChildCompleted));
    } else { 
        MessageLoop::current()->PostTask(
            FROM_HERE, NewRunnableFunction(ChildDie));
    }
}

} 
} 





namespace mozilla {
namespace _ipdltest {

void
DeleteChildActor()
{
    if (!gChildActor)
        return;

    switch (IPDLUnitTest()) {


${CHILD_DELETE_CASES}

    default:  ::mozilla::_ipdltest::fail("???");
    }
}

void
IPDLUnitTestChildInit(IPC::Channel* transport,
                      base::ProcessId parentPid,
                      MessageLoop* worker)
{
    switch (IPDLUnitTest()) {


${CHILD_INIT_CASES}


    default:
        fail("not reached");
        return;                 
    }
}

} 
} 
