



































#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_DEATH_TEST_INTERNAL_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_DEATH_TEST_INTERNAL_H_

#include <gtest/internal/gtest-internal.h>

namespace testing {
namespace internal {

GTEST_DECLARE_string_(internal_run_death_test);


const char kDeathTestStyleFlag[] = "death_test_style";
const char kDeathTestUseFork[] = "death_test_use_fork";
const char kInternalRunDeathTestFlag[] = "internal_run_death_test";

#if GTEST_HAS_DEATH_TEST














class DeathTest {
 public:
  
  
  
  
  
  
  
  
  static bool Create(const char* statement, const RE* regex,
                     const char* file, int line, DeathTest** test);
  DeathTest();
  virtual ~DeathTest() { }

  
  class ReturnSentinel {
   public:
    explicit ReturnSentinel(DeathTest* test) : test_(test) { }
    ~ReturnSentinel() { test_->Abort(TEST_ENCOUNTERED_RETURN_STATEMENT); }
   private:
    DeathTest* const test_;
    GTEST_DISALLOW_COPY_AND_ASSIGN_(ReturnSentinel);
  } GTEST_ATTRIBUTE_UNUSED_;

  
  
  
  
  
  enum TestRole { OVERSEE_TEST, EXECUTE_TEST };

  
  enum AbortReason { TEST_ENCOUNTERED_RETURN_STATEMENT, TEST_DID_NOT_DIE };

  
  virtual TestRole AssumeRole() = 0;

  
  virtual int Wait() = 0;

  
  
  
  
  
  
  
  virtual bool Passed(bool exit_status_ok) = 0;

  
  virtual void Abort(AbortReason reason) = 0;

  
  
  static const char* LastMessage();

  static void set_last_death_test_message(const String& message);

 private:
  
  static String last_death_test_message_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(DeathTest);
};


class DeathTestFactory {
 public:
  virtual ~DeathTestFactory() { }
  virtual bool Create(const char* statement, const RE* regex,
                      const char* file, int line, DeathTest** test) = 0;
};


class DefaultDeathTestFactory : public DeathTestFactory {
 public:
  virtual bool Create(const char* statement, const RE* regex,
                      const char* file, int line, DeathTest** test);
};



bool ExitedUnsuccessfully(int exit_status);



#define GTEST_DEATH_TEST_(statement, predicate, regex, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (true) { \
    const ::testing::internal::RE& gtest_regex = (regex); \
    ::testing::internal::DeathTest* gtest_dt; \
    if (!::testing::internal::DeathTest::Create(#statement, &gtest_regex, \
        __FILE__, __LINE__, &gtest_dt)) { \
      goto GTEST_CONCAT_TOKEN_(gtest_label_, __LINE__); \
    } \
    if (gtest_dt != NULL) { \
      ::testing::internal::scoped_ptr< ::testing::internal::DeathTest> \
          gtest_dt_ptr(gtest_dt); \
      switch (gtest_dt->AssumeRole()) { \
        case ::testing::internal::DeathTest::OVERSEE_TEST: \
          if (!gtest_dt->Passed(predicate(gtest_dt->Wait()))) { \
            goto GTEST_CONCAT_TOKEN_(gtest_label_, __LINE__); \
          } \
          break; \
        case ::testing::internal::DeathTest::EXECUTE_TEST: { \
          ::testing::internal::DeathTest::ReturnSentinel \
              gtest_sentinel(gtest_dt); \
          GTEST_HIDE_UNREACHABLE_CODE_(statement); \
          gtest_dt->Abort(::testing::internal::DeathTest::TEST_DID_NOT_DIE); \
          break; \
        } \
      } \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_, __LINE__): \
      fail(::testing::internal::DeathTest::LastMessage())






class InternalRunDeathTestFlag {
 public:
  InternalRunDeathTestFlag(const String& file,
                           int line,
                           int index,
                           int write_fd)
      : file_(file), line_(line), index_(index), write_fd_(write_fd) {}

  ~InternalRunDeathTestFlag() {
    if (write_fd_ >= 0)
      posix::Close(write_fd_);
  }

  String file() const { return file_; }
  int line() const { return line_; }
  int index() const { return index_; }
  int write_fd() const { return write_fd_; }

 private:
  String file_;
  int line_;
  int index_;
  int write_fd_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(InternalRunDeathTestFlag);
};




InternalRunDeathTestFlag* ParseInternalRunDeathTestFlag();

#endif  

}  
}  

#endif  
