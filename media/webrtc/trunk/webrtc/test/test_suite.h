









#ifndef TEST_TEST_SUITE_H_
#define TEST_TEST_SUITE_H_







#include "system_wrappers/interface/constructor_magic.h"

namespace webrtc {
namespace test {
class TestSuite {
 public:
  TestSuite(int argc, char** argv);
  virtual ~TestSuite();

  int Run();

 protected:
  
  
  virtual void Initialize();
  virtual void Shutdown();

  DISALLOW_COPY_AND_ASSIGN(TestSuite);
};
}  
}  

#endif  
