










#ifndef VP9_COMMON_VP9_POSTPROC_H_
#define VP9_COMMON_VP9_POSTPROC_H_

#include "vpx_ports/mem.h"

struct postproc_state {
  int last_q;
  int last_noise;
  char noise[3072];
  DECLARE_ALIGNED(16, char, blackclamp[16]);
  DECLARE_ALIGNED(16, char, whiteclamp[16]);
  DECLARE_ALIGNED(16, char, bothclamp[16]);
};

#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/common/vp9_ppflags.h"

int vp9_post_proc_frame(struct VP9Common *cm,
                        YV12_BUFFER_CONFIG *dest, vp9_ppflags_t *flags);

void vp9_denoise(const YV12_BUFFER_CONFIG *src, YV12_BUFFER_CONFIG *dst, int q);

void vp9_deblock(const YV12_BUFFER_CONFIG *src, YV12_BUFFER_CONFIG *dst, int q);

#endif  
