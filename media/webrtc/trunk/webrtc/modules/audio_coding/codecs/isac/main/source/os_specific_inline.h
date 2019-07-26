










#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_OS_SPECIFIC_INLINE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_OS_SPECIFIC_INLINE_H_

#include <math.h>
#include "typedefs.h"


#if defined(WEBRTC_LINUX) || defined(WEBRTC_MAC)
#define WebRtcIsac_lrint lrint
#elif (defined(WEBRTC_ARCH_X86) && defined(WIN32))
static __inline long int WebRtcIsac_lrint(double x_dbl) {
  long int x_int;

  __asm {
    fld x_dbl
    fistp x_int
  };

  return x_int;
}
#else

static __inline long int WebRtcIsac_lrint(double x_dbl) {
  long int x_int;
  x_int = (long int)floor(x_dbl + 0.499999999999);
  return x_int;
}

#endif

#endif
