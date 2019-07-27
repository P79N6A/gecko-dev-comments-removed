









#include "webrtc/common_audio/real_fourier.h"

#include <cstdlib>

#include "third_party/openmax_dl/dl/sp/api/omxSP.h"
#include "webrtc/base/checks.h"

namespace webrtc {

using std::complex;


const int RealFourier::kMaxFftOrder = TWIDDLE_TABLE_ORDER;
const int RealFourier::kFftBufferAlignment = 32;

RealFourier::RealFourier(int fft_order)
    : order_(fft_order),
      omx_spec_(nullptr) {
  CHECK_GE(order_, 1);
  CHECK_LE(order_, kMaxFftOrder);

  OMX_INT buffer_size;
  OMXResult r;

  r = omxSP_FFTGetBufSize_R_F32(order_, &buffer_size);
  CHECK_EQ(r, OMX_Sts_NoErr);

  omx_spec_ = malloc(buffer_size);
  DCHECK(omx_spec_);

  r = omxSP_FFTInit_R_F32(omx_spec_, order_);
  CHECK_EQ(r, OMX_Sts_NoErr);
}

RealFourier::~RealFourier() {
  free(omx_spec_);
}

int RealFourier::FftOrder(int length) {
  for (int order = 0; order <= kMaxFftOrder; order++) {
    if ((1 << order) >= length) {
      return order;
    }
  }
  return -1;
}

int RealFourier::ComplexLength(int order) {
  CHECK_LE(order, kMaxFftOrder);
  CHECK_GT(order, 0);
  return (1 << order) / 2 + 1;
}

RealFourier::fft_real_scoper RealFourier::AllocRealBuffer(int count) {
  return fft_real_scoper(static_cast<float*>(
      AlignedMalloc(sizeof(float) * count, kFftBufferAlignment)));
}

RealFourier::fft_cplx_scoper RealFourier::AllocCplxBuffer(int count) {
  return fft_cplx_scoper(static_cast<complex<float>*>(
      AlignedMalloc(sizeof(complex<float>) * count, kFftBufferAlignment)));
}

void RealFourier::Forward(const float* src, complex<float>* dest) const {
  OMXResult r;
  r = omxSP_FFTFwd_RToCCS_F32(src, reinterpret_cast<OMX_F32*>(dest), omx_spec_);
  CHECK_EQ(r, OMX_Sts_NoErr);
}

void RealFourier::Inverse(const complex<float>* src, float* dest) const {
  OMXResult r;
  r = omxSP_FFTInv_CCSToR_F32(reinterpret_cast<const OMX_F32*>(src), dest,
                              omx_spec_);
  CHECK_EQ(r, OMX_Sts_NoErr);
}

}  

