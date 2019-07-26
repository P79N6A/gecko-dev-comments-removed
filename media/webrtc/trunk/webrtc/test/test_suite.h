









#ifndef WEBRTC_TEST_TEST_SUITE_H_
#define WEBRTC_TEST_TEST_SUITE_H_







#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
namespace test {

class TraceToStderr;

class TestSuite {
 public:
  TestSuite(int argc, char** argv);
  virtual ~TestSuite();

  int Run();

 protected:
  
  
  virtual void Initialize();
  virtual void Shutdown();

  DISALLOW_COPY_AND_ASSIGN(TestSuite);

 private:
  scoped_ptr<TraceToStderr> trace_to_stderr_;
};

}  
}  

#endif  
