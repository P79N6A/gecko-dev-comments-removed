








#include <assert.h>
#include <stdlib.h>

#include "./vpx_config.h"

#include "vp9/common/vp9_common.h"

#include "vp9/encoder/vp9_extend.h"
#include "vp9/encoder/vp9_lookahead.h"
#include "vp9/encoder/vp9_onyx_int.h"

struct lookahead_ctx {
  unsigned int max_sz;         
  unsigned int sz;             
  unsigned int read_idx;       
  unsigned int write_idx;      
  struct lookahead_entry *buf; 
};



static struct lookahead_entry * pop(struct lookahead_ctx *ctx,
                                    unsigned int *idx) {
  unsigned int index = *idx;
  struct lookahead_entry *buf = ctx->buf + index;

  assert(index < ctx->max_sz);
  if (++index >= ctx->max_sz)
    index -= ctx->max_sz;
  *idx = index;
  return buf;
}


void vp9_lookahead_destroy(struct lookahead_ctx *ctx) {
  if (ctx) {
    if (ctx->buf) {
      unsigned int i;

      for (i = 0; i < ctx->max_sz; i++)
        vp9_free_frame_buffer(&ctx->buf[i].img);
      free(ctx->buf);
    }
    free(ctx);
  }
}


struct lookahead_ctx * vp9_lookahead_init(unsigned int width,
                                          unsigned int height,
                                          unsigned int subsampling_x,
                                          unsigned int subsampling_y,
                                          unsigned int depth) {
  struct lookahead_ctx *ctx = NULL;

  
  depth = clamp(depth, 1, MAX_LAG_BUFFERS);

  
  ctx = calloc(1, sizeof(*ctx));
  if (ctx) {
    unsigned int i;
    ctx->max_sz = depth;
    ctx->buf = calloc(depth, sizeof(*ctx->buf));
    if (!ctx->buf)
      goto bail;
    for (i = 0; i < depth; i++)
      if (vp9_alloc_frame_buffer(&ctx->buf[i].img,
                                 width, height, subsampling_x, subsampling_y,
                                 VP9_ENC_BORDER_IN_PIXELS))
        goto bail;
  }
  return ctx;
 bail:
  vp9_lookahead_destroy(ctx);
  return NULL;
}

#define USE_PARTIAL_COPY 0

int vp9_lookahead_push(struct lookahead_ctx *ctx, YV12_BUFFER_CONFIG   *src,
                       int64_t ts_start, int64_t ts_end, unsigned int flags,
                       unsigned char *active_map) {
  struct lookahead_entry *buf;
#if USE_PARTIAL_COPY
  int row, col, active_end;
  int mb_rows = (src->y_height + 15) >> 4;
  int mb_cols = (src->y_width + 15) >> 4;
#endif

  if (ctx->sz + 1 > ctx->max_sz)
    return 1;
  ctx->sz++;
  buf = pop(ctx, &ctx->write_idx);

#if USE_PARTIAL_COPY
  
  

  
  
  
  
  if (ctx->max_sz == 1 && active_map && !flags) {
    for (row = 0; row < mb_rows; ++row) {
      col = 0;

      while (1) {
        
        for (; col < mb_cols; ++col) {
          if (active_map[col])
            break;
        }

        
        if (col == mb_cols)
          break;

        
        active_end = col;

        for (; active_end < mb_cols; ++active_end) {
          if (!active_map[active_end])
            break;
        }

        
        vp9_copy_and_extend_frame_with_rect(src, &buf->img,
                                            row << 4,
                                            col << 4, 16,
                                            (active_end - col) << 4);

        
        col = active_end;
      }

      active_map += mb_cols;
    }
  } else {
    vp9_copy_and_extend_frame(src, &buf->img);
  }
#else
  
  vp9_copy_and_extend_frame(src, &buf->img);
#endif

  buf->ts_start = ts_start;
  buf->ts_end = ts_end;
  buf->flags = flags;
  return 0;
}


struct lookahead_entry * vp9_lookahead_pop(struct lookahead_ctx *ctx,
                                           int drain) {
  struct lookahead_entry *buf = NULL;

  if (ctx->sz && (drain || ctx->sz == ctx->max_sz)) {
    buf = pop(ctx, &ctx->read_idx);
    ctx->sz--;
  }
  return buf;
}


struct lookahead_entry * vp9_lookahead_peek(struct lookahead_ctx *ctx,
                                            int index) {
  struct lookahead_entry *buf = NULL;

  if (index < (int)ctx->sz) {
    index += ctx->read_idx;
    if (index >= (int)ctx->max_sz)
      index -= ctx->max_sz;
    buf = ctx->buf + index;
  }
  return buf;
}

unsigned int vp9_lookahead_depth(struct lookahead_ctx *ctx) {
  return ctx->sz;
}
