









#ifndef VP9_COMMON_VP9_LOOPFILTER_H_
#define VP9_COMMON_VP9_LOOPFILTER_H_

#include "vpx_ports/mem.h"
#include "./vpx_config.h"

#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_seg_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LOOP_FILTER 63
#define MAX_SHARPNESS 7

#define SIMD_WIDTH 16

#define MAX_REF_LF_DELTAS       4
#define MAX_MODE_LF_DELTAS      2

struct loopfilter {
  int filter_level;

  int sharpness_level;
  int last_sharpness_level;

  uint8_t mode_ref_delta_enabled;
  uint8_t mode_ref_delta_update;

  
  signed char ref_deltas[MAX_REF_LF_DELTAS];
  signed char last_ref_deltas[MAX_REF_LF_DELTAS];

  
  signed char mode_deltas[MAX_MODE_LF_DELTAS];
  signed char last_mode_deltas[MAX_MODE_LF_DELTAS];
};



typedef struct {
  DECLARE_ALIGNED(SIMD_WIDTH, uint8_t, mblim[SIMD_WIDTH]);
  DECLARE_ALIGNED(SIMD_WIDTH, uint8_t, lim[SIMD_WIDTH]);
  DECLARE_ALIGNED(SIMD_WIDTH, uint8_t, hev_thr[SIMD_WIDTH]);
} loop_filter_thresh;

typedef struct {
  loop_filter_thresh lfthr[MAX_LOOP_FILTER + 1];
  uint8_t lvl[MAX_SEGMENTS][MAX_REF_FRAMES][MAX_MODE_LF_DELTAS];
} loop_filter_info_n;










typedef struct {
  uint64_t left_y[TX_SIZES];
  uint64_t above_y[TX_SIZES];
  uint64_t int_4x4_y;
  uint16_t left_uv[TX_SIZES];
  uint16_t above_uv[TX_SIZES];
  uint16_t int_4x4_uv;
  uint8_t lfl_y[64];
  uint8_t lfl_uv[16];
} LOOP_FILTER_MASK;


struct VP9Common;
struct macroblockd;
struct VP9LfSyncData;



void vp9_setup_mask(struct VP9Common *const cm,
                    const int mi_row, const int mi_col,
                    MODE_INFO **mi_8x8, const int mode_info_stride,
                    LOOP_FILTER_MASK *lfm);

void vp9_filter_block_plane(struct VP9Common *const cm,
                            struct macroblockd_plane *const plane,
                            int mi_row,
                            LOOP_FILTER_MASK *lfm);

void vp9_loop_filter_init(struct VP9Common *cm);




void vp9_loop_filter_frame_init(struct VP9Common *cm, int default_filt_lvl);

void vp9_loop_filter_frame(struct VP9Common *cm,
                           struct macroblockd *mbd,
                           int filter_level,
                           int y_only, int partial_frame);


void vp9_loop_filter_rows(const YV12_BUFFER_CONFIG *frame_buffer,
                          struct VP9Common *cm, struct macroblockd *xd,
                          int start, int stop, int y_only);

typedef struct LoopFilterWorkerData {
  const YV12_BUFFER_CONFIG *frame_buffer;
  struct VP9Common *cm;
  struct macroblockd xd;  
                          
                          
  int start;
  int stop;
  int y_only;

  struct VP9LfSyncData *lf_sync;
  int num_lf_workers;
} LFWorkerData;


int vp9_loop_filter_worker(void *arg1, void *arg2);
#ifdef __cplusplus
}  
#endif

#endif
