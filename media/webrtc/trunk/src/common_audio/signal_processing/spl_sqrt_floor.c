






#include "signal_processing_library.h"















#define WEBRTC_SPL_SQRT_ITER(N)                 \
  try1 = root + (1 << (N));                     \
  if (value >= try1 << (N))                     \
  {                                             \
    value -= try1 << (N);                       \
    root |= 2 << (N);                           \
  }

int32_t WebRtcSpl_SqrtFloor(int32_t value)
{
  int32_t root = 0, try1;

  WEBRTC_SPL_SQRT_ITER (15);
  WEBRTC_SPL_SQRT_ITER (14);
  WEBRTC_SPL_SQRT_ITER (13);
  WEBRTC_SPL_SQRT_ITER (12);
  WEBRTC_SPL_SQRT_ITER (11);
  WEBRTC_SPL_SQRT_ITER (10);
  WEBRTC_SPL_SQRT_ITER ( 9);
  WEBRTC_SPL_SQRT_ITER ( 8);
  WEBRTC_SPL_SQRT_ITER ( 7);
  WEBRTC_SPL_SQRT_ITER ( 6);
  WEBRTC_SPL_SQRT_ITER ( 5);
  WEBRTC_SPL_SQRT_ITER ( 4);
  WEBRTC_SPL_SQRT_ITER ( 3);
  WEBRTC_SPL_SQRT_ITER ( 2);
  WEBRTC_SPL_SQRT_ITER ( 1);
  WEBRTC_SPL_SQRT_ITER ( 0);

  return root >> 1;
}
