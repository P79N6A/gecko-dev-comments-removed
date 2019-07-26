















#include "webrtc/common_audio/signal_processing/include/real_fft.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"


MaxAbsValueW16 WebRtcSpl_MaxAbsValueW16;
MaxAbsValueW32 WebRtcSpl_MaxAbsValueW32;
MaxValueW16 WebRtcSpl_MaxValueW16;
MaxValueW32 WebRtcSpl_MaxValueW32;
MinValueW16 WebRtcSpl_MinValueW16;
MinValueW32 WebRtcSpl_MinValueW32;
CrossCorrelation WebRtcSpl_CrossCorrelation;
DownsampleFast WebRtcSpl_DownsampleFast;
ScaleAndAddVectorsWithRound WebRtcSpl_ScaleAndAddVectorsWithRound;
CreateRealFFT WebRtcSpl_CreateRealFFT;
FreeRealFFT WebRtcSpl_FreeRealFFT;
RealForwardFFT WebRtcSpl_RealForwardFFT;
RealInverseFFT WebRtcSpl_RealInverseFFT;

#if (defined(WEBRTC_DETECT_ARM_NEON) || !defined(WEBRTC_ARCH_ARM_NEON)) && \
     !defined(MIPS32_LE)

static void InitPointersToC() {
  WebRtcSpl_MaxAbsValueW16 = WebRtcSpl_MaxAbsValueW16C;
  WebRtcSpl_MaxAbsValueW32 = WebRtcSpl_MaxAbsValueW32C;
  WebRtcSpl_MaxValueW16 = WebRtcSpl_MaxValueW16C;
  WebRtcSpl_MaxValueW32 = WebRtcSpl_MaxValueW32C;
  WebRtcSpl_MinValueW16 = WebRtcSpl_MinValueW16C;
  WebRtcSpl_MinValueW32 = WebRtcSpl_MinValueW32C;
  WebRtcSpl_CrossCorrelation = WebRtcSpl_CrossCorrelationC;
  WebRtcSpl_DownsampleFast = WebRtcSpl_DownsampleFastC;
  WebRtcSpl_ScaleAndAddVectorsWithRound =
      WebRtcSpl_ScaleAndAddVectorsWithRoundC;
  WebRtcSpl_CreateRealFFT = WebRtcSpl_CreateRealFFTC;
  WebRtcSpl_FreeRealFFT = WebRtcSpl_FreeRealFFTC;
  WebRtcSpl_RealForwardFFT = WebRtcSpl_RealForwardFFTC;
  WebRtcSpl_RealInverseFFT = WebRtcSpl_RealInverseFFTC;
}
#endif

#if defined(WEBRTC_DETECT_ARM_NEON) || defined(WEBRTC_ARCH_ARM_NEON)

static void InitPointersToNeon() {
  WebRtcSpl_MaxAbsValueW16 = WebRtcSpl_MaxAbsValueW16Neon;
  WebRtcSpl_MaxAbsValueW32 = WebRtcSpl_MaxAbsValueW32Neon;
  WebRtcSpl_MaxValueW16 = WebRtcSpl_MaxValueW16Neon;
  WebRtcSpl_MaxValueW32 = WebRtcSpl_MaxValueW32Neon;
  WebRtcSpl_MinValueW16 = WebRtcSpl_MinValueW16Neon;
  WebRtcSpl_MinValueW32 = WebRtcSpl_MinValueW32Neon;
  WebRtcSpl_CrossCorrelation = WebRtcSpl_CrossCorrelationNeon;
  WebRtcSpl_DownsampleFast = WebRtcSpl_DownsampleFastNeon;
  WebRtcSpl_ScaleAndAddVectorsWithRound =
      WebRtcSpl_ScaleAndAddVectorsWithRoundNeon;
  WebRtcSpl_CreateRealFFT = WebRtcSpl_CreateRealFFTNeon;
  WebRtcSpl_FreeRealFFT = WebRtcSpl_FreeRealFFTNeon;
  WebRtcSpl_RealForwardFFT = WebRtcSpl_RealForwardFFTNeon;
  WebRtcSpl_RealInverseFFT = WebRtcSpl_RealInverseFFTNeon;
}
#endif

#if defined(MIPS32_LE)

static void InitPointersToMIPS() {
  WebRtcSpl_MaxAbsValueW16 = WebRtcSpl_MaxAbsValueW16_mips;
  WebRtcSpl_MaxValueW16 = WebRtcSpl_MaxValueW16_mips;
  WebRtcSpl_MaxValueW32 = WebRtcSpl_MaxValueW32_mips;
  WebRtcSpl_MinValueW16 = WebRtcSpl_MinValueW16_mips;
  WebRtcSpl_MinValueW32 = WebRtcSpl_MinValueW32_mips;
  WebRtcSpl_CrossCorrelation = WebRtcSpl_CrossCorrelation_mips;
  WebRtcSpl_DownsampleFast = WebRtcSpl_DownsampleFast_mips;
  WebRtcSpl_CreateRealFFT = WebRtcSpl_CreateRealFFTC;
  WebRtcSpl_FreeRealFFT = WebRtcSpl_FreeRealFFTC;
  WebRtcSpl_RealForwardFFT = WebRtcSpl_RealForwardFFTC;
  WebRtcSpl_RealInverseFFT = WebRtcSpl_RealInverseFFTC;
#if defined(MIPS_DSP_R1_LE)
  WebRtcSpl_MaxAbsValueW32 = WebRtcSpl_MaxAbsValueW32_mips;
  WebRtcSpl_ScaleAndAddVectorsWithRound =
      WebRtcSpl_ScaleAndAddVectorsWithRound_mips;
#else
  WebRtcSpl_MaxAbsValueW32 = WebRtcSpl_MaxAbsValueW32C;
  WebRtcSpl_ScaleAndAddVectorsWithRound =
      WebRtcSpl_ScaleAndAddVectorsWithRoundC;
#endif
}
#endif

static void InitFunctionPointers(void) {
#if defined(WEBRTC_DETECT_ARM_NEON)
  if ((WebRtc_GetCPUFeaturesARM() & kCPUFeatureNEON) != 0) {
    InitPointersToNeon();
  } else {
    InitPointersToC();
  }
#elif defined(WEBRTC_ARCH_ARM_NEON)
  InitPointersToNeon();
#elif defined(MIPS32_LE)
  InitPointersToMIPS();
#else
  InitPointersToC();
#endif  
}

#if defined(WEBRTC_POSIX)
#include <pthread.h>

static void once(void (*func)(void)) {
  static pthread_once_t lock = PTHREAD_ONCE_INIT;
  pthread_once(&lock, func);
}

#elif defined(_WIN32)
#include <windows.h>

static void once(void (*func)(void)) {
  






  static CRITICAL_SECTION lock = {(void *)((size_t)-1), -1, 0, 0, 0, 0};
  static int done = 0;

  EnterCriticalSection(&lock);
  if (!done) {
    func();
    done = 1;
  }
  LeaveCriticalSection(&lock);
}





#endif  

void WebRtcSpl_Init() {
  once(InitFunctionPointers);
}
