












#ifndef WEBRTC_COMMON_AUDIO_RESAMPLER_SINC_RESAMPLER_H_
#define WEBRTC_COMMON_AUDIO_RESAMPLER_SINC_RESAMPLER_H_

#include "webrtc/system_wrappers/interface/aligned_malloc.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/gtest_prod_util.h"
#include "webrtc/typedefs.h"

#if (defined(WEBRTC_ARCH_X86_FAMILY) && !defined(WEBRTC_IOS) &&  \
        !defined(__SSE__)) ||  \
    (defined(WEBRTC_ARCH_ARM_V7) && !defined(WEBRTC_ARCH_ARM_NEON))

#define WEBRTC_RESAMPLER_CPU_DETECTION
#endif

namespace webrtc {




class SincResamplerCallback {
 public:
  virtual ~SincResamplerCallback() {}
  virtual void Run(int frames, float* destination) = 0;
};


class SincResampler {
 public:
  enum {
    
    
    
    kKernelSize = 32,

    
    
    kDefaultRequestSize = 512,

    
    
    
    kKernelOffsetCount = 32,
    kKernelStorageSize = kKernelSize * (kKernelOffsetCount + 1),
  };

  
  
  
  
  
  
  SincResampler(double io_sample_rate_ratio,
                int request_frames,
                SincResamplerCallback* read_cb);
  virtual ~SincResampler();

  
  void Resample(int frames, float* destination);

  
  
  int ChunkSize() const;

  int request_frames() const { return request_frames_; }

  
  
  void Flush();

  
  
  
  
  
  
  void SetRatio(double io_sample_rate_ratio);

  float* get_kernel_for_testing() { return kernel_storage_.get(); }

 private:
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, Convolve);
  FRIEND_TEST_ALL_PREFIXES(SincResamplerTest, ConvolveBenchmark);

  void InitializeKernel();
  void UpdateRegions(bool second_load);

  
  
  
  
  void InitializeCPUSpecificFeatures();

  
  
  
  
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

  
  const int request_frames_;

  
  int block_size_;

  
  const int input_buffer_size_;

  
  
  
  scoped_ptr_malloc<float, AlignedFree> kernel_storage_;
  scoped_ptr_malloc<float, AlignedFree> kernel_pre_sinc_storage_;
  scoped_ptr_malloc<float, AlignedFree> kernel_window_storage_;

  
  scoped_ptr_malloc<float, AlignedFree> input_buffer_;

  
  
  
  
#if defined(WEBRTC_RESAMPLER_CPU_DETECTION)
  typedef float (*ConvolveProc)(const float*, const float*, const float*,
                                double);
  ConvolveProc convolve_proc_;
#endif

  
  
  float* r0_;
  float* const r1_;
  float* const r2_;
  float* r3_;
  float* r4_;

  DISALLOW_COPY_AND_ASSIGN(SincResampler);
};

}  

#endif
