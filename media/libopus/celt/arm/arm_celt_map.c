


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pitch.h"

#if defined(OPUS_HAVE_RTCD)

# if defined(FIXED_POINT)
opus_val32 (*const CELT_PITCH_XCORR_IMPL[OPUS_ARCHMASK+1])(const opus_val16 *,
    const opus_val16 *, opus_val32 *, int , int) = {
  celt_pitch_xcorr_c,               
  MAY_HAVE_EDSP(celt_pitch_xcorr),  
  MAY_HAVE_MEDIA(celt_pitch_xcorr), 
  MAY_HAVE_NEON(celt_pitch_xcorr)   
};
# else
#  error "Floating-point implementation is not supported by ARM asm yet." \
 "Reconfigure with --disable-rtcd or send patches."
# endif

#endif
