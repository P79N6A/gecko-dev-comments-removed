









#include "./vpx_config.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/common/vp9_reconinter.h"
#include "vp9/decoder/vp9_dthread.h"
#include "vp9/decoder/vp9_decoder.h"




void vp9_frameworker_lock_stats(VP9Worker *const worker) {
#if CONFIG_MULTITHREAD
  FrameWorkerData *const worker_data = worker->data1;
  pthread_mutex_lock(&worker_data->stats_mutex);
#else
  (void)worker;
#endif
}

void vp9_frameworker_unlock_stats(VP9Worker *const worker) {
#if CONFIG_MULTITHREAD
  FrameWorkerData *const worker_data = worker->data1;
  pthread_mutex_unlock(&worker_data->stats_mutex);
#else
  (void)worker;
#endif
}

void vp9_frameworker_signal_stats(VP9Worker *const worker) {
#if CONFIG_MULTITHREAD
  FrameWorkerData *const worker_data = worker->data1;


#if defined(_WIN32) && !HAVE_PTHREAD_H
  pthread_cond_signal(&worker_data->stats_cond);
#else
  pthread_cond_broadcast(&worker_data->stats_cond);
#endif

#else
  (void)worker;
#endif
}


#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define BUILDING_WITH_TSAN
#endif
#endif


void vp9_frameworker_wait(VP9Worker *const worker, RefCntBuffer *const ref_buf,
                          int row) {
#if CONFIG_MULTITHREAD
  if (!ref_buf)
    return;

#ifndef BUILDING_WITH_TSAN
  
  
  if (ref_buf->row >= row && ref_buf->buf.corrupted != 1) return;
#endif

  {
    
    
    VP9Worker *const ref_worker = ref_buf->frame_worker_owner;
    FrameWorkerData *const ref_worker_data =
        (FrameWorkerData *)ref_worker->data1;
    const VP9Decoder *const pbi = ref_worker_data->pbi;

#ifdef DEBUG_THREAD
    {
      FrameWorkerData *const worker_data = (FrameWorkerData *)worker->data1;
      printf("%d %p worker is waiting for %d %p worker (%d)  ref %d \r\n",
             worker_data->worker_id, worker, ref_worker_data->worker_id,
             ref_buf->frame_worker_owner, row, ref_buf->row);
    }
#endif

    vp9_frameworker_lock_stats(ref_worker);
    while (ref_buf->row < row && pbi->cur_buf == ref_buf &&
           ref_buf->buf.corrupted != 1) {
      pthread_cond_wait(&ref_worker_data->stats_cond,
                        &ref_worker_data->stats_mutex);
    }

    if (ref_buf->buf.corrupted == 1) {
      FrameWorkerData *const worker_data = (FrameWorkerData *)worker->data1;
      vp9_frameworker_unlock_stats(ref_worker);
      vpx_internal_error(&worker_data->pbi->common.error,
                         VPX_CODEC_CORRUPT_FRAME,
                         "Worker %p failed to decode frame", worker);
    }
    vp9_frameworker_unlock_stats(ref_worker);
  }
#else
  (void)worker;
  (void)ref_buf;
  (void)row;
  (void)ref_buf;
#endif  
}

void vp9_frameworker_broadcast(RefCntBuffer *const buf, int row) {
#if CONFIG_MULTITHREAD
  VP9Worker *worker = buf->frame_worker_owner;

#ifdef DEBUG_THREAD
  {
    FrameWorkerData *const worker_data = (FrameWorkerData *)worker->data1;
    printf("%d %p worker decode to (%d) \r\n", worker_data->worker_id,
           buf->frame_worker_owner, row);
  }
#endif

  vp9_frameworker_lock_stats(worker);
  buf->row = row;
  vp9_frameworker_signal_stats(worker);
  vp9_frameworker_unlock_stats(worker);
#else
  (void)buf;
  (void)row;
#endif  
}

void vp9_frameworker_copy_context(VP9Worker *const dst_worker,
                                  VP9Worker *const src_worker) {
#if CONFIG_MULTITHREAD
  FrameWorkerData *const src_worker_data = (FrameWorkerData *)src_worker->data1;
  FrameWorkerData *const dst_worker_data = (FrameWorkerData *)dst_worker->data1;
  VP9_COMMON *const src_cm = &src_worker_data->pbi->common;
  VP9_COMMON *const dst_cm = &dst_worker_data->pbi->common;
  int i;

  
  vp9_frameworker_lock_stats(src_worker);
  while (!src_worker_data->frame_context_ready) {
    pthread_cond_wait(&src_worker_data->stats_cond,
        &src_worker_data->stats_mutex);
  }

  dst_cm->last_frame_seg_map = src_cm->seg.enabled ?
      src_cm->current_frame_seg_map : src_cm->last_frame_seg_map;
  dst_worker_data->pbi->need_resync = src_worker_data->pbi->need_resync;
  vp9_frameworker_unlock_stats(src_worker);

  dst_cm->bit_depth = src_cm->bit_depth;
#if CONFIG_VP9_HIGHBITDEPTH
  dst_cm->use_highbitdepth = src_cm->use_highbitdepth;
#endif
  dst_cm->prev_frame = src_cm->show_existing_frame ?
                       src_cm->prev_frame : src_cm->cur_frame;
  dst_cm->last_width = !src_cm->show_existing_frame ?
                       src_cm->width : src_cm->last_width;
  dst_cm->last_height = !src_cm->show_existing_frame ?
                        src_cm->height : src_cm->last_height;
  dst_cm->subsampling_x = src_cm->subsampling_x;
  dst_cm->subsampling_y = src_cm->subsampling_y;
  dst_cm->frame_type = src_cm->frame_type;
  dst_cm->last_show_frame = !src_cm->show_existing_frame ?
                            src_cm->show_frame : src_cm->last_show_frame;
  for (i = 0; i < REF_FRAMES; ++i)
    dst_cm->ref_frame_map[i] = src_cm->next_ref_frame_map[i];

  memcpy(dst_cm->lf_info.lfthr, src_cm->lf_info.lfthr,
         (MAX_LOOP_FILTER + 1) * sizeof(loop_filter_thresh));
  dst_cm->lf.last_sharpness_level = src_cm->lf.sharpness_level;
  dst_cm->lf.filter_level = src_cm->lf.filter_level;
  memcpy(dst_cm->lf.ref_deltas, src_cm->lf.ref_deltas, MAX_REF_LF_DELTAS);
  memcpy(dst_cm->lf.mode_deltas, src_cm->lf.mode_deltas, MAX_MODE_LF_DELTAS);
  dst_cm->seg = src_cm->seg;
  memcpy(dst_cm->frame_contexts, src_cm->frame_contexts,
         FRAME_CONTEXTS * sizeof(dst_cm->frame_contexts[0]));
#else
  (void) dst_worker;
  (void) src_worker;
#endif  
}
