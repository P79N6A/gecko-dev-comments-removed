




































#pragma once

#include <string>
#include <vector>

#ifdef _MSC_VER

#pragma pointers_to_members(full_generality, single_inheritance)
#endif

#define VERIFY(arg) if (!(arg)) { \
  LogMessage("VERIFY FAILED: "#arg"\n"); \
  mTestFailed = true; \
  }

#define REGISTER_TEST(className, testName) \
  mTests.push_back(Test(static_cast<TestCall>(&className::testName), #testName, this))

class TestBase
{
public:
  TestBase() {}

  typedef void (TestBase::*TestCall)();

  int RunTests(int *aFailures);

protected:
  static void LogMessage(std::string aMessage);

  struct Test {
    Test(TestCall aCall, std::string aName, void *aImplPointer)
      : funcCall(aCall)
      , name(aName)
      , implPointer(aImplPointer)
    {
    }
    TestCall funcCall;
    std::string name;
    void *implPointer;
  };
  std::vector<Test> mTests;

  bool mTestFailed;

private:
  
  TestBase(const TestBase &aOther);
};
