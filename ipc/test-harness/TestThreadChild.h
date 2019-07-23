




































#ifndef ipc_test_harness_TestThreadChild_h
#define ipc_test_harness_TestThreadChild_h 1

#include "mozilla/ipc/GeckoThread.h"
#include "mozilla/test/TestChild.h"

namespace mozilla {
namespace test {

class TestThreadChild : public mozilla::ipc::GeckoThread
{
public:
    TestThreadChild();
    ~TestThreadChild();

protected:
    virtual void Init();
    virtual void CleanUp();

private:
    TestChild mChild;
};

} 
} 

#endif 
