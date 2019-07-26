









#include "webrtc/common_audio/signal_processing/include/real_fft.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/typedefs.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace webrtc {
namespace {


const int kOrder = 5;


const int kTimeDataLength = 1 << kOrder;
const int kFreqDataLength = (1 << kOrder) + 2;


const int kComplexFftDataLength = 2 << kOrder;

const int16_t kRefData[kTimeDataLength] = {
  11739, 6848, -8688, 31980, -30295, 25242, 27085, 19410,
  -26299, 15607, -10791, 11778, -23819, 14498, -25772, 10076,
  1173, 6848, -8688, 31980, -30295, 2522, 27085, 19410,
  -2629, 5607, -3, 1178, -23819, 1498, -25772, 10076
};

class RealFFTTest : public ::testing::Test {
 protected:
   RealFFTTest() {
     WebRtcSpl_Init();
   }
};

TEST_F(RealFFTTest, CreateFailsOnBadInput) {
  RealFFT* fft = WebRtcSpl_CreateRealFFT(11);
  EXPECT_TRUE(fft == NULL);
  fft = WebRtcSpl_CreateRealFFT(-1);
  EXPECT_TRUE(fft == NULL);
}

TEST_F(RealFFTTest, RealAndComplexMatch) {
  int i = 0;
  int j = 0;
  int16_t real_fft_time[kTimeDataLength] = {0};
  int16_t real_fft_freq[kFreqDataLength] = {0};
  
  int16_t complex_fft_buff[kComplexFftDataLength] = {0};

  
  memcpy(real_fft_time, kRefData, sizeof(kRefData));
  for (i = 0, j = 0; i < kTimeDataLength; i += 1, j += 2) {
    complex_fft_buff[j] = kRefData[i];
    complex_fft_buff[j + 1] = 0;  
  };

  
  RealFFT* fft = WebRtcSpl_CreateRealFFT(kOrder);
  EXPECT_TRUE(fft != NULL);
  EXPECT_EQ(0, WebRtcSpl_RealForwardFFT(fft, real_fft_time, real_fft_freq));

  
  WebRtcSpl_ComplexBitReverse(complex_fft_buff, kOrder);
  EXPECT_EQ(0, WebRtcSpl_ComplexFFT(complex_fft_buff, kOrder, 1));

  
  for (i = 0; i < kFreqDataLength; i++) {
    EXPECT_EQ(real_fft_freq[i], complex_fft_buff[i]);
  }

  
  
  
  
  
  memcpy(real_fft_freq, complex_fft_buff, sizeof(real_fft_freq));

  
  int real_scale = WebRtcSpl_RealInverseFFT(fft, real_fft_freq, real_fft_time);
  EXPECT_GE(real_scale, 0);

  
  WebRtcSpl_ComplexBitReverse(complex_fft_buff, kOrder);
  int complex_scale = WebRtcSpl_ComplexIFFT(complex_fft_buff, kOrder, 1);

  
  
  
  EXPECT_EQ(real_scale, complex_scale);
  for (i = 0, j = 0; i < kTimeDataLength; i += 1, j += 2) {
    EXPECT_LE(abs(real_fft_time[i] - complex_fft_buff[j]), 1);
  }

  WebRtcSpl_FreeRealFFT(fft);
}

}  
}  
