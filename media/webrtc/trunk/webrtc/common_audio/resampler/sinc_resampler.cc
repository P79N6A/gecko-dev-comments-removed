










































#define _USE_MATH_DEFINES

#include "webrtc/common_audio/resampler/sinc_resampler.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"
#include "webrtc/typedefs.h"

#include <cmath>
#include <cstring>
#include <limits>

namespace webrtc {

static double SincScaleFactor(double io_ratio) {
  
  
  double sinc_scale_factor = io_ratio > 1.0 ? 1.0 / io_ratio : 1.0;

  
  
  
  
  
  
  sinc_scale_factor *= 0.9;

  return sinc_scale_factor;
}

SincResampler::SincResampler(double io_sample_rate_ratio,
                             SincResamplerCallback* read_cb,
                             int block_size)
    : io_sample_rate_ratio_(io_sample_rate_ratio),
      virtual_source_idx_(0),
      buffer_primed_(false),
      read_cb_(read_cb),
      block_size_(block_size),
      buffer_size_(block_size_ + kKernelSize),
      
      kernel_storage_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * kKernelStorageSize, 16))),
      kernel_pre_sinc_storage_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * kKernelStorageSize, 16))),
      kernel_window_storage_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * kKernelStorageSize, 16))),
      input_buffer_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * buffer_size_, 16))),
#if defined(WEBRTC_ARCH_X86_FAMILY) && !defined(__SSE__)
      convolve_proc_(WebRtc_GetCPUInfo(kSSE2) ? Convolve_SSE : Convolve_C),
#elif defined(WEBRTC_ARCH_ARM_V7) && !defined(WEBRTC_ARCH_ARM_NEON)
      convolve_proc_(WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON ?
                     Convolve_NEON : Convolve_C),
#endif
      
      r0_(input_buffer_.get() + kKernelSize / 2),
      r1_(input_buffer_.get()),
      r2_(r0_),
      r3_(r0_ + block_size_ - kKernelSize / 2),
      r4_(r0_ + block_size_),
      r5_(r0_ + kKernelSize / 2) {
  Initialize();
  InitializeKernel();
}

SincResampler::SincResampler(double io_sample_rate_ratio,
                             SincResamplerCallback* read_cb)
    : io_sample_rate_ratio_(io_sample_rate_ratio),
      virtual_source_idx_(0),
      buffer_primed_(false),
      read_cb_(read_cb),
      block_size_(kDefaultBlockSize),
      buffer_size_(kDefaultBufferSize),
      
      kernel_storage_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * kKernelStorageSize, 16))),
      kernel_pre_sinc_storage_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * kKernelStorageSize, 16))),
      kernel_window_storage_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * kKernelStorageSize, 16))),
      input_buffer_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * buffer_size_, 16))),
#if defined(WEBRTC_ARCH_X86_FAMILY) && !defined(__SSE__)
      convolve_proc_(WebRtc_GetCPUInfo(kSSE2) ? Convolve_SSE : Convolve_C),
#elif defined(WEBRTC_ARCH_ARM_V7) && !defined(WEBRTC_ARCH_ARM_NEON)
      convolve_proc_(WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON ?
                     Convolve_NEON : Convolve_C),
#endif
      
      r0_(input_buffer_.get() + kKernelSize / 2),
      r1_(input_buffer_.get()),
      r2_(r0_),
      r3_(r0_ + block_size_ - kKernelSize / 2),
      r4_(r0_ + block_size_),
      r5_(r0_ + kKernelSize / 2) {
  Initialize();
  InitializeKernel();
}

SincResampler::~SincResampler() {}

void SincResampler::Initialize() {
  
  
  
  COMPILE_ASSERT(kKernelSize % 32 == 0);
  assert(block_size_ > kKernelSize);
  
  
  assert(r0_ == r2_);
  
  assert(r1_ == input_buffer_.get());
  
  assert(r2_ - r1_ == r5_ - r2_);
  
  assert(r4_ - r3_ == r5_ - r0_);
  
  assert(r4_ + (r4_ - r3_) == r1_ + buffer_size_);
  
  assert(r5_ + block_size_ == r1_ + buffer_size_);

  memset(kernel_storage_.get(), 0,
         sizeof(*kernel_storage_.get()) * kKernelStorageSize);
  memset(kernel_pre_sinc_storage_.get(), 0,
         sizeof(*kernel_pre_sinc_storage_.get()) * kKernelStorageSize);
  memset(kernel_window_storage_.get(), 0,
         sizeof(*kernel_window_storage_.get()) * kKernelStorageSize);
  memset(input_buffer_.get(), 0, sizeof(*input_buffer_.get()) * buffer_size_);
}

void SincResampler::InitializeKernel() {
  
  static const double kAlpha = 0.16;
  static const double kA0 = 0.5 * (1.0 - kAlpha);
  static const double kA1 = 0.5;
  static const double kA2 = 0.5 * kAlpha;

  
  
  const double sinc_scale_factor = SincScaleFactor(io_sample_rate_ratio_);
  for (int offset_idx = 0; offset_idx <= kKernelOffsetCount; ++offset_idx) {
    const float subsample_offset =
        static_cast<float>(offset_idx) / kKernelOffsetCount;

    for (int i = 0; i < kKernelSize; ++i) {
      const int idx = i + offset_idx * kKernelSize;
      const float pre_sinc = M_PI * (i - kKernelSize / 2 - subsample_offset);
      kernel_pre_sinc_storage_.get()[idx] = pre_sinc;

      
      const float x = (i - subsample_offset) / kKernelSize;
      const float window = kA0 - kA1 * cos(2.0 * M_PI * x) + kA2
          * cos(4.0 * M_PI * x);
      kernel_window_storage_.get()[idx] = window;

      
      
      if (pre_sinc == 0) {
        kernel_storage_.get()[idx] = sinc_scale_factor * window;
      } else {
        kernel_storage_.get()[idx] =
            window * sin(sinc_scale_factor * pre_sinc) / pre_sinc;
      }
    }
  }
}

void SincResampler::SetRatio(double io_sample_rate_ratio) {
  if (fabs(io_sample_rate_ratio_ - io_sample_rate_ratio) <
      std::numeric_limits<double>::epsilon()) {
    return;
  }

  io_sample_rate_ratio_ = io_sample_rate_ratio;

  
  
  const double sinc_scale_factor = SincScaleFactor(io_sample_rate_ratio_);
  for (int offset_idx = 0; offset_idx <= kKernelOffsetCount; ++offset_idx) {
    for (int i = 0; i < kKernelSize; ++i) {
      const int idx = i + offset_idx * kKernelSize;
      const float window = kernel_window_storage_.get()[idx];
      const float pre_sinc = kernel_pre_sinc_storage_.get()[idx];

      if (pre_sinc == 0) {
        kernel_storage_.get()[idx] = sinc_scale_factor * window;
      } else {
        kernel_storage_.get()[idx] =
            window * sin(sinc_scale_factor * pre_sinc) / pre_sinc;
      }
    }
  }
}


#if defined(WEBRTC_ARCH_X86_FAMILY)
#if defined(__SSE__)
#define CONVOLVE_FUNC Convolve_SSE
#else


#define CONVOLVE_FUNC convolve_proc_
#endif
#elif defined(WEBRTC_ARCH_ARM_V7)
#if defined(WEBRTC_ARCH_ARM_NEON)
#define CONVOLVE_FUNC Convolve_NEON
#else

#define CONVOLVE_FUNC convolve_proc_
#endif
#else

#define CONVOLVE_FUNC Convolve_C
#endif

void SincResampler::Resample(float* destination, int frames) {
  int remaining_frames = frames;

  
  if (!buffer_primed_) {
    read_cb_->Run(r0_, block_size_ + kKernelSize / 2);
    buffer_primed_ = true;
  }

  
  while (remaining_frames) {
    while (virtual_source_idx_ < block_size_) {
      
      
      int source_idx = static_cast<int>(virtual_source_idx_);
      double subsample_remainder = virtual_source_idx_ - source_idx;

      double virtual_offset_idx = subsample_remainder * kKernelOffsetCount;
      int offset_idx = static_cast<int>(virtual_offset_idx);

      
      
      float* k1 = kernel_storage_.get() + offset_idx * kKernelSize;
      float* k2 = k1 + kKernelSize;

      
      
      assert((reinterpret_cast<uintptr_t>(k1) & 0x0F) == 0u);
      assert((reinterpret_cast<uintptr_t>(k2) & 0x0F) == 0u);

      
      float* input_ptr = r1_ + source_idx;

      
      double kernel_interpolation_factor = virtual_offset_idx - offset_idx;
      *destination++ = CONVOLVE_FUNC(
          input_ptr, k1, k2, kernel_interpolation_factor);

      
      virtual_source_idx_ += io_sample_rate_ratio_;

      if (!--remaining_frames)
        return;
    }

    
    virtual_source_idx_ -= block_size_;

    
    
    memcpy(r1_, r3_, sizeof(*input_buffer_.get()) * (kKernelSize / 2));
    memcpy(r2_, r4_, sizeof(*input_buffer_.get()) * (kKernelSize / 2));

    
    
    read_cb_->Run(r5_, block_size_);
  }
}

#undef CONVOLVE_FUNC

int SincResampler::ChunkSize() {
  return block_size_ / io_sample_rate_ratio_;
}

int SincResampler::BlockSize() {
  return block_size_;
}

void SincResampler::Flush() {
  virtual_source_idx_ = 0;
  buffer_primed_ = false;
  memset(input_buffer_.get(), 0, sizeof(*input_buffer_.get()) * buffer_size_);
}

float SincResampler::Convolve_C(const float* input_ptr, const float* k1,
                                const float* k2,
                                double kernel_interpolation_factor) {
  float sum1 = 0;
  float sum2 = 0;

  
  
  int n = kKernelSize;
  while (n--) {
    sum1 += *input_ptr * *k1++;
    sum2 += *input_ptr++ * *k2++;
  }

  
  return (1.0 - kernel_interpolation_factor) * sum1
      + kernel_interpolation_factor * sum2;
}

}  
