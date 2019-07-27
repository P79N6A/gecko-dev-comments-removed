

































#include "gtest/gtest.h"

#include <iostream>

#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

using ::std::cout;

namespace testing {




TEST(GTestEnvVarTest, Dummy) {
}

void PrintFlag(const char* flag) {
  if (strcmp(flag, "break_on_failure") == 0) {
    cout << GTEST_FLAG(break_on_failure);
    return;
  }

  if (strcmp(flag, "catch_exceptions") == 0) {
    cout << GTEST_FLAG(catch_exceptions);
    return;
  }

  if (strcmp(flag, "color") == 0) {
    cout << GTEST_FLAG(color);
    return;
  }

  if (strcmp(flag, "death_test_style") == 0) {
    cout << GTEST_FLAG(death_test_style);
    return;
  }

  if (strcmp(flag, "death_test_use_fork") == 0) {
    cout << GTEST_FLAG(death_test_use_fork);
    return;
  }

  if (strcmp(flag, "filter") == 0) {
    cout << GTEST_FLAG(filter);
    return;
  }

  if (strcmp(flag, "output") == 0) {
    cout << GTEST_FLAG(output);
    return;
  }

  if (strcmp(flag, "print_time") == 0) {
    cout << GTEST_FLAG(print_time);
    return;
  }

  if (strcmp(flag, "repeat") == 0) {
    cout << GTEST_FLAG(repeat);
    return;
  }

  if (strcmp(flag, "stack_trace_depth") == 0) {
    cout << GTEST_FLAG(stack_trace_depth);
    return;
  }

  if (strcmp(flag, "throw_on_failure") == 0) {
    cout << GTEST_FLAG(throw_on_failure);
    return;
  }

  cout << "Invalid flag name " << flag
       << ".  Valid names are break_on_failure, color, filter, etc.\n";
  exit(1);
}

}  

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  if (argc != 2) {
    cout << "Usage: gtest_env_var_test_ NAME_OF_FLAG\n";
    return 1;
  }

  testing::PrintFlag(argv[1]);
  return 0;
}
