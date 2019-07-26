












#ifndef WEBRTC_COMMON_AUDIO_RESAMPLER_SINC_RESAMPLER_H_
#define WEBRTC_COMMON_AUDIO_RESAMPLER_SINC_RESAMPLER_H_

#include "webrtc/system_wrappers/interface/aligned_malloc.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/gtest_prod_util.h"

namespace webrtc {


class SincResamplerCallback {
 public:
  virtual ~SincResamplerCallback() {}
  virtual void Run(float* destination, int frames) = 0;
};


class SincResampler {
 public:
  
  
  
  
  SincResampler(double io_sample_rate_ratio,
                SincResamplerCallback* read_cb);
  SincResampler(double io_sample_rate_ratio,
                SincResamplerCallback* read_cb,
                int block_size);
  virtual ~SincResampler();

  
  void Resample(float* destination, int frames);

  
  
  int ChunkSize();

  
  
  
  int BlockSize();

  
  void Flush();

 private:
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, Convolve);
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, ConvolveBenchmark);

  void Initialize();
  void InitializeKernel();

  
  
  
  
  static float Convolve(const float* input_ptr, const float* k1,
                        const float* k2, double kernel_interpolation_factor);
  static float Convolve_C(const float* input_ptr, const float* k1,
                          const float* k2, double kernel_interpolation_factor);
  static float Convolve_SSE(const float* input_ptr, const float* k1,
                            const float* k2,
                            double kernel_interpolation_factor);
  static float Convolve_NEON(const float* input_ptr, const float* k1,
                             const float* k2,
                             double kernel_interpolation_factor);

  
  double io_sample_rate_ratio_;

  
  
  double virtual_source_idx_;

  
  bool buffer_primed_;

  
  SincResamplerCallback* read_cb_;

  
  int block_size_;

  
  int buffer_size_;

  
  
  
  scoped_ptr_malloc<float, AlignedFree> kernel_storage_;

  
  scoped_ptr_malloc<float, AlignedFree> input_buffer_;

  
  
  float* const r0_;
  float* const r1_;
  float* const r2_;
  float* const r3_;
  float* const r4_;
  float* const r5_;

  DISALLOW_COPY_AND_ASSIGN(SincResampler);
};

}  

#endif  
