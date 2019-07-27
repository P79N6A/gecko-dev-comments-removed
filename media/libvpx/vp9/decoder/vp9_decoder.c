









#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "./vp9_rtcd.h"
#include "./vpx_dsp_rtcd.h"
#include "./vpx_scale_rtcd.h"

#include "vpx_mem/vpx_mem.h"
#include "vpx_ports/vpx_once.h"
#include "vpx_ports/vpx_timer.h"
#include "vpx_scale/vpx_scale.h"

#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_onyxc_int.h"
#if CONFIG_VP9_POSTPROC
#include "vp9/common/vp9_postproc.h"
#endif
#include "vp9/common/vp9_quant_common.h"
#include "vp9/common/vp9_reconintra.h"
#include "vp9/common/vp9_systemdependent.h"
#include "vp9/common/vp9_thread.h"

#include "vp9/decoder/vp9_decodeframe.h"
#include "vp9/decoder/vp9_decoder.h"
#include "vp9/decoder/vp9_detokenize.h"

static void initialize_dec(void) {
  static volatile int init_done = 0;

  if (!init_done) {
    vp9_rtcd();
    vpx_dsp_rtcd();
    vpx_scale_rtcd();
    vp9_init_intra_predictors();
    init_done = 1;
  }
}

static void vp9_dec_setup_mi(VP9_COMMON *cm) {
  cm->mi = cm->mip + cm->mi_stride + 1;
  cm->mi_grid_visible = cm->mi_grid_base + cm->mi_stride + 1;
  memset(cm->mi_grid_base, 0,
         cm->mi_stride * (cm->mi_rows + 1) * sizeof(*cm->mi_grid_base));
}

static int vp9_dec_alloc_mi(VP9_COMMON *cm, int mi_size) {
  cm->mip = vpx_calloc(mi_size, sizeof(*cm->mip));
  if (!cm->mip)
    return 1;
  cm->mi_alloc_size = mi_size;
  cm->mi_grid_base = (MODE_INFO **)vpx_calloc(mi_size, sizeof(MODE_INFO*));
  if (!cm->mi_grid_base)
    return 1;
  return 0;
}

static void vp9_dec_free_mi(VP9_COMMON *cm) {
  vpx_free(cm->mip);
  cm->mip = NULL;
  vpx_free(cm->mi_grid_base);
  cm->mi_grid_base = NULL;
}

VP9Decoder *vp9_decoder_create(BufferPool *const pool) {
  VP9Decoder *volatile const pbi = vpx_memalign(32, sizeof(*pbi));
  VP9_COMMON *volatile const cm = pbi ? &pbi->common : NULL;

  if (!cm)
    return NULL;

  vp9_zero(*pbi);

  if (setjmp(cm->error.jmp)) {
    cm->error.setjmp = 0;
    vp9_decoder_remove(pbi);
    return NULL;
  }

  cm->error.setjmp = 1;

  CHECK_MEM_ERROR(cm, cm->fc,
                  (FRAME_CONTEXT *)vpx_calloc(1, sizeof(*cm->fc)));
  CHECK_MEM_ERROR(cm, cm->frame_contexts,
                  (FRAME_CONTEXT *)vpx_calloc(FRAME_CONTEXTS,
                  sizeof(*cm->frame_contexts)));

  pbi->need_resync = 1;
  once(initialize_dec);

  
  memset(&cm->ref_frame_map, -1, sizeof(cm->ref_frame_map));
  memset(&cm->next_ref_frame_map, -1, sizeof(cm->next_ref_frame_map));

  cm->current_video_frame = 0;
  pbi->ready_for_new_data = 1;
  pbi->common.buffer_pool = pool;

  cm->bit_depth = VPX_BITS_8;
  cm->dequant_bit_depth = VPX_BITS_8;

  cm->alloc_mi = vp9_dec_alloc_mi;
  cm->free_mi = vp9_dec_free_mi;
  cm->setup_mi = vp9_dec_setup_mi;

  vp9_loop_filter_init(cm);

  cm->error.setjmp = 0;

  vp9_get_worker_interface()->init(&pbi->lf_worker);

  return pbi;
}

void vp9_decoder_remove(VP9Decoder *pbi) {
  int i;

  vp9_get_worker_interface()->end(&pbi->lf_worker);
  vpx_free(pbi->lf_worker.data1);
  vpx_free(pbi->tile_data);
  for (i = 0; i < pbi->num_tile_workers; ++i) {
    VP9Worker *const worker = &pbi->tile_workers[i];
    vp9_get_worker_interface()->end(worker);
  }
  vpx_free(pbi->tile_worker_data);
  vpx_free(pbi->tile_worker_info);
  vpx_free(pbi->tile_workers);

  if (pbi->num_tile_workers > 0) {
    vp9_loop_filter_dealloc(&pbi->lf_row_sync);
  }

  vpx_free(pbi);
}

static int equal_dimensions(const YV12_BUFFER_CONFIG *a,
                            const YV12_BUFFER_CONFIG *b) {
    return a->y_height == b->y_height && a->y_width == b->y_width &&
           a->uv_height == b->uv_height && a->uv_width == b->uv_width;
}

vpx_codec_err_t vp9_copy_reference_dec(VP9Decoder *pbi,
                                       VP9_REFFRAME ref_frame_flag,
                                       YV12_BUFFER_CONFIG *sd) {
  VP9_COMMON *cm = &pbi->common;

  




  if (ref_frame_flag == VP9_LAST_FLAG) {
    const YV12_BUFFER_CONFIG *const cfg = get_ref_frame(cm, 0);
    if (cfg == NULL) {
      vpx_internal_error(&cm->error, VPX_CODEC_ERROR,
                         "No 'last' reference frame");
      return VPX_CODEC_ERROR;
    }
    if (!equal_dimensions(cfg, sd))
      vpx_internal_error(&cm->error, VPX_CODEC_ERROR,
                         "Incorrect buffer dimensions");
    else
      vp8_yv12_copy_frame(cfg, sd);
  } else {
    vpx_internal_error(&cm->error, VPX_CODEC_ERROR,
                       "Invalid reference frame");
  }

  return cm->error.error_code;
}


vpx_codec_err_t vp9_set_reference_dec(VP9_COMMON *cm,
                                      VP9_REFFRAME ref_frame_flag,
                                      YV12_BUFFER_CONFIG *sd) {
  RefBuffer *ref_buf = NULL;
  RefCntBuffer *const frame_bufs = cm->buffer_pool->frame_bufs;

  
  
  
  
  if (ref_frame_flag == VP9_LAST_FLAG) {
    ref_buf = &cm->frame_refs[0];
  } else if (ref_frame_flag == VP9_GOLD_FLAG) {
    ref_buf = &cm->frame_refs[1];
  } else if (ref_frame_flag == VP9_ALT_FLAG) {
    ref_buf = &cm->frame_refs[2];
  } else {
    vpx_internal_error(&cm->error, VPX_CODEC_ERROR,
                       "Invalid reference frame");
    return cm->error.error_code;
  }

  if (!equal_dimensions(ref_buf->buf, sd)) {
    vpx_internal_error(&cm->error, VPX_CODEC_ERROR,
                       "Incorrect buffer dimensions");
  } else {
    int *ref_fb_ptr = &ref_buf->idx;

    
    const int free_fb = get_free_fb(cm);
    if (cm->new_fb_idx == INVALID_IDX)
      return VPX_CODEC_MEM_ERROR;

    
    
    --frame_bufs[free_fb].ref_count;

    
    ref_cnt_fb(frame_bufs, ref_fb_ptr, free_fb);
    ref_buf->buf = &frame_bufs[*ref_fb_ptr].buf;
    vp8_yv12_copy_frame(sd, ref_buf->buf);
  }

  return cm->error.error_code;
}


static void swap_frame_buffers(VP9Decoder *pbi) {
  int ref_index = 0, mask;
  VP9_COMMON *const cm = &pbi->common;
  BufferPool *const pool = cm->buffer_pool;
  RefCntBuffer *const frame_bufs = cm->buffer_pool->frame_bufs;

  lock_buffer_pool(pool);
  for (mask = pbi->refresh_frame_flags; mask; mask >>= 1) {
    const int old_idx = cm->ref_frame_map[ref_index];
    
    decrease_ref_count(old_idx, frame_bufs, pool);

    
    if ((mask & 1) && old_idx >= 0) {
      decrease_ref_count(old_idx, frame_bufs, pool);
    }
    cm->ref_frame_map[ref_index] = cm->next_ref_frame_map[ref_index];
    ++ref_index;
  }

  
  for (; ref_index < REF_FRAMES && !cm->show_existing_frame; ++ref_index) {
    const int old_idx = cm->ref_frame_map[ref_index];
    decrease_ref_count(old_idx, frame_bufs, pool);
    cm->ref_frame_map[ref_index] = cm->next_ref_frame_map[ref_index];
  }
  unlock_buffer_pool(pool);
  pbi->hold_ref_buf = 0;
  cm->frame_to_show = get_frame_new_buffer(cm);

  if (!pbi->frame_parallel_decode || !cm->show_frame) {
    lock_buffer_pool(pool);
    --frame_bufs[cm->new_fb_idx].ref_count;
    unlock_buffer_pool(pool);
  }

  
  for (ref_index = 0; ref_index < 3; ref_index++)
    cm->frame_refs[ref_index].idx = -1;
}

int vp9_receive_compressed_data(VP9Decoder *pbi,
                                size_t size, const uint8_t **psource) {
  VP9_COMMON *volatile const cm = &pbi->common;
  BufferPool *volatile const pool = cm->buffer_pool;
  RefCntBuffer *volatile const frame_bufs = cm->buffer_pool->frame_bufs;
  const uint8_t *source = *psource;
  int retcode = 0;
  cm->error.error_code = VPX_CODEC_OK;

  if (size == 0) {
    
    
    
    
    
    
    
    
    if (cm->frame_refs[0].idx > 0) {
      assert(cm->frame_refs[0].buf != NULL);
      cm->frame_refs[0].buf->corrupted = 1;
    }
  }

  pbi->ready_for_new_data = 0;

  
  
  if (!pbi->frame_parallel_decode && cm->new_fb_idx >= 0
      && frame_bufs[cm->new_fb_idx].ref_count == 0)
    pool->release_fb_cb(pool->cb_priv,
                        &frame_bufs[cm->new_fb_idx].raw_frame_buffer);
  
  cm->new_fb_idx = get_free_fb(cm);
  if (cm->new_fb_idx == INVALID_IDX)
    return VPX_CODEC_MEM_ERROR;

  
  cm->cur_frame = &pool->frame_bufs[cm->new_fb_idx];

  pbi->hold_ref_buf = 0;
  if (pbi->frame_parallel_decode) {
    VP9Worker *const worker = pbi->frame_worker_owner;
    vp9_frameworker_lock_stats(worker);
    frame_bufs[cm->new_fb_idx].frame_worker_owner = worker;
    
    pbi->cur_buf = &frame_bufs[cm->new_fb_idx];
    pbi->cur_buf->row = -1;
    pbi->cur_buf->col = -1;
    vp9_frameworker_unlock_stats(worker);
  } else {
    pbi->cur_buf = &frame_bufs[cm->new_fb_idx];
  }


  if (setjmp(cm->error.jmp)) {
    const VP9WorkerInterface *const winterface = vp9_get_worker_interface();
    int i;

    cm->error.setjmp = 0;
    pbi->ready_for_new_data = 1;

    
    
    winterface->sync(&pbi->lf_worker);
    for (i = 0; i < pbi->num_tile_workers; ++i) {
      winterface->sync(&pbi->tile_workers[i]);
    }

    lock_buffer_pool(pool);
    
    if (pbi->hold_ref_buf == 1) {
      int ref_index = 0, mask;
      for (mask = pbi->refresh_frame_flags; mask; mask >>= 1) {
        const int old_idx = cm->ref_frame_map[ref_index];
        
        decrease_ref_count(old_idx, frame_bufs, pool);

        
        if ((mask & 1) && old_idx >= 0) {
          decrease_ref_count(old_idx, frame_bufs, pool);
        }
        ++ref_index;
      }

      
      for (; ref_index < REF_FRAMES && !cm->show_existing_frame; ++ref_index) {
        const int old_idx = cm->ref_frame_map[ref_index];
        decrease_ref_count(old_idx, frame_bufs, pool);
      }
      pbi->hold_ref_buf = 0;
    }
    
    decrease_ref_count(cm->new_fb_idx, frame_bufs, pool);
    unlock_buffer_pool(pool);

    vp9_clear_system_state();
    return -1;
  }

  cm->error.setjmp = 1;
  vp9_decode_frame(pbi, source, source + size, psource);

  swap_frame_buffers(pbi);

  vp9_clear_system_state();

  if (!cm->show_existing_frame) {
    cm->last_show_frame = cm->show_frame;
    cm->prev_frame = cm->cur_frame;
    if (cm->seg.enabled && !pbi->frame_parallel_decode)
      vp9_swap_current_and_last_seg_map(cm);
  }

  
  if (pbi->frame_parallel_decode) {
    
    
    VP9Worker *const worker = pbi->frame_worker_owner;
    FrameWorkerData *const frame_worker_data = worker->data1;
    vp9_frameworker_lock_stats(worker);

    if (cm->show_frame) {
      cm->current_video_frame++;
    }
    frame_worker_data->frame_decoded = 1;
    frame_worker_data->frame_context_ready = 1;
    vp9_frameworker_signal_stats(worker);
    vp9_frameworker_unlock_stats(worker);
  } else {
    cm->last_width = cm->width;
    cm->last_height = cm->height;
    if (cm->show_frame) {
      cm->current_video_frame++;
    }
  }

  cm->error.setjmp = 0;
  return retcode;
}

int vp9_get_raw_frame(VP9Decoder *pbi, YV12_BUFFER_CONFIG *sd,
                      vp9_ppflags_t *flags) {
  VP9_COMMON *const cm = &pbi->common;
  int ret = -1;
#if !CONFIG_VP9_POSTPROC
  (void)*flags;
#endif

  if (pbi->ready_for_new_data == 1)
    return ret;

  pbi->ready_for_new_data = 1;

  
  if (!cm->show_frame)
    return ret;

  pbi->ready_for_new_data = 1;

#if CONFIG_VP9_POSTPROC
  if (!cm->show_existing_frame) {
    ret = vp9_post_proc_frame(cm, sd, flags);
  } else {
    *sd = *cm->frame_to_show;
    ret = 0;
  }
#else
  *sd = *cm->frame_to_show;
  ret = 0;
#endif 
  vp9_clear_system_state();
  return ret;
}

vpx_codec_err_t vp9_parse_superframe_index(const uint8_t *data,
                                           size_t data_sz,
                                           uint32_t sizes[8], int *count,
                                           vpx_decrypt_cb decrypt_cb,
                                           void *decrypt_state) {
  
  
  
  
  

  uint8_t marker;

  assert(data_sz);
  marker = read_marker(decrypt_cb, decrypt_state, data + data_sz - 1);
  *count = 0;

  if ((marker & 0xe0) == 0xc0) {
    const uint32_t frames = (marker & 0x7) + 1;
    const uint32_t mag = ((marker >> 3) & 0x3) + 1;
    const size_t index_sz = 2 + mag * frames;

    
    
    if (data_sz < index_sz)
      return VPX_CODEC_CORRUPT_FRAME;

    {
      const uint8_t marker2 = read_marker(decrypt_cb, decrypt_state,
                                          data + data_sz - index_sz);

      
      
      
      if (marker != marker2)
        return VPX_CODEC_CORRUPT_FRAME;
    }

    {
      
      uint32_t i, j;
      const uint8_t *x = &data[data_sz - index_sz + 1];

      
      uint8_t clear_buffer[32];
      assert(sizeof(clear_buffer) >= frames * mag);
      if (decrypt_cb) {
        decrypt_cb(decrypt_state, x, clear_buffer, frames * mag);
        x = clear_buffer;
      }

      for (i = 0; i < frames; ++i) {
        uint32_t this_sz = 0;

        for (j = 0; j < mag; ++j)
          this_sz |= (*x++) << (j * 8);
        sizes[i] = this_sz;
      }
      *count = frames;
    }
  }
  return VPX_CODEC_OK;
}
