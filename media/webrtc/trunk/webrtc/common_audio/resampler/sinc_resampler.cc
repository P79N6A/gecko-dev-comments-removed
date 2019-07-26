




















































































#define _USE_MATH_DEFINES

#include "webrtc/common_audio/resampler/sinc_resampler.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"
#include "webrtc/typedefs.h"

#include <math.h>
#include <string.h>

#include <limits>

namespace webrtc {

static double SincScaleFactor(double io_ratio) {
  
  
  double sinc_scale_factor = io_ratio > 1.0 ? 1.0 / io_ratio : 1.0;

  
  
  
  
  
  
  sinc_scale_factor *= 0.9;

  return sinc_scale_factor;
}



#if defined(WEBRTC_ARCH_X86_FAMILY) && !defined(WEBRTC_IOS)
#if defined(__SSE__)
#define CONVOLVE_FUNC Convolve_SSE
void SincResampler::InitializeCPUSpecificFeatures() {}
#else



#define CONVOLVE_FUNC convolve_proc_

void SincResampler::InitializeCPUSpecificFeatures() {
  convolve_proc_ = WebRtc_GetCPUInfo(kSSE2) ? Convolve_SSE : Convolve_C;
}
#endif
#elif defined(WEBRTC_ARCH_ARM_V7)
#if defined(WEBRTC_ARCH_ARM_NEON)
#define CONVOLVE_FUNC Convolve_NEON
void SincResampler::InitializeCPUSpecificFeatures() {}
#else


#define CONVOLVE_FUNC convolve_proc_

void SincResampler::InitializeCPUSpecificFeatures() {
  convolve_proc_ = WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON ?
      Convolve_NEON : Convolve_C;
}
#endif
#else

#define CONVOLVE_FUNC Convolve_C
void SincResampler::InitializeCPUSpecificFeatures() {}
#endif

SincResampler::SincResampler(double io_sample_rate_ratio,
                             int request_frames,
                             SincResamplerCallback* read_cb)
    : io_sample_rate_ratio_(io_sample_rate_ratio),
      read_cb_(read_cb),
      request_frames_(request_frames),
      input_buffer_size_(request_frames_ + kKernelSize),
      
      kernel_storage_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * kKernelStorageSize, 16))),
      kernel_pre_sinc_storage_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * kKernelStorageSize, 16))),
      kernel_window_storage_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * kKernelStorageSize, 16))),
      input_buffer_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * input_buffer_size_, 16))),
#if defined(WEBRTC_RESAMPLER_CPU_DETECTION)
      convolve_proc_(NULL),
#endif
      r1_(input_buffer_.get()),
      r2_(input_buffer_.get() + kKernelSize / 2) {
#if defined(WEBRTC_RESAMPLER_CPU_DETECTION)
  InitializeCPUSpecificFeatures();
  assert(convolve_proc_);
#endif
  assert(request_frames_ > 0);
  Flush();
  assert(block_size_ > kKernelSize);

  memset(kernel_storage_.get(), 0,
         sizeof(*kernel_storage_.get()) * kKernelStorageSize);
  memset(kernel_pre_sinc_storage_.get(), 0,
         sizeof(*kernel_pre_sinc_storage_.get()) * kKernelStorageSize);
  memset(kernel_window_storage_.get(), 0,
         sizeof(*kernel_window_storage_.get()) * kKernelStorageSize);

  InitializeKernel();
}

SincResampler::~SincResampler() {}

void SincResampler::UpdateRegions(bool second_load) {
  
  
  r0_ = input_buffer_.get() + (second_load ? kKernelSize : kKernelSize / 2);
  r3_ = r0_ + request_frames_ - kKernelSize;
  r4_ = r0_ + request_frames_ - kKernelSize / 2;
  block_size_ = r4_ - r2_;

  
  assert(r1_ == input_buffer_.get());
  
  assert(r2_ - r1_ == r4_ - r3_);
  
  assert(r2_ < r3_);
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

void SincResampler::Resample(int frames, float* destination) {
  int remaining_frames = frames;

  
  if (!buffer_primed_ && remaining_frames) {
    read_cb_->Run(request_frames_, r0_);
    buffer_primed_ = true;
  }

  
  
  const double current_io_ratio = io_sample_rate_ratio_;
  const float* const kernel_ptr = kernel_storage_.get();
  while (remaining_frames) {
    
    
    
    
    
    for (int i = ceil((block_size_ - virtual_source_idx_) / current_io_ratio);
         i > 0; --i) {
      assert(virtual_source_idx_ < block_size_);

      
      
      const int source_idx = virtual_source_idx_;
      const double subsample_remainder = virtual_source_idx_ - source_idx;

      const double virtual_offset_idx =
          subsample_remainder * kKernelOffsetCount;
      const int offset_idx = virtual_offset_idx;

      
      
      const float* const k1 = kernel_ptr + offset_idx * kKernelSize;
      const float* const k2 = k1 + kKernelSize;

      
      
      assert(0u == (reinterpret_cast<uintptr_t>(k1) & 0x0F));
      assert(0u == (reinterpret_cast<uintptr_t>(k2) & 0x0F));

      
      const float* const input_ptr = r1_ + source_idx;

      
      const double kernel_interpolation_factor =
          virtual_offset_idx - offset_idx;
      *destination++ = CONVOLVE_FUNC(
          input_ptr, k1, k2, kernel_interpolation_factor);

      
      virtual_source_idx_ += current_io_ratio;

      if (!--remaining_frames)
        return;
    }

    
    virtual_source_idx_ -= block_size_;

    
    
    memcpy(r1_, r3_, sizeof(*input_buffer_.get()) * kKernelSize);

    
    if (r0_ == r2_)
      UpdateRegions(true);

    
    read_cb_->Run(request_frames_, r0_);
  }
}

#undef CONVOLVE_FUNC

int SincResampler::ChunkSize() const {
  return block_size_ / io_sample_rate_ratio_;
}

void SincResampler::Flush() {
  virtual_source_idx_ = 0;
  buffer_primed_ = false;
  memset(input_buffer_.get(), 0,
         sizeof(*input_buffer_.get()) * input_buffer_size_);
  UpdateRegions(false);
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
