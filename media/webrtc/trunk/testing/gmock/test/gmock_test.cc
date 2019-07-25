


































#include "gmock/gmock.h"

#include <string>
#include "gtest/gtest.h"

using testing::GMOCK_FLAG(verbose);
using testing::InitGoogleMock;
using testing::internal::g_init_gtest_count;



template <typename Char, int M, int N>
void TestInitGoogleMock(const Char* (&argv)[M], const Char* (&new_argv)[N],
                        const ::std::string& expected_gmock_verbose) {
  const ::std::string old_verbose = GMOCK_FLAG(verbose);

  int argc = M;
  InitGoogleMock(&argc, const_cast<Char**>(argv));
  ASSERT_EQ(N, argc) << "The new argv has wrong number of elements.";

  for (int i = 0; i < N; i++) {
    EXPECT_STREQ(new_argv[i], argv[i]);
  }

  EXPECT_EQ(expected_gmock_verbose, GMOCK_FLAG(verbose).c_str());
  GMOCK_FLAG(verbose) = old_verbose;  
}

TEST(InitGoogleMockTest, ParsesInvalidCommandLine) {
  const char* argv[] = {
    NULL
  };

  const char* new_argv[] = {
    NULL
  };

  TestInitGoogleMock(argv, new_argv, GMOCK_FLAG(verbose));
}

TEST(InitGoogleMockTest, ParsesEmptyCommandLine) {
  const char* argv[] = {
    "foo.exe",
    NULL
  };

  const char* new_argv[] = {
    "foo.exe",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, GMOCK_FLAG(verbose));
}

TEST(InitGoogleMockTest, ParsesSingleFlag) {
  const char* argv[] = {
    "foo.exe",
    "--gmock_verbose=info",
    NULL
  };

  const char* new_argv[] = {
    "foo.exe",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, "info");
}

TEST(InitGoogleMockTest, ParsesUnrecognizedFlag) {
  const char* argv[] = {
    "foo.exe",
    "--non_gmock_flag=blah",
    NULL
  };

  const char* new_argv[] = {
    "foo.exe",
    "--non_gmock_flag=blah",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, GMOCK_FLAG(verbose));
}

TEST(InitGoogleMockTest, ParsesGoogleMockFlagAndUnrecognizedFlag) {
  const char* argv[] = {
    "foo.exe",
    "--non_gmock_flag=blah",
    "--gmock_verbose=error",
    NULL
  };

  const char* new_argv[] = {
    "foo.exe",
    "--non_gmock_flag=blah",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, "error");
}

TEST(InitGoogleMockTest, CallsInitGoogleTest) {
  const int old_init_gtest_count = g_init_gtest_count;
  const char* argv[] = {
    "foo.exe",
    "--non_gmock_flag=blah",
    "--gmock_verbose=error",
    NULL
  };

  const char* new_argv[] = {
    "foo.exe",
    "--non_gmock_flag=blah",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, "error");
  EXPECT_EQ(old_init_gtest_count + 1, g_init_gtest_count);
}

TEST(WideInitGoogleMockTest, ParsesInvalidCommandLine) {
  const wchar_t* argv[] = {
    NULL
  };

  const wchar_t* new_argv[] = {
    NULL
  };

  TestInitGoogleMock(argv, new_argv, GMOCK_FLAG(verbose));
}

TEST(WideInitGoogleMockTest, ParsesEmptyCommandLine) {
  const wchar_t* argv[] = {
    L"foo.exe",
    NULL
  };

  const wchar_t* new_argv[] = {
    L"foo.exe",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, GMOCK_FLAG(verbose));
}

TEST(WideInitGoogleMockTest, ParsesSingleFlag) {
  const wchar_t* argv[] = {
    L"foo.exe",
    L"--gmock_verbose=info",
    NULL
  };

  const wchar_t* new_argv[] = {
    L"foo.exe",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, "info");
}

TEST(WideInitGoogleMockTest, ParsesUnrecognizedFlag) {
  const wchar_t* argv[] = {
    L"foo.exe",
    L"--non_gmock_flag=blah",
    NULL
  };

  const wchar_t* new_argv[] = {
    L"foo.exe",
    L"--non_gmock_flag=blah",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, GMOCK_FLAG(verbose));
}

TEST(WideInitGoogleMockTest, ParsesGoogleMockFlagAndUnrecognizedFlag) {
  const wchar_t* argv[] = {
    L"foo.exe",
    L"--non_gmock_flag=blah",
    L"--gmock_verbose=error",
    NULL
  };

  const wchar_t* new_argv[] = {
    L"foo.exe",
    L"--non_gmock_flag=blah",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, "error");
}

TEST(WideInitGoogleMockTest, CallsInitGoogleTest) {
  const int old_init_gtest_count = g_init_gtest_count;
  const wchar_t* argv[] = {
    L"foo.exe",
    L"--non_gmock_flag=blah",
    L"--gmock_verbose=error",
    NULL
  };

  const wchar_t* new_argv[] = {
    L"foo.exe",
    L"--non_gmock_flag=blah",
    NULL
  };

  TestInitGoogleMock(argv, new_argv, "error");
  EXPECT_EQ(old_init_gtest_count + 1, g_init_gtest_count);
}


TEST(FlagTest, IsAccessibleInCode) {
  bool dummy = testing::GMOCK_FLAG(catch_leaked_mocks) &&
      testing::GMOCK_FLAG(verbose) == "";
  (void)dummy;  
}
