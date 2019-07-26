












#ifndef WEBRTC_COMMON_AUDIO_RESAMPLER_SINC_RESAMPLER_H_
#define WEBRTC_COMMON_AUDIO_RESAMPLER_SINC_RESAMPLER_H_

#include "webrtc/system_wrappers/interface/aligned_malloc.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/gtest_prod_util.h"
#include "webrtc/typedefs.h"

namespace webrtc {




class SincResamplerCallback {
 public:
  virtual ~SincResamplerCallback() {}
  virtual void Run(float* destination, int frames) = 0;
};


class SincResampler {
 public:
  enum {
    
    
    
    kKernelSize = 32,

    
    
    
    kDefaultBlockSize = 512,

    
    
    
    kKernelOffsetCount = 32,
    kKernelStorageSize = kKernelSize * (kKernelOffsetCount + 1),

    
    kDefaultBufferSize = kDefaultBlockSize + kKernelSize,
  };

  
  
  
  
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

  
  
  
  
  
  
  void SetRatio(double io_sample_rate_ratio);

  float* get_kernel_for_testing() { return kernel_storage_.get(); }

 private:
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, Convolve);
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, ConvolveBenchmark);

  void Initialize();
  void InitializeKernel();

  
  
  
  
  static float Convolve_C(const float* input_ptr, const float* k1,
                          const float* k2, double kernel_interpolation_factor);
#if defined(WEBRTC_ARCH_X86_FAMILY)
  static float Convolve_SSE(const float* input_ptr, const float* k1,
                            const float* k2,
                            double kernel_interpolation_factor);
#elif defined(WEBRTC_ARCH_ARM_V7)
  static float Convolve_NEON(const float* input_ptr, const float* k1,
                             const float* k2,
                             double kernel_interpolation_factor);
#endif

  
  double io_sample_rate_ratio_;

  
  
  double virtual_source_idx_;

  
  bool buffer_primed_;

  
  SincResamplerCallback* read_cb_;

  
  int block_size_;

  
  int buffer_size_;

  
  
  
  scoped_ptr_malloc<float, AlignedFree> kernel_storage_;
  scoped_ptr_malloc<float, AlignedFree> kernel_pre_sinc_storage_;
  scoped_ptr_malloc<float, AlignedFree> kernel_window_storage_;

  
  scoped_ptr_malloc<float, AlignedFree> input_buffer_;

  
#if (defined(WEBRTC_ARCH_X86_FAMILY) && !defined(__SSE__)) ||  \
    (defined(WEBRTC_ARCH_ARM_V7) && !defined(WEBRTC_ARCH_ARM_NEON))
  typedef float (*ConvolveProc)(const float*, const float*, const float*,
                                double);
  const ConvolveProc convolve_proc_;
#endif

  
  
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
