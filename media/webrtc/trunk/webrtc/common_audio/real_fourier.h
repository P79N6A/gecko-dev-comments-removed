









#ifndef WEBRTC_COMMON_AUDIO_REAL_FOURIER_H_
#define WEBRTC_COMMON_AUDIO_REAL_FOURIER_H_

#include <complex>

#include "webrtc/system_wrappers/interface/aligned_malloc.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"





namespace webrtc {

class RealFourier {
 public:
  
  typedef scoped_ptr<float[], AlignedFreeDeleter> fft_real_scoper;
  typedef scoped_ptr<std::complex<float>[], AlignedFreeDeleter> fft_cplx_scoper;

  
  static const int kMaxFftOrder;

  
  static const int kFftBufferAlignment;

  
  
  explicit RealFourier(int fft_order);
  ~RealFourier();

  
  
  
  static int FftOrder(int length);

  
  
  static int ComplexLength(int order);

  
  
  
  
  static fft_real_scoper AllocRealBuffer(int count);
  static fft_cplx_scoper AllocCplxBuffer(int count);

  
  
  
  
  
  void Forward(const float* src, std::complex<float>* dest) const;

  
  
  void Inverse(const std::complex<float>* src, float* dest) const;

  int order() const {
    return order_;
  }

 private:
  
  
  typedef void OMXFFTSpec_R_F32_;
  const int order_;

  OMXFFTSpec_R_F32_* omx_spec_;
};

}  

#endif  

