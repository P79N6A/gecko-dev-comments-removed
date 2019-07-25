








































#include "gtest/gtest.h"

#if GTEST_OS_WINDOWS
# include <windows.h>
# include <stdlib.h>
#endif

namespace {


TEST(Foo, Bar) {
  EXPECT_EQ(2, 3);
}

#if GTEST_HAS_SEH && !GTEST_OS_WINDOWS_MOBILE

LONG WINAPI ExitWithExceptionCode(
    struct _EXCEPTION_POINTERS* exception_pointers) {
  exit(exception_pointers->ExceptionRecord->ExceptionCode);
}
#endif

}  

int main(int argc, char **argv) {
#if GTEST_OS_WINDOWS
  
  
  SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);

# if GTEST_HAS_SEH && !GTEST_OS_WINDOWS_MOBILE

  
  
  
  
  
  
  SetUnhandledExceptionFilter(ExitWithExceptionCode);

# endif
#endif

  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
