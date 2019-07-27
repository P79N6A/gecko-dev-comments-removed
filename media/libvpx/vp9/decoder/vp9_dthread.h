









#ifndef VP9_DECODER_VP9_DTHREAD_H_
#define VP9_DECODER_VP9_DTHREAD_H_

#include "./vpx_config.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/decoder/vp9_reader.h"
#include "vp9/decoder/vp9_thread.h"

struct macroblockd;
struct VP9Common;
struct VP9Decompressor;

typedef struct TileWorkerData {
  struct VP9Common *cm;
  vp9_reader bit_reader;
  DECLARE_ALIGNED(16, struct macroblockd, xd);
  DECLARE_ALIGNED(16, int16_t, dqcoeff[MAX_MB_PLANE][64 * 64]);

  
  LFWorkerData lfdata;
} TileWorkerData;


typedef struct VP9LfSyncData {
#if CONFIG_MULTITHREAD
  pthread_mutex_t *mutex_;
  pthread_cond_t *cond_;
#endif
  
  int *cur_sb_col;
  
  
  int sync_range;
} VP9LfSync;


void vp9_loop_filter_alloc(struct VP9Common *cm, struct VP9LfSyncData *lf_sync,
                           int rows, int width);


void vp9_loop_filter_dealloc(struct VP9LfSyncData *lf_sync, int rows);


void vp9_loop_filter_frame_mt(struct VP9Decompressor *pbi,
                              struct VP9Common *cm,
                              struct macroblockd *xd,
                              int frame_filter_level,
                              int y_only, int partial_frame);

#endif  
