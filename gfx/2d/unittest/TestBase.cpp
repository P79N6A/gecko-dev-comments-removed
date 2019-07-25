




































#include "TestBase.h"

#include <sstream>

using namespace std;

int
TestBase::RunTests(int *aFailures)
{
  int testsRun = 0;
  *aFailures = 0;

  for(unsigned int i = 0; i < mTests.size(); i++) {
    stringstream stream;
    stream << "Test (" << mTests[i].name << "): ";
    LogMessage(stream.str());
    stream.str("");

    mTestFailed = false;

    
    
    
    
    ((*reinterpret_cast<TestBase*>((mTests[i].implPointer))).*(mTests[i].funcCall))();

    if (!mTestFailed) {
      LogMessage("PASSED\n");
    } else {
      LogMessage("FAILED\n");
      (*aFailures)++;
    }
    testsRun++;
  }

  return testsRun;
}

void
TestBase::LogMessage(string aMessage)
{
  printf(aMessage.c_str());
}
