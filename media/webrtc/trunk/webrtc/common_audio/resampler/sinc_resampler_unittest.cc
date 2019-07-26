













#define _USE_MATH_DEFINES

#include <math.h>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/common_audio/resampler/sinc_resampler.h"
#include "webrtc/common_audio/resampler/sinusoidal_linear_chirp_source.h"
#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/stringize_macros.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/test/test_suite.h"

using testing::_;

namespace webrtc {

static const double kSampleRateRatio = 192000.0 / 44100.0;
static const double kKernelInterpolationFactor = 0.5;


class MockSource : public SincResamplerCallback {
 public:
  MOCK_METHOD2(Run, void(int frames, float* destination));
};

ACTION(ClearBuffer) {
  memset(arg1, 0, arg0 * sizeof(float));
}

ACTION(FillBuffer) {
  
  
  
  memset(arg1, 64, arg0 * sizeof(float));
}



TEST(SincResamplerTest, ChunkedResample) {
  MockSource mock_source;

  
  
  SincResampler resampler(kSampleRateRatio, SincResampler::kDefaultRequestSize,
                          &mock_source);

  static const int kChunks = 2;
  int max_chunk_size = resampler.ChunkSize() * kChunks;
  scoped_array<float> resampled_destination(new float[max_chunk_size]);

  
  EXPECT_CALL(mock_source, Run(_, _))
      .Times(1).WillOnce(ClearBuffer());
  resampler.Resample(resampler.ChunkSize(), resampled_destination.get());

  
  testing::Mock::VerifyAndClear(&mock_source);
  EXPECT_CALL(mock_source, Run(_, _))
      .Times(kChunks).WillRepeatedly(ClearBuffer());
  resampler.Resample(max_chunk_size, resampled_destination.get());
}


TEST(SincResamplerTest, Flush) {
  MockSource mock_source;
  SincResampler resampler(kSampleRateRatio, SincResampler::kDefaultRequestSize,
                          &mock_source);
  scoped_array<float> resampled_destination(new float[resampler.ChunkSize()]);

  
  EXPECT_CALL(mock_source, Run(_, _))
      .Times(1).WillOnce(FillBuffer());
  resampler.Resample(resampler.ChunkSize() / 2, resampled_destination.get());
  ASSERT_NE(resampled_destination[0], 0);

  
  resampler.Flush();
  testing::Mock::VerifyAndClear(&mock_source);
  EXPECT_CALL(mock_source, Run(_, _))
      .Times(1).WillOnce(ClearBuffer());
  resampler.Resample(resampler.ChunkSize() / 2, resampled_destination.get());
  for (int i = 0; i < resampler.ChunkSize() / 2; ++i)
    ASSERT_FLOAT_EQ(resampled_destination[i], 0);
}


TEST(SincResamplerTest, DISABLED_SetRatioBench) {
  MockSource mock_source;
  SincResampler resampler(kSampleRateRatio, SincResampler::kDefaultRequestSize,
                          &mock_source);

  TickTime start = TickTime::Now();
  for (int i = 1; i < 10000; ++i)
    resampler.SetRatio(1.0 / i);
  double total_time_c_us = (TickTime::Now() - start).Microseconds();
  printf("SetRatio() took %.2fms.\n", total_time_c_us / 1000);
}



#if defined(WEBRTC_ARCH_X86_FAMILY)
#define CONVOLVE_FUNC Convolve_SSE
#elif defined(WEBRTC_ARCH_ARM_V7)
#define CONVOLVE_FUNC Convolve_NEON
#endif




#if defined(CONVOLVE_FUNC)
TEST(SincResamplerTest, Convolve) {
#if defined(WEBRTC_ARCH_X86_FAMILY)
  ASSERT_TRUE(WebRtc_GetCPUInfo(kSSE2));
#elif defined(WEBRTC_ARCH_ARM_V7)
  ASSERT_TRUE(WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON);
#endif

  
  MockSource mock_source;
  SincResampler resampler(kSampleRateRatio, SincResampler::kDefaultRequestSize,
                          &mock_source);

  
  
  static const double kEpsilon = 0.00000005;

  
  
  double result = resampler.Convolve_C(
      resampler.kernel_storage_.get(), resampler.kernel_storage_.get(),
      resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  double result2 = resampler.CONVOLVE_FUNC(
      resampler.kernel_storage_.get(), resampler.kernel_storage_.get(),
      resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  EXPECT_NEAR(result2, result, kEpsilon);

  
  result = resampler.Convolve_C(
      resampler.kernel_storage_.get() + 1, resampler.kernel_storage_.get(),
      resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  result2 = resampler.CONVOLVE_FUNC(
      resampler.kernel_storage_.get() + 1, resampler.kernel_storage_.get(),
      resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  EXPECT_NEAR(result2, result, kEpsilon);
}
#endif




TEST(SincResamplerTest, ConvolveBenchmark) {
  
  MockSource mock_source;
  SincResampler resampler(kSampleRateRatio, SincResampler::kDefaultRequestSize,
                          &mock_source);

  
  
  const int kConvolveIterations = 1000000;

  printf("Benchmarking %d iterations:\n", kConvolveIterations);

  
  TickTime start = TickTime::Now();
  for (int i = 0; i < kConvolveIterations; ++i) {
    resampler.Convolve_C(
        resampler.kernel_storage_.get(), resampler.kernel_storage_.get(),
        resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  }
  double total_time_c_us = (TickTime::Now() - start).Microseconds();
  printf("Convolve_C took %.2fms.\n", total_time_c_us / 1000);

#if defined(CONVOLVE_FUNC)
#if defined(WEBRTC_ARCH_X86_FAMILY)
  ASSERT_TRUE(WebRtc_GetCPUInfo(kSSE2));
#elif defined(WEBRTC_ARCH_ARM_V7)
  ASSERT_TRUE(WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON);
#endif

  
  start = TickTime::Now();
  for (int j = 0; j < kConvolveIterations; ++j) {
    resampler.CONVOLVE_FUNC(
        resampler.kernel_storage_.get() + 1, resampler.kernel_storage_.get(),
        resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  }
  double total_time_optimized_unaligned_us =
      (TickTime::Now() - start).Microseconds();
  printf(STRINGIZE(CONVOLVE_FUNC) "(unaligned) took %.2fms; which is %.2fx "
         "faster than Convolve_C.\n", total_time_optimized_unaligned_us / 1000,
         total_time_c_us / total_time_optimized_unaligned_us);

  
  start = TickTime::Now();
  for (int j = 0; j < kConvolveIterations; ++j) {
    resampler.CONVOLVE_FUNC(
        resampler.kernel_storage_.get(), resampler.kernel_storage_.get(),
        resampler.kernel_storage_.get(), kKernelInterpolationFactor);
  }
  double total_time_optimized_aligned_us =
      (TickTime::Now() - start).Microseconds();
  printf(STRINGIZE(CONVOLVE_FUNC) " (aligned) took %.2fms; which is %.2fx "
         "faster than Convolve_C and %.2fx faster than "
         STRINGIZE(CONVOLVE_FUNC) " (unaligned).\n",
         total_time_optimized_aligned_us / 1000,
         total_time_c_us / total_time_optimized_aligned_us,
         total_time_optimized_unaligned_us / total_time_optimized_aligned_us);
#endif
}

#undef CONVOLVE_FUNC

typedef std::tr1::tuple<int, int, double, double> SincResamplerTestData;
class SincResamplerTest
    : public testing::TestWithParam<SincResamplerTestData> {
 public:
  SincResamplerTest()
      : input_rate_(std::tr1::get<0>(GetParam())),
        output_rate_(std::tr1::get<1>(GetParam())),
        rms_error_(std::tr1::get<2>(GetParam())),
        low_freq_error_(std::tr1::get<3>(GetParam())) {
  }

  virtual ~SincResamplerTest() {}

 protected:
  int input_rate_;
  int output_rate_;
  double rms_error_;
  double low_freq_error_;
};


TEST_P(SincResamplerTest, Resample) {
  
  static const double kTestDurationSecs = 1;
  const int input_samples = kTestDurationSecs * input_rate_;
  const int output_samples = kTestDurationSecs * output_rate_;

  
  const double input_nyquist_freq = 0.5 * input_rate_;

  
  SinusoidalLinearChirpSource resampler_source(
      input_rate_, input_samples, input_nyquist_freq, 0);

  const double io_ratio = input_rate_ / static_cast<double>(output_rate_);
  SincResampler resampler(io_ratio, SincResampler::kDefaultRequestSize,
                          &resampler_source);

  
  
  scoped_array<float> kernel(new float[SincResampler::kKernelStorageSize]);
  memcpy(kernel.get(), resampler.get_kernel_for_testing(),
         SincResampler::kKernelStorageSize);
  resampler.SetRatio(M_PI);
  ASSERT_NE(0, memcmp(kernel.get(), resampler.get_kernel_for_testing(),
                      SincResampler::kKernelStorageSize));
  resampler.SetRatio(io_ratio);
  ASSERT_EQ(0, memcmp(kernel.get(), resampler.get_kernel_for_testing(),
                      SincResampler::kKernelStorageSize));

  
  
  scoped_array<float> resampled_destination(new float[output_samples]);
  scoped_array<float> pure_destination(new float[output_samples]);

  
  resampler.Resample(output_samples, resampled_destination.get());

  
  SinusoidalLinearChirpSource pure_source(
      output_rate_, output_samples, input_nyquist_freq, 0);
  pure_source.Run(output_samples, pure_destination.get());

  
  
  static const double kLowFrequencyNyquistRange = 0.7;
  static const double kHighFrequencyNyquistRange = 0.9;

  
  double sum_of_squares = 0;
  double low_freq_max_error = 0;
  double high_freq_max_error = 0;
  int minimum_rate = std::min(input_rate_, output_rate_);
  double low_frequency_range = kLowFrequencyNyquistRange * 0.5 * minimum_rate;
  double high_frequency_range = kHighFrequencyNyquistRange * 0.5 * minimum_rate;
  for (int i = 0; i < output_samples; ++i) {
    double error = fabs(resampled_destination[i] - pure_destination[i]);

    if (pure_source.Frequency(i) < low_frequency_range) {
      if (error > low_freq_max_error)
        low_freq_max_error = error;
    } else if (pure_source.Frequency(i) < high_frequency_range) {
      if (error > high_freq_max_error)
        high_freq_max_error = error;
    }
    

    sum_of_squares += error * error;
  }

  double rms_error = sqrt(sum_of_squares / output_samples);

  
  #define DBFS(x) 20 * log10(x)
  rms_error = DBFS(rms_error);
  low_freq_max_error = DBFS(low_freq_max_error);
  high_freq_max_error = DBFS(high_freq_max_error);

  EXPECT_LE(rms_error, rms_error_);
  EXPECT_LE(low_freq_max_error, low_freq_error_);

  
  static const double kHighFrequencyMaxError = -6.02;
  EXPECT_LE(high_freq_max_error, kHighFrequencyMaxError);
}


static const double kResamplingRMSError = -14.58;



INSTANTIATE_TEST_CASE_P(
    SincResamplerTest, SincResamplerTest, testing::Values(
        
        std::tr1::make_tuple(8000, 44100, kResamplingRMSError, -62.73),
        std::tr1::make_tuple(11025, 44100, kResamplingRMSError, -72.19),
        std::tr1::make_tuple(16000, 44100, kResamplingRMSError, -62.54),
        std::tr1::make_tuple(22050, 44100, kResamplingRMSError, -73.53),
        std::tr1::make_tuple(32000, 44100, kResamplingRMSError, -63.32),
        std::tr1::make_tuple(44100, 44100, kResamplingRMSError, -73.53),
        std::tr1::make_tuple(48000, 44100, -15.01, -64.04),
        std::tr1::make_tuple(96000, 44100, -18.49, -25.51),
        std::tr1::make_tuple(192000, 44100, -20.50, -13.31),

        
        std::tr1::make_tuple(8000, 48000, kResamplingRMSError, -63.43),
        std::tr1::make_tuple(11025, 48000, kResamplingRMSError, -62.61),
        std::tr1::make_tuple(16000, 48000, kResamplingRMSError, -63.96),
        std::tr1::make_tuple(22050, 48000, kResamplingRMSError, -62.42),
        std::tr1::make_tuple(32000, 48000, kResamplingRMSError, -64.04),
        std::tr1::make_tuple(44100, 48000, kResamplingRMSError, -62.63),
        std::tr1::make_tuple(48000, 48000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(96000, 48000, -18.40, -28.44),
        std::tr1::make_tuple(192000, 48000, -20.43, -14.11),

        
        std::tr1::make_tuple(8000, 96000, kResamplingRMSError, -63.19),
        std::tr1::make_tuple(11025, 96000, kResamplingRMSError, -62.61),
        std::tr1::make_tuple(16000, 96000, kResamplingRMSError, -63.39),
        std::tr1::make_tuple(22050, 96000, kResamplingRMSError, -62.42),
        std::tr1::make_tuple(32000, 96000, kResamplingRMSError, -63.95),
        std::tr1::make_tuple(44100, 96000, kResamplingRMSError, -62.63),
        std::tr1::make_tuple(48000, 96000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(96000, 96000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(192000, 96000, kResamplingRMSError, -28.41),

        
        std::tr1::make_tuple(8000, 192000, kResamplingRMSError, -63.10),
        std::tr1::make_tuple(11025, 192000, kResamplingRMSError, -62.61),
        std::tr1::make_tuple(16000, 192000, kResamplingRMSError, -63.14),
        std::tr1::make_tuple(22050, 192000, kResamplingRMSError, -62.42),
        std::tr1::make_tuple(32000, 192000, kResamplingRMSError, -63.38),
        std::tr1::make_tuple(44100, 192000, kResamplingRMSError, -62.63),
        std::tr1::make_tuple(48000, 192000, kResamplingRMSError, -73.44),
        std::tr1::make_tuple(96000, 192000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(192000, 192000, kResamplingRMSError, -73.52)));

}  
