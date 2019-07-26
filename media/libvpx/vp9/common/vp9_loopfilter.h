









#ifndef VP9_COMMON_VP9_LOOPFILTER_H_
#define VP9_COMMON_VP9_LOOPFILTER_H_

#include "vpx_ports/mem.h"
#include "./vpx_config.h"

#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_seg_common.h"

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
  uint8_t mode_lf_lut[MB_MODE_COUNT];
} loop_filter_info_n;


struct VP9Common;
struct macroblockd;

void vp9_loop_filter_init(struct VP9Common *cm);




void vp9_loop_filter_frame_init(struct VP9Common *cm, int default_filt_lvl);

void vp9_loop_filter_frame(struct VP9Common *cm,
                           struct macroblockd *mbd,
                           int filter_level,
                           int y_only, int partial);


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
} LFWorkerData;


int vp9_loop_filter_worker(void *arg1, void *arg2);
#endif  
