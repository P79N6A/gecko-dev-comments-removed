









#ifndef VP8_ENCODER_DENOISING_H_
#define VP8_ENCODER_DENOISING_H_

#include "block.h"
#include "vp8/common/loopfilter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SUM_DIFF_THRESHOLD (16 * 16 * 2)
#define SUM_DIFF_THRESHOLD_HIGH (600)  // ~(16 * 16 * 1.5)
#define MOTION_MAGNITUDE_THRESHOLD (8*3)

#define SUM_DIFF_THRESHOLD_UV (96)   // (8 * 8 * 1.5)
#define SUM_DIFF_THRESHOLD_HIGH_UV (8 * 8 * 2)
#define SUM_DIFF_FROM_AVG_THRESH_UV (8 * 8 * 8)
#define MOTION_MAGNITUDE_THRESHOLD_UV (8*3)

#define MAX_GF_ARF_DENOISE_RANGE (8)

enum vp8_denoiser_decision
{
  COPY_BLOCK,
  FILTER_BLOCK
};

enum vp8_denoiser_filter_state {
  kNoFilter,
  kFilterZeroMV,
  kFilterNonZeroMV
};

enum vp8_denoiser_mode {
  kDenoiserOff,
  kDenoiserOnYOnly,
  kDenoiserOnYUV,
  kDenoiserOnYUVAggressive,
  kDenoiserOnAdaptive
};

typedef struct {
  
  unsigned int scale_sse_thresh;
  
  
  unsigned int scale_motion_thresh;
  
  
  unsigned int scale_increase_filter;
  
  unsigned int denoise_mv_bias;
  
  unsigned int pickmode_mv_bias;
  
  
  
  
  unsigned int qp_thresh;
  
  unsigned int consec_zerolast;
  
  unsigned int spatial_blur;
} denoise_params;

typedef struct vp8_denoiser
{
    YV12_BUFFER_CONFIG yv12_running_avg[MAX_REF_FRAMES];
    YV12_BUFFER_CONFIG yv12_mc_running_avg;
    
    YV12_BUFFER_CONFIG yv12_last_source;
    unsigned char* denoise_state;
    int num_mb_cols;
    int denoiser_mode;
    int threshold_aggressive_mode;
    int nmse_source_diff;
    int nmse_source_diff_count;
    int qp_avg;
    int qp_threshold_up;
    int qp_threshold_down;
    int bitrate_threshold;
    denoise_params denoise_pars;
} VP8_DENOISER;

int vp8_denoiser_allocate(VP8_DENOISER *denoiser, int width, int height,
                          int num_mb_rows, int num_mb_cols, int mode);

void vp8_denoiser_free(VP8_DENOISER *denoiser);

void vp8_denoiser_set_parameters(VP8_DENOISER *denoiser, int mode);

void vp8_denoiser_denoise_mb(VP8_DENOISER *denoiser,
                             MACROBLOCK *x,
                             unsigned int best_sse,
                             unsigned int zero_mv_sse,
                             int recon_yoffset,
                             int recon_uvoffset,
                             loop_filter_info_n *lfi_n,
                             int mb_row,
                             int mb_col,
                             int block_index);

#ifdef __cplusplus
}  
#endif

#endif
