#include "TestRPC.h"

#include "IPDLUnitTests.h"      
#if defined(OS_POSIX)
#include <unistd.h>
#else
#include <windows.h>
#endif

namespace mozilla {
namespace _ipdltest {




TestRPCParent::TestRPCParent()
 : reentered_(false),
   resolved_first_cpow_(false)
{
  MOZ_COUNT_CTOR(TestRPCParent);
}

TestRPCParent::~TestRPCParent()
{
  MOZ_COUNT_DTOR(TestRPCParent);
}

void
TestRPCParent::Main()
{
  if (!SendStart())
    fail("sending Start");
}

bool
TestRPCParent::RecvTest1_Start(uint32_t* aResult)
{
  uint32_t result;
  if (!SendTest1_InnerQuery(&result))
    fail("SendTest1_InnerQuery");
  if (result != 300)
    fail("Wrong result (expected 300)");

  *aResult = 100;
  return true;
}

bool
TestRPCParent::RecvTest1_InnerEvent(uint32_t* aResult)
{
  uint32_t result;
  if (!SendTest1_NoReenter(&result))
    fail("SendTest1_NoReenter");
  if (result != 400)
    fail("Wrong result (expected 400)");

  *aResult = 200;
  return true;
}

bool
TestRPCParent::RecvTest2_Start()
{
  
  
  if (!SendTest2_FirstUrgent())
    fail("SendTest2_FirstUrgent");

  MOZ_ASSERT(!reentered_);
  resolved_first_cpow_ = true;
  return true;
}

bool
TestRPCParent::RecvTest2_OutOfOrder()
{
  
  
  if (!SendTest2_SecondUrgent())
    fail("SendTest2_SecondUrgent");

  reentered_ = true;
  return true;
}





TestRPCChild::TestRPCChild()
{
    MOZ_COUNT_CTOR(TestRPCChild);
}

TestRPCChild::~TestRPCChild()
{
    MOZ_COUNT_DTOR(TestRPCChild);
}

bool
TestRPCChild::RecvStart()
{
  uint32_t result;
  if (!SendTest1_Start(&result))
    fail("SendTest1_Start");
  if (result != 100)
    fail("Wrong result (expected 100)");

  if (!SendTest2_Start())
    fail("SendTest2_Start");

  if (!SendTest2_OutOfOrder())
    fail("SendTest2_OutOfOrder");

  Close();
  return true;
}

bool
TestRPCChild::RecvTest1_InnerQuery(uint32_t* aResult)
{
  uint32_t result;
  if (!SendTest1_InnerEvent(&result))
    fail("SendTest1_InnerEvent");
  if (result != 200)
    fail("Wrong result (expected 200)");

  *aResult = 300;
  return true;
}

bool
TestRPCChild::RecvTest1_NoReenter(uint32_t* aResult)
{
  *aResult = 400;
  return true;
}

bool
TestRPCChild::RecvTest2_FirstUrgent()
{
  return true;
}

bool
TestRPCChild::RecvTest2_SecondUrgent()
{
  return true;
}

} 
} 
