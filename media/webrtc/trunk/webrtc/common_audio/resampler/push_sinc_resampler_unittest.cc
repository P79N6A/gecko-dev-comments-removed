









#include <math.h>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/common_audio/resampler/push_sinc_resampler.h"
#include "webrtc/common_audio/resampler/sinusoidal_linear_chirp_source.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/typedefs.h"

namespace webrtc {

typedef std::tr1::tuple<int, int, double, double> PushSincResamplerTestData;
class PushSincResamplerTest
    : public testing::TestWithParam<PushSincResamplerTestData> {
 public:
  PushSincResamplerTest()
      : input_rate_(std::tr1::get<0>(GetParam())),
        output_rate_(std::tr1::get<1>(GetParam())),
        rms_error_(std::tr1::get<2>(GetParam())),
        low_freq_error_(std::tr1::get<3>(GetParam())) {
  }

  virtual ~PushSincResamplerTest() {}

 protected:
  int input_rate_;
  int output_rate_;
  double rms_error_;
  double low_freq_error_;
};

class ZeroSource : public SincResamplerCallback {
 public:
  void Run(int frames, float* destination) {
    memset(destination, 0, sizeof(float) * frames);
  }
};



TEST_P(PushSincResamplerTest, DISABLED_ResampleBenchmark) {
  const int input_samples = input_rate_ / 100;
  const int output_samples = output_rate_ / 100;
  const int kResampleIterations = 200000;

  
  ZeroSource resampler_source;

  scoped_array<float> resampled_destination(new float[output_samples]);
  scoped_array<float> source(new float[input_samples]);
  scoped_array<int16_t> source_int(new int16_t[input_samples]);
  scoped_array<int16_t> destination_int(new int16_t[output_samples]);

  resampler_source.Run(input_samples, source.get());
  for (int i = 0; i < input_samples; ++i) {
    source_int[i] = static_cast<int16_t>(floor(32767 * source[i] + 0.5));
  }

  printf("Benchmarking %d iterations of %d Hz -> %d Hz:\n",
         kResampleIterations, input_rate_, output_rate_);
  const double io_ratio = input_rate_ / static_cast<double>(output_rate_);
  SincResampler sinc_resampler(io_ratio, SincResampler::kDefaultRequestSize,
                               &resampler_source);
  TickTime start = TickTime::Now();
  for (int i = 0; i < kResampleIterations; ++i) {
    sinc_resampler.Resample(output_samples, resampled_destination.get());
  }
  double total_time_sinc_us = (TickTime::Now() - start).Microseconds();
  printf("SincResampler took %.2f us per frame.\n",
         total_time_sinc_us / kResampleIterations);

  PushSincResampler resampler(input_samples, output_samples);
  start = TickTime::Now();
  for (int i = 0; i < kResampleIterations; ++i) {
    EXPECT_EQ(output_samples,
              resampler.Resample(source_int.get(), input_samples,
                                 destination_int.get(), output_samples));
  }
  double total_time_us = (TickTime::Now() - start).Microseconds();
  printf("PushSincResampler took %.2f us per frame; which is a %.1f%% overhead "
         "on SincResampler.\n\n", total_time_us / kResampleIterations,
         (total_time_us - total_time_sinc_us) / total_time_sinc_us * 100);
}


TEST_P(PushSincResamplerTest, Resample) {
  
  static const double kTestDurationSecs = 1;
  
  const int kNumBlocks = kTestDurationSecs * 100;
  const int input_block_size = input_rate_ / 100;
  const int output_block_size = output_rate_ / 100;
  const int input_samples = kTestDurationSecs * input_rate_;
  const int output_samples = kTestDurationSecs * output_rate_;

  
  const double input_nyquist_freq = 0.5 * input_rate_;

  
  SinusoidalLinearChirpSource resampler_source(
      input_rate_, input_samples, input_nyquist_freq, 0);

  PushSincResampler resampler(input_block_size, output_block_size);

  
  
  scoped_array<float> resampled_destination(new float[output_samples]);
  scoped_array<float> pure_destination(new float[output_samples]);
  scoped_array<float> source(new float[input_samples]);
  scoped_array<int16_t> source_int(new int16_t[input_block_size]);
  scoped_array<int16_t> destination_int(new int16_t[output_block_size]);

  
  
  
  
  
  
  const int output_delay_samples = output_block_size -
      resampler.get_resampler_for_testing()->ChunkSize();

  
  
  
  resampler_source.Run(input_samples, source.get());
  for (int i = 0; i < kNumBlocks; ++i) {
    for (int j = 0; j < input_block_size; ++j) {
      source_int[j] = static_cast<int16_t>(floor(32767 *
          source[i * input_block_size + j] + 0.5));
    }
    EXPECT_EQ(output_block_size,
              resampler.Resample(source_int.get(), input_block_size,
                                 destination_int.get(), output_block_size));
    for (int j = 0; j < output_block_size; ++j) {
      resampled_destination[i * output_block_size + j] =
          static_cast<float>(destination_int[j]) / 32767;
    }
  }

  
  SinusoidalLinearChirpSource pure_source(
      output_rate_, output_samples, input_nyquist_freq, output_delay_samples);
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
  
  
  
  
  
  
  
  low_freq_max_error = DBFS(low_freq_max_error - 2.0 / 32767);
  high_freq_max_error = DBFS(high_freq_max_error - 2.0 / 32767);

  EXPECT_LE(rms_error, rms_error_);
  EXPECT_LE(low_freq_max_error, low_freq_error_);

  
  static const double kHighFrequencyMaxError = -6.02;
  EXPECT_LE(high_freq_max_error, kHighFrequencyMaxError);
}


static const double kResamplingRMSError = -14.42;



INSTANTIATE_TEST_CASE_P(
    PushSincResamplerTest, PushSincResamplerTest, testing::Values(
        
        
        
        
        
        

        
        std::tr1::make_tuple(8000, 44100, kResamplingRMSError, -62.73),
        std::tr1::make_tuple(16000, 44100, kResamplingRMSError, -62.54),
        std::tr1::make_tuple(32000, 44100, kResamplingRMSError, -63.32),
        std::tr1::make_tuple(44100, 44100, kResamplingRMSError, -73.53),
        std::tr1::make_tuple(48000, 44100, -15.01, -64.04),
        std::tr1::make_tuple(96000, 44100, -18.49, -25.51),
        std::tr1::make_tuple(192000, 44100, -20.50, -13.31),

        
        std::tr1::make_tuple(8000, 48000, kResamplingRMSError, -63.43),
        std::tr1::make_tuple(16000, 48000, kResamplingRMSError, -63.96),
        std::tr1::make_tuple(32000, 48000, kResamplingRMSError, -64.04),
        std::tr1::make_tuple(44100, 48000, kResamplingRMSError, -62.63),
        std::tr1::make_tuple(48000, 48000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(96000, 48000, -18.40, -28.44),
        std::tr1::make_tuple(192000, 48000, -20.43, -14.11),

        
        std::tr1::make_tuple(8000, 96000, kResamplingRMSError, -63.19),
        std::tr1::make_tuple(16000, 96000, kResamplingRMSError, -63.39),
        std::tr1::make_tuple(32000, 96000, kResamplingRMSError, -63.95),
        std::tr1::make_tuple(44100, 96000, kResamplingRMSError, -62.63),
        std::tr1::make_tuple(48000, 96000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(96000, 96000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(192000, 96000, kResamplingRMSError, -28.41),

        
        std::tr1::make_tuple(8000, 192000, kResamplingRMSError, -63.10),
        std::tr1::make_tuple(16000, 192000, kResamplingRMSError, -63.14),
        std::tr1::make_tuple(32000, 192000, kResamplingRMSError, -63.38),
        std::tr1::make_tuple(44100, 192000, kResamplingRMSError, -62.63),
        std::tr1::make_tuple(48000, 192000, kResamplingRMSError, -73.44),
        std::tr1::make_tuple(96000, 192000, kResamplingRMSError, -73.52),
        std::tr1::make_tuple(192000, 192000, kResamplingRMSError, -73.52),

        
        
        
        
        

        
        std::tr1::make_tuple(8000, 8000, kResamplingRMSError, -75.51),
        std::tr1::make_tuple(16000, 8000, -18.56, -28.79),
        std::tr1::make_tuple(32000, 8000, -20.36, -14.13),
        std::tr1::make_tuple(44100, 8000, -21.00, -11.39),
        std::tr1::make_tuple(48000, 8000, -20.96, -11.04),

        
        std::tr1::make_tuple(8000, 16000, kResamplingRMSError, -70.30),
        std::tr1::make_tuple(16000, 16000, kResamplingRMSError, -75.51),
        std::tr1::make_tuple(32000, 16000, -18.48, -28.59),
        std::tr1::make_tuple(44100, 16000, -19.30, -19.67),
        std::tr1::make_tuple(48000, 16000, -19.81, -18.11),
        std::tr1::make_tuple(96000, 16000, -20.95, -10.96),

        
        std::tr1::make_tuple(8000, 32000, kResamplingRMSError, -70.30),
        std::tr1::make_tuple(16000, 32000, kResamplingRMSError, -75.51),
        std::tr1::make_tuple(32000, 32000, kResamplingRMSError, -75.56),
        std::tr1::make_tuple(44100, 32000, -16.44, -51.10),
        std::tr1::make_tuple(48000, 32000, -16.90, -44.03),
        std::tr1::make_tuple(96000, 32000, -19.61, -18.04),
        std::tr1::make_tuple(192000, 32000, -21.02, -10.94)));

}  
