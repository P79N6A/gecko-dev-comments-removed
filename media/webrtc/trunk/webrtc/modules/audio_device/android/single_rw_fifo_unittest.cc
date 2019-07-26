









#include "webrtc/modules/audio_device/android/single_rw_fifo.h"

#include <list>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class SingleRwFifoTest : public testing::Test {
 public:
  enum {
    
    kBufferSize = 8,
    kCapacity = 6,
  };

  SingleRwFifoTest() : fifo_(kCapacity), pushed_(0), available_(0) {
  }
  virtual ~SingleRwFifoTest() {}

  void SetUp() {
    for (int8_t i = 0; i < kCapacity; ++i) {
      
      buffer_[i].reset(new int8_t[kBufferSize]);
      
      
      buffer_[i][0] = i;
      
      memory_queue_.push_back(buffer_[i].get());
    }
    available_ = kCapacity;
    VerifySizes();
  }

  void Push(int number_of_buffers) {
    for (int8_t i = 0; i < number_of_buffers; ++i) {
      int8_t* data = memory_queue_.front();
      memory_queue_.pop_front();
      fifo_.Push(data);
      --available_;
      ++pushed_;
    }
    VerifySizes();
    VerifyOrdering();
  }
  void Pop(int number_of_buffers) {
    for (int8_t i = 0; i < number_of_buffers; ++i) {
      int8_t* data = fifo_.Pop();
      memory_queue_.push_back(data);
      ++available_;
      --pushed_;
    }
    VerifySizes();
    VerifyOrdering();
  }

  void VerifyOrdering() const {
    std::list<int8_t*>::const_iterator iter = memory_queue_.begin();
    if (iter == memory_queue_.end()) {
      return;
    }
    int8_t previous_index = DataToElementIndex(*iter);
    ++iter;
    for (; iter != memory_queue_.end(); ++iter) {
      int8_t current_index = DataToElementIndex(*iter);
      EXPECT_EQ(current_index, ++previous_index % kCapacity);
    }
  }

  void VerifySizes() {
    EXPECT_EQ(available_, static_cast<int>(memory_queue_.size()));
    EXPECT_EQ(pushed_, fifo_.size());
  }

  int8_t DataToElementIndex(int8_t* data) const {
    return data[0];
  }

 protected:
  SingleRwFifo fifo_;
  
  scoped_array<int8_t> buffer_[kCapacity];
  std::list<int8_t*> memory_queue_;

  int pushed_;
  int available_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleRwFifoTest);
};

TEST_F(SingleRwFifoTest, Construct) {
  
}

TEST_F(SingleRwFifoTest, Push) {
  Push(kCapacity);
}

TEST_F(SingleRwFifoTest, Pop) {
  
  Push(available_);

  
  
  Pop(1);
  Push(1);

  
  Pop(pushed_);
  Push(1);
  Pop(1);
}

}  
