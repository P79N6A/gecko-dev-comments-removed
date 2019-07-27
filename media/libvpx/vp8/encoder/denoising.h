









#ifndef VP8_ENCODER_DENOISING_H_
#define VP8_ENCODER_DENOISING_H_

#include "block.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SUM_DIFF_THRESHOLD (16 * 16 * 2)
#define MOTION_MAGNITUDE_THRESHOLD (8*3)

enum vp8_denoiser_decision
{
  COPY_BLOCK,
  FILTER_BLOCK
};

typedef struct vp8_denoiser
{
    YV12_BUFFER_CONFIG yv12_running_avg[MAX_REF_FRAMES];
    YV12_BUFFER_CONFIG yv12_mc_running_avg;
} VP8_DENOISER;

int vp8_denoiser_allocate(VP8_DENOISER *denoiser, int width, int height);

void vp8_denoiser_free(VP8_DENOISER *denoiser);

void vp8_denoiser_denoise_mb(VP8_DENOISER *denoiser,
                             MACROBLOCK *x,
                             unsigned int best_sse,
                             unsigned int zero_mv_sse,
                             int recon_yoffset,
                             int recon_uvoffset);

#ifdef __cplusplus
}  
#endif

#endif
