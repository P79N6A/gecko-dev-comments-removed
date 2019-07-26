









#include "system_wrappers/interface/condition_variable_wrapper.h"

#include "gtest/gtest.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/thread_wrapper.h"
#include "system_wrappers/interface/trace.h"
#include "system_wrappers/source/unittest_utilities.h"

namespace webrtc {

namespace {

const int kLogTrace = false;  
const int kLongWaitMs = 100 * 1000; 
const int kShortWaitMs = 2 * 1000; 








class Baton {
 public:
  Baton()
    : giver_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      crit_sect_(CriticalSectionWrapper::CreateCriticalSection()),
      cond_var_(ConditionVariableWrapper::CreateConditionVariable()),
      being_passed_(false),
      pass_count_(0) {
  }

  ~Baton() {
    delete giver_sect_;
    delete crit_sect_;
    delete cond_var_;
  }

  
  
  
  bool Pass(WebRtc_UWord32 max_msecs) {
    CriticalSectionScoped cs_giver(giver_sect_);
    CriticalSectionScoped cs(crit_sect_);
    SignalBatonAvailable();
    const bool result = TakeBatonIfStillFree(max_msecs);
    if (result) {
      ++pass_count_;
    }
    return result;
  }

  
  bool Grab(WebRtc_UWord32 max_msecs) {
    CriticalSectionScoped cs(crit_sect_);
    return WaitUntilBatonOffered(max_msecs);
  }

  int PassCount() {
    
    
    
    
    
    CriticalSectionScoped cs(giver_sect_);
    return pass_count_;
  }

 private:
  
  
  bool WaitUntilBatonOffered(int timeout_ms) {
    while (!being_passed_) {
      if (!cond_var_->SleepCS(*crit_sect_, timeout_ms)) {
        return false;
      }
    }
    being_passed_ = false;
    cond_var_->Wake();
    return true;
  }

  void SignalBatonAvailable() {
    assert(!being_passed_);
    being_passed_ = true;
    cond_var_->Wake();
  }

  
  
  
  
  
  
  bool TakeBatonIfStillFree(int timeout_ms) {
    bool not_timeout = true;
    while (being_passed_ && not_timeout) {
      not_timeout = cond_var_->SleepCS(*crit_sect_, timeout_ms);
      
      
      
    }
    if (!being_passed_) {
      return true;
    } else {
      assert(!not_timeout);
      being_passed_ = false;
      return false;
    }
  }

  
  
  
  CriticalSectionWrapper* giver_sect_;
  
  CriticalSectionWrapper* crit_sect_;
  ConditionVariableWrapper* cond_var_;
  bool being_passed_;
  
  int pass_count_;
};



bool WaitingRunFunction(void* obj) {
  Baton* the_baton = static_cast<Baton*> (obj);
  EXPECT_TRUE(the_baton->Grab(kLongWaitMs));
  EXPECT_TRUE(the_baton->Pass(kLongWaitMs));
  return true;
}

class CondVarTest : public ::testing::Test {
 public:
  CondVarTest()
    : trace_(kLogTrace) {
  }

  virtual void SetUp() {
    thread_ = ThreadWrapper::CreateThread(&WaitingRunFunction,
                                          &baton_);
    unsigned int id = 42;
    ASSERT_TRUE(thread_->Start(id));
  }

  virtual void TearDown() {
    
    
    
    
    
    ASSERT_TRUE(baton_.Pass(kShortWaitMs));
    thread_->SetNotAlive();
    ASSERT_TRUE(baton_.Grab(kShortWaitMs));
    ASSERT_TRUE(thread_->Stop());
    delete thread_;
  }

 protected:
  Baton baton_;

 private:
  ScopedTracing trace_;
  ThreadWrapper* thread_;
};



TEST_F(CondVarTest, InitFunctionsWork) {
  
}


TEST_F(CondVarTest, PassBatonMultipleTimes) {
  const int kNumberOfRounds = 2;
  for (int i = 0; i < kNumberOfRounds; ++i) {
    ASSERT_TRUE(baton_.Pass(kShortWaitMs));
    ASSERT_TRUE(baton_.Grab(kShortWaitMs));
  }
  EXPECT_EQ(2 * kNumberOfRounds, baton_.PassCount());
}

}  

}  
