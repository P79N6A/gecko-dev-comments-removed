









#include "webrtc/system_wrappers/interface/logging.h"

#include "gtest/gtest.h"
#include "webrtc/system_wrappers/interface/condition_variable_wrapper.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {
namespace {

const size_t kBoilerplateLength = 71;

class LoggingTest : public ::testing::Test, public TraceCallback {
 public:
  virtual void Print(TraceLevel level, const char* msg, int length) {
    CriticalSectionScoped cs(crit_.get());
    
    
    if (level_ != kTraceNone &&
        expected_log_.str().size() == length - kBoilerplateLength - 1) {
      EXPECT_EQ(level_, level);
      EXPECT_EQ(expected_log_.str(), &msg[kBoilerplateLength]);
      level_ = kTraceNone;
      cv_->Wake();
    }
  }

 protected:
  LoggingTest()
    : crit_(CriticalSectionWrapper::CreateCriticalSection()),
      cv_(ConditionVariableWrapper::CreateConditionVariable()),
      level_(kTraceNone),
      expected_log_() {
  }

  void SetUp() {
    Trace::CreateTrace();
    Trace::SetTraceCallback(this);
    
    Trace::SetLevelFilter(kTraceWarning | kTraceError);
  }

  void TearDown() {
    CriticalSectionScoped cs(crit_.get());
    Trace::SetTraceCallback(NULL);
    Trace::ReturnTrace();
    ASSERT_EQ(kTraceNone, level_) << "Print() was not called";
  }

  scoped_ptr<CriticalSectionWrapper> crit_;
  scoped_ptr<ConditionVariableWrapper> cv_;
  TraceLevel level_;
  int length_;
  std::ostringstream expected_log_;
};

TEST_F(LoggingTest, LogStream) {
  {
    CriticalSectionScoped cs(crit_.get());
    level_ = kTraceWarning;
    std::string msg = "Important message";
    expected_log_ << "(logging_unittest.cc:" << __LINE__ + 1 << "): " << msg;
    LOG(LS_WARNING) << msg;
    cv_->SleepCS(*crit_.get(), 2000);
  }
}

TEST_F(LoggingTest, LogFunctionError) {
  {
    CriticalSectionScoped cs(crit_.get());
    int bar = 42;
    int baz = 99;
    level_ = kTraceError;
    expected_log_ << "(logging_unittest.cc:" << __LINE__ + 2
                  << "): Foo failed: bar=" << bar << ", baz=" << baz;
    LOG_FERR2(LS_ERROR, Foo, bar, baz);
    cv_->SleepCS(*crit_.get(), 2000);
  }
}

}  
}  
