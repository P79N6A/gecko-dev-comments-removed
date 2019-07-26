









#include "webrtc/system_wrappers/source/unittest_utilities.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {



TEST(UnittestUtilities, TraceOn) {
  ScopedTracing trace(true);
  WEBRTC_TRACE(kTraceInfo, kTraceUtility, 0, "Log line that should appear");
  
  
}

TEST(UnittestUtilities, TraceOff) {
  ScopedTracing trace(false);
  WEBRTC_TRACE(kTraceInfo, kTraceUtility, 0,
               "Log line that should not appear");
  
}

}  
