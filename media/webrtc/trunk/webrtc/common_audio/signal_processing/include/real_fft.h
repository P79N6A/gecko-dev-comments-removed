









#ifndef WEBRTC_COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_REAL_FFT_H_
#define WEBRTC_COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_REAL_FFT_H_

#include "typedefs.h"

struct RealFFT;

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*RealForwardFFT)(struct RealFFT* self,
                              const int16_t* data_in,
                              int16_t* data_out);
typedef int (*RealInverseFFT)(struct RealFFT* self,
                              const int16_t* data_in,
                              int16_t* data_out);

extern RealForwardFFT WebRtcSpl_RealForwardFFT;
extern RealInverseFFT WebRtcSpl_RealInverseFFT;

struct RealFFT* WebRtcSpl_CreateRealFFT(int order);
void WebRtcSpl_FreeRealFFT(struct RealFFT* self);















int WebRtcSpl_RealForwardFFTC(struct RealFFT* self,
                              const int16_t* data_in,
                              int16_t* data_out);

#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int WebRtcSpl_RealForwardFFTNeon(struct RealFFT* self,
                                 const int16_t* data_in,
                                 int16_t* data_out);
#endif














int WebRtcSpl_RealInverseFFTC(struct RealFFT* self,
                              const int16_t* data_in,
                              int16_t* data_out);

#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int WebRtcSpl_RealInverseFFTNeon(struct RealFFT* self,
                                 const int16_t* data_in,
                                 int16_t* data_out);
#endif

#ifdef __cplusplus
}
#endif

#endif
