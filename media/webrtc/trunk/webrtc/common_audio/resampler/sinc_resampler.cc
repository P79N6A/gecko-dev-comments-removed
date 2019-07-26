










































#define _USE_MATH_DEFINES

#include "webrtc/common_audio/resampler/sinc_resampler.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"
#include "webrtc/typedefs.h"

#include <cmath>
#include <cstring>

#if defined(WEBRTC_USE_SSE2)
#include <xmmintrin.h>
#endif






namespace webrtc {

namespace {

enum {
  
  
  
  kKernelSize = 32,

  
  
  
  kDefaultBlockSize = 512,

  
  
  
  kKernelOffsetCount = 32,
  kKernelStorageSize = kKernelSize * (kKernelOffsetCount + 1),

  
  kDefaultBufferSize = kDefaultBlockSize + kKernelSize
};

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
      input_buffer_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * buffer_size_, 16))),
      
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
      input_buffer_(static_cast<float*>(
          AlignedMalloc(sizeof(float) * buffer_size_, 16))),
      
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
  memset(input_buffer_.get(), 0, sizeof(*input_buffer_.get()) * buffer_size_);
}

void SincResampler::InitializeKernel() {
  
  static const double kAlpha = 0.16;
  static const double kA0 = 0.5 * (1.0 - kAlpha);
  static const double kA1 = 0.5;
  static const double kA2 = 0.5 * kAlpha;

  
  
  double sinc_scale_factor =
      io_sample_rate_ratio_ > 1.0 ? 1.0 / io_sample_rate_ratio_ : 1.0;

  
  
  
  
  
  
  sinc_scale_factor *= 0.9;

  
  
  for (int offset_idx = 0; offset_idx <= kKernelOffsetCount; ++offset_idx) {
    double subsample_offset =
        static_cast<double>(offset_idx) / kKernelOffsetCount;

    for (int i = 0; i < kKernelSize; ++i) {
      
      double s =
          sinc_scale_factor * M_PI * (i - kKernelSize / 2 - subsample_offset);
      double sinc = (!s ? 1.0 : sin(s) / s) * sinc_scale_factor;

      
      double x = (i - subsample_offset) / kKernelSize;
      double window = kA0 - kA1 * cos(2.0 * M_PI * x) + kA2
          * cos(4.0 * M_PI * x);

      
      kernel_storage_.get()[i + offset_idx * kKernelSize] = sinc * window;
    }
  }
}

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

      
      float* input_ptr = r1_ + source_idx;

      
      double kernel_interpolation_factor = virtual_offset_idx - offset_idx;
      *destination++ = Convolve(
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

float SincResampler::Convolve(const float* input_ptr, const float* k1,
                              const float* k2,
                              double kernel_interpolation_factor) {
  
  
  typedef float (*ConvolveProc)(const float* src, const float* k1,
                                const float* k2,
                                double kernel_interpolation_factor);
#if defined(WEBRTC_USE_SSE2)
  static const ConvolveProc kConvolveProc =
      WebRtc_GetCPUInfo(kSSE2) ? Convolve_SSE : Convolve_C;
#elif defined(WEBRTC_ARCH_ARM_NEON)
  static const ConvolveProc kConvolveProc = Convolve_NEON;
#elif defined(WEBRTC_DETECT_ARM_NEON)
  static const ConvolveProc kConvolveProc =
      WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON ? Convolve_NEON :
                                                     Convolve_C;
#else
  static const ConvolveProc kConvolveProc = Convolve_C;
#endif

  return kConvolveProc(input_ptr, k1, k2, kernel_interpolation_factor);
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

#if defined(WEBRTC_USE_SSE2)
float SincResampler::Convolve_SSE(const float* input_ptr, const float* k1,
                                  const float* k2,
                                  double kernel_interpolation_factor) {
  
  
  assert(0u == (reinterpret_cast<uintptr_t>(k1) & 0x0F));
  assert(0u == (reinterpret_cast<uintptr_t>(k2) & 0x0F));

  __m128 m_input;
  __m128 m_sums1 = _mm_setzero_ps();
  __m128 m_sums2 = _mm_setzero_ps();

  
  
  if (reinterpret_cast<uintptr_t>(input_ptr) & 0x0F) {
    for (int i = 0; i < kKernelSize; i += 4) {
      m_input = _mm_loadu_ps(input_ptr + i);
      m_sums1 = _mm_add_ps(m_sums1, _mm_mul_ps(m_input, _mm_load_ps(k1 + i)));
      m_sums2 = _mm_add_ps(m_sums2, _mm_mul_ps(m_input, _mm_load_ps(k2 + i)));
    }
  } else {
    for (int i = 0; i < kKernelSize; i += 4) {
      m_input = _mm_load_ps(input_ptr + i);
      m_sums1 = _mm_add_ps(m_sums1, _mm_mul_ps(m_input, _mm_load_ps(k1 + i)));
      m_sums2 = _mm_add_ps(m_sums2, _mm_mul_ps(m_input, _mm_load_ps(k2 + i)));
    }
  }

  
  m_sums1 = _mm_mul_ps(m_sums1, _mm_set_ps1(1.0 - kernel_interpolation_factor));
  m_sums2 = _mm_mul_ps(m_sums2, _mm_set_ps1(kernel_interpolation_factor));
  m_sums1 = _mm_add_ps(m_sums1, m_sums2);

  
  float result;
  m_sums2 = _mm_add_ps(_mm_movehl_ps(m_sums1, m_sums1), m_sums1);
  _mm_store_ss(&result, _mm_add_ss(m_sums2, _mm_shuffle_ps(
      m_sums2, m_sums2, 1)));

  return result;
}
#endif

#if defined(WEBRTC_ARCH_ARM_NEON) || defined(WEBRTC_DETECT_ARM_NEON)
float SincResampler::Convolve_NEON(const float* input_ptr, const float* k1,
                                   const float* k2,
                                   double kernel_interpolation_factor) {
  
  
  return Convolve_C(input_ptr, k1, k2, kernel_interpolation_factor);
  
  
  

  
  
  
  
  
  
  
  
  

  
  
  
  

  
  
  
}
#endif

}  
