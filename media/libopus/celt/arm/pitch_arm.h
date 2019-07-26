


























#if !defined(PITCH_ARM_H)
# define PITCH_ARM_H

# include "armcpu.h"

# if defined(FIXED_POINT)

#  if defined(OPUS_ARM_MAY_HAVE_NEON)
opus_val32 celt_pitch_xcorr_neon(const opus_val16 *_x, const opus_val16 *_y,
    opus_val32 *xcorr, int len, int max_pitch);
#  endif

#  if defined(OPUS_ARM_MAY_HAVE_MEDIA)
#   define celt_pitch_xcorr_media MAY_HAVE_EDSP(celt_pitch_xcorr)
#  endif

#  if defined(OPUS_ARM_MAY_HAVE_EDSP)
opus_val32 celt_pitch_xcorr_edsp(const opus_val16 *_x, const opus_val16 *_y,
    opus_val32 *xcorr, int len, int max_pitch);
#  endif

#  if !defined(OPUS_HAVE_RTCD)
#   define OVERRIDE_PITCH_XCORR (1)
#   define celt_pitch_xcorr(_x, _y, xcorr, len, max_pitch, arch) \
  ((void)(arch),PRESUME_NEON(celt_pitch_xcorr)(_x, _y, xcorr, len, max_pitch))
#  endif

# endif

#endif
