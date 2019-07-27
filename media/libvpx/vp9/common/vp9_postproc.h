










#ifndef VP9_COMMON_VP9_POSTPROC_H_
#define VP9_COMMON_VP9_POSTPROC_H_

#include "vpx_ports/mem.h"
#include "vpx_scale/yv12config.h"
#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_mfqe.h"
#include "vp9/common/vp9_ppflags.h"

#ifdef __cplusplus
extern "C" {
#endif

struct postproc_state {
  int last_q;
  int last_noise;
  char noise[3072];
  int last_base_qindex;
  int last_frame_valid;
  MODE_INFO *prev_mip;
  MODE_INFO *prev_mi;
  DECLARE_ALIGNED(16, char, blackclamp[16]);
  DECLARE_ALIGNED(16, char, whiteclamp[16]);
  DECLARE_ALIGNED(16, char, bothclamp[16]);
};

struct VP9Common;

#define MFQE_PRECISION 4

int vp9_post_proc_frame(struct VP9Common *cm,
                        YV12_BUFFER_CONFIG *dest, vp9_ppflags_t *flags);

void vp9_denoise(const YV12_BUFFER_CONFIG *src, YV12_BUFFER_CONFIG *dst, int q);

void vp9_deblock(const YV12_BUFFER_CONFIG *src, YV12_BUFFER_CONFIG *dst, int q);

#ifdef __cplusplus
}  
#endif

#endif
