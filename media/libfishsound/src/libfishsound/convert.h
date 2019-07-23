































#ifndef __FISH_SOUND_CONVERT_H__
#define __FISH_SOUND_CONVERT_H__



static inline void
_fs_deinterleave (float ** src, float * dest[],
		  long frames, int channels, float mult_factor)
{
  int i, j;
  float * d, * s = (float *)src;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      d = dest[j];
      d[i] = s[i*channels + j] * mult_factor;
    }
  }
}

static inline void
_fs_interleave (float * src[], float ** dest,
		long frames, int channels, float mult_factor)
{
  int i, j;
  float * s, * d = (float *)dest;

  for (i = 0; i < frames; i++) {
    for (j = 0; j < channels; j++) {
      s = src[j];
      d[i*channels + j] = s[i] * mult_factor;
    }
  }
}

#endif 
