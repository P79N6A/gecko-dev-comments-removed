




#include "gtest/gtest.h"
#include "mozilla/Atomics.h"
#include "LulMain.h"
#include "GeckoProfiler.h"       
#include "platform-linux-lul.h"  




#define DEBUG_LUL_TEST 0


static void
gtest_logging_sink_for_LUL(const char* str) {
  if (DEBUG_LUL_TEST == 0) {
    return;
  }
  
  size_t n = strlen(str);
  if (n > 0 && str[n-1] == '\n') {
    char* tmp = strdup(str);
    tmp[n-1] = 0;
    fprintf(stderr, "LUL-in-gtest: %s\n", tmp);
    free(tmp);
  } else {
    fprintf(stderr, "LUL-in-gtest: %s\n", str);
  }
}

TEST(LUL, unwind_consistency) {
  
  
  
  lul::LUL* lul = new lul::LUL(gtest_logging_sink_for_LUL);
  read_procmaps(lul);

  
  
  lul->EnableUnwinding();
  int nTests = 0, nTestsPassed = 0;
  RunLulUnitTests(&nTests, &nTestsPassed, lul);
  EXPECT_TRUE(nTests == 6) << "Unexpected number of tests";
  EXPECT_TRUE(nTestsPassed == nTests) << "Not all tests passed";

  delete lul;
}
