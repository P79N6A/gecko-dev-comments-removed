



#include "IPDLUnitTests.h"

#include <string.h>

#include "base/command_line.h"
#include "base/string_util.h"

#include "IPDLUnitTestSubprocess.h"
#include "IPDLUnitTestThreadChild.h"



${INCLUDES}


using mozilla::_ipdltest::IPDLUnitTestSubprocess;
using mozilla::_ipdltest::IPDLUnitTestThreadChild;




namespace {
char* gIPDLUnitTestName = NULL; 
}


namespace mozilla {
namespace _ipdltest {

const char* const
IPDLUnitTestName()
{
    if (!gIPDLUnitTestName) {
        std::vector<std::wstring> args =
            CommandLine::ForCurrentProcess()->GetLooseValues();
        gIPDLUnitTestName = strdup(WideToUTF8(args[
#ifndef OS_WIN
args.size()-1
#else
args.size()-2
#endif
						   ]).c_str());
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
        return NULL;
    }
}


IPDLUnitTestType
IPDLUnitTest()
{
    return IPDLUnitTestFromString(mozilla::_ipdltest::IPDLUnitTestName());
}


} 





namespace {
void* gParentActor = NULL;
}


namespace mozilla {
namespace _ipdltest {

void
IPDLUnitTestMain(void* aData)
{
    char* testString = reinterpret_cast<char*>(aData);
    IPDLUnitTestType test = IPDLUnitTestFromString(testString);
    if (!test) {
        
        fprintf(stderr, MOZ_IPDL_TESTFAIL_LABEL "| %s | unknown unit test %s\\n",
                "<--->", testString);
        NS_RUNTIMEABORT("can't continue");
    }
    gIPDLUnitTestName = testString;

    std::wstring testWString = UTF8ToWide(testString);
    std::vector<std::wstring> testCaseArgs;
    testCaseArgs.push_back(testWString);

    IPDLUnitTestSubprocess* subprocess = new IPDLUnitTestSubprocess();
    if (!subprocess->SyncLaunch(testCaseArgs))
        fail("problem launching subprocess");

    IPC::Channel* transport = subprocess->GetChannel();
    if (!transport)
        fail("no transport");

    switch (test) {


${PARENT_MAIN_CASES}


    default:
        fail("not reached");
        return;                 
    }
}

} 
} 





namespace {
void* gChildActor = NULL;
}


namespace mozilla {
namespace _ipdltest {

void
IPDLUnitTestChildInit(IPC::Channel* transport, MessageLoop* worker)
{
    switch (IPDLUnitTest()) {


${CHILD_INIT_CASES}


    default:
        fail("not reached");
        return;                 
    }
}

void IPDLUnitTestChildCleanUp()
{
    if (!gChildActor)
        return;

    switch (IPDLUnitTest()) {


${CHILD_CLEANUP_CASES}


    default:
        fail("not reached");
        return;                 
    }
}

} 
} 
