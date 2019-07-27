









#ifndef VP9_DECODER_VP9_DTHREAD_H_
#define VP9_DECODER_VP9_DTHREAD_H_

#include "./vpx_config.h"
#include "vp9/common/vp9_thread.h"
#include "vpx/internal/vpx_codec_internal.h"

struct VP9Common;
struct VP9Decoder;



typedef struct FrameWorkerData {
  struct VP9Decoder *pbi;
  const uint8_t *data;
  const uint8_t *data_end;
  size_t data_size;
  void *user_priv;
  int result;
  int worker_id;
  int received_frame;

  
  
  uint8_t *scratch_buffer;
  size_t scratch_buffer_size;

#if CONFIG_MULTITHREAD
  pthread_mutex_t stats_mutex;
  pthread_cond_t stats_cond;
#endif

  int frame_context_ready;  
  int frame_decoded;        
} FrameWorkerData;

void vp9_frameworker_lock_stats(VP9Worker *const worker);
void vp9_frameworker_unlock_stats(VP9Worker *const worker);
void vp9_frameworker_signal_stats(VP9Worker *const worker);





void vp9_frameworker_wait(VP9Worker *const worker, RefCntBuffer *const ref_buf,
                          int row);



void vp9_frameworker_broadcast(RefCntBuffer *const buf, int row);


void vp9_frameworker_copy_context(VP9Worker *const dst_worker,
                                  VP9Worker *const src_worker);

#endif  
