

































#include "gtest/gtest.h"

#include <stdio.h>  
#include <stdlib.h>  

#if GTEST_HAS_SEH
# include <windows.h>
#endif

#if GTEST_HAS_EXCEPTIONS
# include <exception>  
# include <stdexcept>
#endif

using testing::Test;

#if GTEST_HAS_SEH

class SehExceptionInConstructorTest : public Test {
 public:
  SehExceptionInConstructorTest() { RaiseException(42, 0, 0, NULL); }
};

TEST_F(SehExceptionInConstructorTest, ThrowsExceptionInConstructor) {}

class SehExceptionInDestructorTest : public Test {
 public:
  ~SehExceptionInDestructorTest() { RaiseException(42, 0, 0, NULL); }
};

TEST_F(SehExceptionInDestructorTest, ThrowsExceptionInDestructor) {}

class SehExceptionInSetUpTestCaseTest : public Test {
 public:
  static void SetUpTestCase() { RaiseException(42, 0, 0, NULL); }
};

TEST_F(SehExceptionInSetUpTestCaseTest, ThrowsExceptionInSetUpTestCase) {}

class SehExceptionInTearDownTestCaseTest : public Test {
 public:
  static void TearDownTestCase() { RaiseException(42, 0, 0, NULL); }
};

TEST_F(SehExceptionInTearDownTestCaseTest, ThrowsExceptionInTearDownTestCase) {}

class SehExceptionInSetUpTest : public Test {
 protected:
  virtual void SetUp() { RaiseException(42, 0, 0, NULL); }
};

TEST_F(SehExceptionInSetUpTest, ThrowsExceptionInSetUp) {}

class SehExceptionInTearDownTest : public Test {
 protected:
  virtual void TearDown() { RaiseException(42, 0, 0, NULL); }
};

TEST_F(SehExceptionInTearDownTest, ThrowsExceptionInTearDown) {}

TEST(SehExceptionTest, ThrowsSehException) {
  RaiseException(42, 0, 0, NULL);
}

#endif  

#if GTEST_HAS_EXCEPTIONS

class CxxExceptionInConstructorTest : public Test {
 public:
  CxxExceptionInConstructorTest() {
    
    
    GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(
        throw std::runtime_error("Standard C++ exception"));
  }

  static void TearDownTestCase() {
    printf("%s",
           "CxxExceptionInConstructorTest::TearDownTestCase() "
           "called as expected.\n");
  }

 protected:
  ~CxxExceptionInConstructorTest() {
    ADD_FAILURE() << "CxxExceptionInConstructorTest destructor "
                  << "called unexpectedly.";
  }

  virtual void SetUp() {
    ADD_FAILURE() << "CxxExceptionInConstructorTest::SetUp() "
                  << "called unexpectedly.";
  }

  virtual void TearDown() {
    ADD_FAILURE() << "CxxExceptionInConstructorTest::TearDown() "
                  << "called unexpectedly.";
  }
};

TEST_F(CxxExceptionInConstructorTest, ThrowsExceptionInConstructor) {
  ADD_FAILURE() << "CxxExceptionInConstructorTest test body "
                << "called unexpectedly.";
}


#if !defined(__GXX_EXPERIMENTAL_CXX0X__) &&  __cplusplus < 201103L
class CxxExceptionInDestructorTest : public Test {
 public:
  static void TearDownTestCase() {
    printf("%s",
           "CxxExceptionInDestructorTest::TearDownTestCase() "
           "called as expected.\n");
  }

 protected:
  ~CxxExceptionInDestructorTest() {
    GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(
        throw std::runtime_error("Standard C++ exception"));
  }
};

TEST_F(CxxExceptionInDestructorTest, ThrowsExceptionInDestructor) {}
#endif  

class CxxExceptionInSetUpTestCaseTest : public Test {
 public:
  CxxExceptionInSetUpTestCaseTest() {
    printf("%s",
           "CxxExceptionInSetUpTestCaseTest constructor "
           "called as expected.\n");
  }

  static void SetUpTestCase() {
    throw std::runtime_error("Standard C++ exception");
  }

  static void TearDownTestCase() {
    printf("%s",
           "CxxExceptionInSetUpTestCaseTest::TearDownTestCase() "
           "called as expected.\n");
  }

 protected:
  ~CxxExceptionInSetUpTestCaseTest() {
    printf("%s",
           "CxxExceptionInSetUpTestCaseTest destructor "
           "called as expected.\n");
  }

  virtual void SetUp() {
    printf("%s",
           "CxxExceptionInSetUpTestCaseTest::SetUp() "
           "called as expected.\n");
  }

  virtual void TearDown() {
    printf("%s",
           "CxxExceptionInSetUpTestCaseTest::TearDown() "
           "called as expected.\n");
  }
};

TEST_F(CxxExceptionInSetUpTestCaseTest, ThrowsExceptionInSetUpTestCase) {
  printf("%s",
         "CxxExceptionInSetUpTestCaseTest test body "
         "called as expected.\n");
}

class CxxExceptionInTearDownTestCaseTest : public Test {
 public:
  static void TearDownTestCase() {
    throw std::runtime_error("Standard C++ exception");
  }
};

TEST_F(CxxExceptionInTearDownTestCaseTest, ThrowsExceptionInTearDownTestCase) {}

class CxxExceptionInSetUpTest : public Test {
 public:
  static void TearDownTestCase() {
    printf("%s",
           "CxxExceptionInSetUpTest::TearDownTestCase() "
           "called as expected.\n");
  }

 protected:
  ~CxxExceptionInSetUpTest() {
    printf("%s",
           "CxxExceptionInSetUpTest destructor "
           "called as expected.\n");
  }

  virtual void SetUp() { throw std::runtime_error("Standard C++ exception"); }

  virtual void TearDown() {
    printf("%s",
           "CxxExceptionInSetUpTest::TearDown() "
           "called as expected.\n");
  }
};

TEST_F(CxxExceptionInSetUpTest, ThrowsExceptionInSetUp) {
  ADD_FAILURE() << "CxxExceptionInSetUpTest test body "
                << "called unexpectedly.";
}

class CxxExceptionInTearDownTest : public Test {
 public:
  static void TearDownTestCase() {
    printf("%s",
           "CxxExceptionInTearDownTest::TearDownTestCase() "
           "called as expected.\n");
  }

 protected:
  ~CxxExceptionInTearDownTest() {
    printf("%s",
           "CxxExceptionInTearDownTest destructor "
           "called as expected.\n");
  }

  virtual void TearDown() {
    throw std::runtime_error("Standard C++ exception");
  }
};

TEST_F(CxxExceptionInTearDownTest, ThrowsExceptionInTearDown) {}

class CxxExceptionInTestBodyTest : public Test {
 public:
  static void TearDownTestCase() {
    printf("%s",
           "CxxExceptionInTestBodyTest::TearDownTestCase() "
           "called as expected.\n");
  }

 protected:
  ~CxxExceptionInTestBodyTest() {
    printf("%s",
           "CxxExceptionInTestBodyTest destructor "
           "called as expected.\n");
  }

  virtual void TearDown() {
    printf("%s",
           "CxxExceptionInTestBodyTest::TearDown() "
           "called as expected.\n");
  }
};

TEST_F(CxxExceptionInTestBodyTest, ThrowsStdCxxException) {
  throw std::runtime_error("Standard C++ exception");
}

TEST(CxxExceptionTest, ThrowsNonStdCxxException) {
  throw "C-string";
}




void TerminateHandler() {
  fprintf(stderr, "%s\n", "Unhandled C++ exception terminating the program.");
  fflush(NULL);
  exit(3);
}

#endif  

int main(int argc, char** argv) {
#if GTEST_HAS_EXCEPTIONS
  std::set_terminate(&TerminateHandler);
#endif
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
