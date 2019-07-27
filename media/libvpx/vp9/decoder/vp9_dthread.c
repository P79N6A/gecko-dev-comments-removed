









#include "./vpx_config.h"

#include "vpx_mem/vpx_mem.h"

#include "vp9/common/vp9_reconinter.h"

#include "vp9/decoder/vp9_dthread.h"
#include "vp9/decoder/vp9_decoder.h"

#if CONFIG_MULTITHREAD
static INLINE void mutex_lock(pthread_mutex_t *const mutex) {
  const int kMaxTryLocks = 4000;
  int locked = 0;
  int i;

  for (i = 0; i < kMaxTryLocks; ++i) {
    if (!pthread_mutex_trylock(mutex)) {
      locked = 1;
      break;
    }
  }

  if (!locked)
    pthread_mutex_lock(mutex);
}
#endif  

static INLINE void sync_read(VP9LfSync *const lf_sync, int r, int c) {
#if CONFIG_MULTITHREAD
  const int nsync = lf_sync->sync_range;

  if (r && !(c & (nsync - 1))) {
    pthread_mutex_t *const mutex = &lf_sync->mutex_[r - 1];
    mutex_lock(mutex);

    while (c > lf_sync->cur_sb_col[r - 1] - nsync) {
      pthread_cond_wait(&lf_sync->cond_[r - 1], mutex);
    }
    pthread_mutex_unlock(mutex);
  }
#else
  (void)lf_sync;
  (void)r;
  (void)c;
#endif  
}

static INLINE void sync_write(VP9LfSync *const lf_sync, int r, int c,
                              const int sb_cols) {
#if CONFIG_MULTITHREAD
  const int nsync = lf_sync->sync_range;
  int cur;
  
  int sig = 1;

  if (c < sb_cols - 1) {
    cur = c;
    if (c % nsync)
      sig = 0;
  } else {
    cur = sb_cols + nsync;
  }

  if (sig) {
    mutex_lock(&lf_sync->mutex_[r]);

    lf_sync->cur_sb_col[r] = cur;

    pthread_cond_signal(&lf_sync->cond_[r]);
    pthread_mutex_unlock(&lf_sync->mutex_[r]);
  }
#else
  (void)lf_sync;
  (void)r;
  (void)c;
  (void)sb_cols;
#endif  
}


static void loop_filter_rows_mt(const YV12_BUFFER_CONFIG *const frame_buffer,
                                VP9_COMMON *const cm,
                                struct macroblockd_plane planes[MAX_MB_PLANE],
                                int start, int stop, int y_only,
                                VP9LfSync *const lf_sync, int num_lf_workers) {
  const int num_planes = y_only ? 1 : MAX_MB_PLANE;
  int r, c;  
  const int sb_cols = mi_cols_aligned_to_sb(cm->mi_cols) >> MI_BLOCK_SIZE_LOG2;

  for (r = start; r < stop; r += num_lf_workers) {
    const int mi_row = r << MI_BLOCK_SIZE_LOG2;
    MODE_INFO **const mi = cm->mi_grid_visible + mi_row * cm->mi_stride;

    for (c = 0; c < sb_cols; ++c) {
      const int mi_col = c << MI_BLOCK_SIZE_LOG2;
      LOOP_FILTER_MASK lfm;
      int plane;

      sync_read(lf_sync, r, c);

      vp9_setup_dst_planes(planes, frame_buffer, mi_row, mi_col);
      vp9_setup_mask(cm, mi_row, mi_col, mi + mi_col, cm->mi_stride, &lfm);

      for (plane = 0; plane < num_planes; ++plane) {
        vp9_filter_block_plane(cm, &planes[plane], mi_row, &lfm);
      }

      sync_write(lf_sync, r, c, sb_cols);
    }
  }
}


static int loop_filter_row_worker(void *arg1, void *arg2) {
  TileWorkerData *const tile_data = (TileWorkerData*)arg1;
  LFWorkerData *const lf_data = &tile_data->lfdata;
  (void) arg2;
  loop_filter_rows_mt(lf_data->frame_buffer, lf_data->cm, lf_data->planes,
                      lf_data->start, lf_data->stop, lf_data->y_only,
                      lf_data->lf_sync, lf_data->num_lf_workers);
  return 1;
}



void vp9_loop_filter_frame_mt(YV12_BUFFER_CONFIG *frame,
                              VP9Decoder *pbi, VP9_COMMON *cm,
                              int frame_filter_level,
                              int y_only) {
  VP9LfSync *const lf_sync = &pbi->lf_row_sync;
  const VP9WorkerInterface *const winterface = vp9_get_worker_interface();
  
  const int sb_rows = mi_cols_aligned_to_sb(cm->mi_rows) >> MI_BLOCK_SIZE_LOG2;
  const int tile_cols = 1 << cm->log2_tile_cols;
  const int num_workers = MIN(pbi->max_threads & ~1, tile_cols);
  int i;

  
  
  if (!lf_sync->sync_range || cm->last_height != cm->height) {
    vp9_loop_filter_dealloc(lf_sync);
    vp9_loop_filter_alloc(cm, lf_sync, sb_rows, cm->width);
  }

  if (!frame_filter_level) return;

  vp9_loop_filter_frame_init(cm, frame_filter_level);

  
  vpx_memset(lf_sync->cur_sb_col, -1, sizeof(*lf_sync->cur_sb_col) * sb_rows);

  
  
  
  
  
  
  
  
  
  
  for (i = 0; i < num_workers; ++i) {
    VP9Worker *const worker = &pbi->tile_workers[i];
    TileWorkerData *const tile_data = (TileWorkerData*)worker->data1;
    LFWorkerData *const lf_data = &tile_data->lfdata;

    worker->hook = (VP9WorkerHook)loop_filter_row_worker;

    
    lf_data->frame_buffer = frame;
    lf_data->cm = cm;
    vp9_copy(lf_data->planes, pbi->mb.plane);
    lf_data->start = i;
    lf_data->stop = sb_rows;
    lf_data->y_only = y_only;   

    lf_data->lf_sync = lf_sync;
    lf_data->num_lf_workers = num_workers;

    
    if (i == num_workers - 1) {
      winterface->execute(worker);
    } else {
      winterface->launch(worker);
    }
  }

  
  for (i = 0; i < num_workers; ++i) {
    winterface->sync(&pbi->tile_workers[i]);
  }
}


static int get_sync_range(int width) {
  
  
  if (width < 640)
    return 1;
  else if (width <= 1280)
    return 2;
  else if (width <= 4096)
    return 4;
  else
    return 8;
}


void vp9_loop_filter_alloc(VP9_COMMON *cm, VP9LfSync *lf_sync, int rows,
                           int width) {
  lf_sync->rows = rows;
#if CONFIG_MULTITHREAD
  {
    int i;

    CHECK_MEM_ERROR(cm, lf_sync->mutex_,
                    vpx_malloc(sizeof(*lf_sync->mutex_) * rows));
    for (i = 0; i < rows; ++i) {
      pthread_mutex_init(&lf_sync->mutex_[i], NULL);
    }

    CHECK_MEM_ERROR(cm, lf_sync->cond_,
                    vpx_malloc(sizeof(*lf_sync->cond_) * rows));
    for (i = 0; i < rows; ++i) {
      pthread_cond_init(&lf_sync->cond_[i], NULL);
    }
  }
#endif  

  CHECK_MEM_ERROR(cm, lf_sync->cur_sb_col,
                  vpx_malloc(sizeof(*lf_sync->cur_sb_col) * rows));

  
  lf_sync->sync_range = get_sync_range(width);
}


void vp9_loop_filter_dealloc(VP9LfSync *lf_sync) {
  if (lf_sync != NULL) {
#if CONFIG_MULTITHREAD
    int i;

    if (lf_sync->mutex_ != NULL) {
      for (i = 0; i < lf_sync->rows; ++i) {
        pthread_mutex_destroy(&lf_sync->mutex_[i]);
      }
      vpx_free(lf_sync->mutex_);
    }
    if (lf_sync->cond_ != NULL) {
      for (i = 0; i < lf_sync->rows; ++i) {
        pthread_cond_destroy(&lf_sync->cond_[i]);
      }
      vpx_free(lf_sync->cond_);
    }
#endif  
    vpx_free(lf_sync->cur_sb_col);
    
    
    vp9_zero(*lf_sync);
  }
}
