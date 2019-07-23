





































#ifndef mozilla__ipdltest_IPDLUnitTests_h
#define mozilla__ipdltest_IPDLUnitTests_h 1

#include "base/message_loop.h"
#include "chrome/common/ipc_channel.h"


#define MOZ_IPDL_TESTFAIL_LABEL "TEST-UNEXPECTED-FAIL"
#define MOZ_IPDL_TESTPASS_LABEL "TEST-PASS"





#define fail(fmt, ...)                                                  \
    do {                                                                \
        fprintf(stderr, MOZ_IPDL_TESTFAIL_LABEL " | %s | " fmt "\n",    \
                IPDLUnitTestName(), ## __VA_ARGS__);                    \
        NS_RUNTIMEABORT("failed test");                                 \
    } while (0)

#define passed(fmt, ...)                                                \
    fprintf(stderr, MOZ_IPDL_TESTPASS_LABEL " | %s | " fmt "\n",        \
            IPDLUnitTestName(), ## __VA_ARGS__)


namespace mozilla {
namespace _ipdltest {




const char* const IPDLUnitTestName();





void IPDLUnitTestMain(void* aData);







void IPDLUnitTestChildInit(IPC::Channel* transport, MessageLoop* worker);
void IPDLUnitTestChildCleanUp();


} 
} 


#endif 
