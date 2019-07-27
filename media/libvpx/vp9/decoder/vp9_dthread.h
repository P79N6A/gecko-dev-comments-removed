









#ifndef VP9_DECODER_VP9_DTHREAD_H_
#define VP9_DECODER_VP9_DTHREAD_H_

#include "./vpx_config.h"
#include "vp9/common/vp9_thread.h"
#include "vp9/decoder/vp9_reader.h"

struct VP9Common;
struct VP9Decoder;

typedef struct TileWorkerData {
  struct VP9Common *cm;
  vp9_reader bit_reader;
  DECLARE_ALIGNED(16, struct macroblockd, xd);

  
  LFWorkerData lfdata;
} TileWorkerData;


typedef struct VP9LfSyncData {
#if CONFIG_MULTITHREAD
  pthread_mutex_t *mutex_;
  pthread_cond_t *cond_;
#endif
  
  int *cur_sb_col;
  
  
  int sync_range;
  int rows;
} VP9LfSync;


void vp9_loop_filter_alloc(VP9LfSync *lf_sync, VP9_COMMON *cm, int rows,
                           int width);


void vp9_loop_filter_dealloc(VP9LfSync *lf_sync);


void vp9_loop_filter_frame_mt(YV12_BUFFER_CONFIG *frame,
                              struct VP9Decoder *pbi,
                              struct VP9Common *cm,
                              int frame_filter_level,
                              int y_only);

#endif  
