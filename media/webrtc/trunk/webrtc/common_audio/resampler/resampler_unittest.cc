









#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/common_audio/resampler/include/resampler.h"



namespace webrtc {
namespace {
const ResamplerType kTypes[] = {
  kResamplerSynchronous,
  kResamplerAsynchronous,
  kResamplerSynchronousStereo,
  kResamplerAsynchronousStereo
  
};
const size_t kTypesSize = sizeof(kTypes) / sizeof(*kTypes);


const int kMaxRate = 96000;
const int kRates[] = {
  8000,
  16000,
  32000,
  44000,
  48000,
  kMaxRate
};
const size_t kRatesSize = sizeof(kRates) / sizeof(*kRates);
const int kMaxChannels = 2;
const size_t kDataSize = static_cast<size_t> (kMaxChannels * kMaxRate / 100);


bool ValidRates(int in_rate, int out_rate) {
  
  if ((in_rate == 44000 && (out_rate == 48000 || out_rate == 96000)) ||
      (out_rate == 44000 && (in_rate == 48000 || in_rate == 96000))) {
    return false;
  }

  return true;
}

class ResamplerTest : public testing::Test {
 protected:
  ResamplerTest();
  virtual void SetUp();
  virtual void TearDown();

  Resampler rs_;
  int16_t data_in_[kDataSize];
  int16_t data_out_[kDataSize];
};

ResamplerTest::ResamplerTest() {}

void ResamplerTest::SetUp() {
  
  memset(data_in_, 1, sizeof(data_in_));
}

void ResamplerTest::TearDown() {}

TEST_F(ResamplerTest, Reset) {
  
  
  

  
  for (size_t i = 0; i < kRatesSize; ++i) {
    for (size_t j = 0; j < kRatesSize; ++j) {
      for (size_t k = 0; k < kTypesSize; ++k) {
        std::ostringstream ss;
        ss << "Input rate: " << kRates[i] << ", output rate: " << kRates[j]
            << ", type: " << kTypes[k];
        SCOPED_TRACE(ss.str());
        if (ValidRates(kRates[i], kRates[j]))
          EXPECT_EQ(0, rs_.Reset(kRates[i], kRates[j], kTypes[k]));
        else
          EXPECT_EQ(-1, rs_.Reset(kRates[i], kRates[j], kTypes[k]));
      }
    }
  }
}



TEST_F(ResamplerTest, Synchronous) {
  for (size_t i = 0; i < kRatesSize; ++i) {
    for (size_t j = 0; j < kRatesSize; ++j) {
      std::ostringstream ss;
      ss << "Input rate: " << kRates[i] << ", output rate: " << kRates[j];
      SCOPED_TRACE(ss.str());

      if (ValidRates(kRates[i], kRates[j])) {
        int in_length = kRates[i] / 100;
        int out_length = 0;
        EXPECT_EQ(0, rs_.Reset(kRates[i], kRates[j], kResamplerSynchronous));
        EXPECT_EQ(0, rs_.Push(data_in_, in_length, data_out_, kDataSize,
                              out_length));
        EXPECT_EQ(kRates[j] / 100, out_length);
      } else {
        EXPECT_EQ(-1, rs_.Reset(kRates[i], kRates[j], kResamplerSynchronous));
      }
    }
  }
}

TEST_F(ResamplerTest, SynchronousStereo) {
  
  const int kChannels = 2;
  for (size_t i = 0; i < kRatesSize; ++i) {
    for (size_t j = 0; j < kRatesSize; ++j) {
      std::ostringstream ss;
      ss << "Input rate: " << kRates[i] << ", output rate: " << kRates[j];
      SCOPED_TRACE(ss.str());

      if (ValidRates(kRates[i], kRates[j])) {
        int in_length = kChannels * kRates[i] / 100;
        int out_length = 0;
        EXPECT_EQ(0, rs_.Reset(kRates[i], kRates[j],
                               kResamplerSynchronousStereo));
        EXPECT_EQ(0, rs_.Push(data_in_, in_length, data_out_, kDataSize,
                              out_length));
        EXPECT_EQ(kChannels * kRates[j] / 100, out_length);
      } else {
        EXPECT_EQ(-1, rs_.Reset(kRates[i], kRates[j],
                                kResamplerSynchronousStereo));
      }
    }
  }
}
}  
}  
