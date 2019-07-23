



#include <sstream>
#include <string>

#include "base/debug_util.h"
#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(StackTrace, OutputToStream) {
  StackTrace trace;

  
  std::ostringstream os;
  trace.OutputToStream(&os);
  std::string backtrace_message = os.str();

  size_t frames_found = 0;
  trace.Addresses(&frames_found);
  if (frames_found == 0) {
    LOG(ERROR) << "No stack frames found.  Skipping rest of test.";
    return;
  }

  
  if (backtrace_message.find("Dumping unresolved backtrace") != 
      std::string::npos) {
    LOG(ERROR) << "Unable to resolve symbols.  Skipping rest of test.";
    return;
  }

#if 0



#if defined(OS_MACOSX)

  
  
  
  
  
  
  
  
  
  

  
  EXPECT_TRUE(backtrace_message.find("start") != std::string::npos)
      << "Expected to find start in backtrace:\n"
      << backtrace_message;

#else  

  
  EXPECT_TRUE(backtrace_message.find("main") != std::string::npos)
      << "Expected to find main in backtrace:\n"
      << backtrace_message;

#if defined(OS_WIN)


#define __func__ __FUNCTION__
#endif

  
  
  EXPECT_TRUE(backtrace_message.find(__func__) != std::string::npos)
      << "Expected to find " << __func__ << " in backtrace:\n"
      << backtrace_message;

#endif  
#endif
}
