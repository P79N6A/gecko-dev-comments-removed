




































#ifndef ipc_test_harness_TestProcessParent_h
#define ipc_test_harness_TestProcessParent_h 1

#include "mozilla/ipc/GeckoChildProcessHost.h"

namespace mozilla {
namespace test {


class TestProcessParent : public mozilla::ipc::GeckoChildProcessHost
{
public:
    TestProcessParent();
    ~TestProcessParent();

    


    
    

private:
    DISALLOW_EVIL_CONSTRUCTORS(TestProcessParent);
};


} 
} 

#endif 
