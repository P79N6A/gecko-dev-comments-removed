









#ifndef WEBRTC_COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_REAL_FFT_H_
#define WEBRTC_COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_REAL_FFT_H_

#include "webrtc/typedefs.h"




enum {kMaxFFTOrder = 10};

struct RealFFT;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RealFFT* (*CreateRealFFT)(int order);
typedef void (*FreeRealFFT)(struct RealFFT* self);
typedef int (*RealForwardFFT)(struct RealFFT* self,
                              const int16_t* real_data_in,
                              int16_t* complex_data_out);
typedef int (*RealInverseFFT)(struct RealFFT* self,
                              const int16_t* complex_data_in,
                              int16_t* real_data_out);

extern CreateRealFFT WebRtcSpl_CreateRealFFT;
extern FreeRealFFT WebRtcSpl_FreeRealFFT;
extern RealForwardFFT WebRtcSpl_RealForwardFFT;
extern RealInverseFFT WebRtcSpl_RealInverseFFT;

struct RealFFT* WebRtcSpl_CreateRealFFTC(int order);
void WebRtcSpl_FreeRealFFTC(struct RealFFT* self);

#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
struct RealFFT* WebRtcSpl_CreateRealFFTNeon(int order);
void WebRtcSpl_FreeRealFFTNeon(struct RealFFT* self);
#endif
































int WebRtcSpl_RealForwardFFTC(struct RealFFT* self,
                              const int16_t* real_data_in,
                              int16_t* complex_data_out);

#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int WebRtcSpl_RealForwardFFTNeon(struct RealFFT* self,
                                 const int16_t* real_data_in,
                                 int16_t* complex_data_out);
#endif

























int WebRtcSpl_RealInverseFFTC(struct RealFFT* self,
                              const int16_t* complex_data_in,
                              int16_t* real_data_out);

#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int WebRtcSpl_RealInverseFFTNeon(struct RealFFT* self,
                                 const int16_t* complex_data_in,
                                 int16_t* real_data_out);
#endif

#ifdef __cplusplus
}
#endif

#endif
