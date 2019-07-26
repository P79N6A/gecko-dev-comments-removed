







#ifndef gtest_ringbuffer_dumper_h__
#define gtest_ringbuffer_dumper_h__

#include "mozilla/SyncRunnable.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

#include "runnable_utils.h"
#include "rlogringbuffer.h"

using mozilla::RLogRingBuffer;
using mozilla::WrapRunnable;

namespace test {
class RingbufferDumper : public ::testing::EmptyTestEventListener {
  public:
    explicit RingbufferDumper(MtransportTestUtils* test_utils) :
      test_utils_(test_utils)
    {}

    void ClearRingBuffer_s() {
      RLogRingBuffer::CreateInstance();
      
      RLogRingBuffer::GetInstance()->SetLogLimit(0);
      RLogRingBuffer::GetInstance()->SetLogLimit(UINT32_MAX);
    }

    void DestroyRingBuffer_s() {
      RLogRingBuffer::DestroyInstance();
    }

    void DumpRingBuffer_s() {
      std::deque<std::string> logs;
      
      RLogRingBuffer::GetInstance()->GetAny(0, &logs);
      for (auto l = logs.begin(); l != logs.end(); ++l) {
        std::cout << *l << std::endl;
      }
      ClearRingBuffer_s();
    }

    virtual void OnTestStart(const ::testing::TestInfo& testInfo) {
      mozilla::SyncRunnable::DispatchToThread(
          test_utils_->sts_target(),
          WrapRunnable(this, &RingbufferDumper::ClearRingBuffer_s));
    }

    virtual void OnTestEnd(const ::testing::TestInfo& testInfo) {
      mozilla::SyncRunnable::DispatchToThread(
          test_utils_->sts_target(),
          WrapRunnable(this, &RingbufferDumper::DestroyRingBuffer_s));
    }

    
    virtual void OnTestPartResult(const ::testing::TestPartResult& testResult) {
      if (testResult.failed()) {
        
        mozilla::SyncRunnable::DispatchToThread(
            test_utils_->sts_target(),
            WrapRunnable(this, &RingbufferDumper::DumpRingBuffer_s));
      }
    }

  private:
    MtransportTestUtils *test_utils_;
};

} 

#endif 



