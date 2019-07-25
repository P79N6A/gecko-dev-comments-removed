






























#include <iostream>
#include "gmock/gmock.h"
#include "gtest/gtest.h"






#if GTEST_OS_WINDOWS_MOBILE
# include <tchar.h>  

int _tmain(int argc, TCHAR** argv) {
#else
int main(int argc, char** argv) {
#endif
  std::cout << "Running main() from gmock_main.cc\n";
  
  
  
  testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
