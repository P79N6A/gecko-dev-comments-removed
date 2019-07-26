






























#include "utilities.h"
#include "googletest.h"
#include "glog/logging.h"

using namespace GOOGLE_NAMESPACE;

TEST(utilities, sync_val_compare_and_swap) {
  bool now_entering = false;
  EXPECT_FALSE(sync_val_compare_and_swap(&now_entering, false, true));
  EXPECT_TRUE(sync_val_compare_and_swap(&now_entering, false, true));
  EXPECT_TRUE(sync_val_compare_and_swap(&now_entering, false, true));
}

TEST(utilities, InitGoogleLoggingDeathTest) {
  ASSERT_DEATH(InitGoogleLogging("foobar"), "");
}

int main(int argc, char **argv) {
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);

  CHECK_EQ(RUN_ALL_TESTS(), 0);
}
