








#include <assert.h>
#include <stdlib.h>
#include "vpx_config.h"
#include "lookahead.h"
#include "vp8/common/extend.h"

#define MAX_LAG_BUFFERS (CONFIG_REALTIME_ONLY? 1 : 25)

struct lookahead_ctx
{
    unsigned int max_sz;         
    unsigned int sz;             
    unsigned int read_idx;       
    unsigned int write_idx;      
    struct lookahead_entry *buf; 
};



static struct lookahead_entry *
pop(struct lookahead_ctx *ctx,
    unsigned int         *idx)
{
    unsigned int            index = *idx;
    struct lookahead_entry *buf = ctx->buf + index;

    assert(index < ctx->max_sz);
    if(++index >= ctx->max_sz)
        index -= ctx->max_sz;
    *idx = index;
    return buf;
}


void
vp8_lookahead_destroy(struct lookahead_ctx *ctx)
{
    if(ctx)
    {
        if(ctx->buf)
        {
            int i;

            for(i = 0; i < ctx->max_sz; i++)
                vp8_yv12_de_alloc_frame_buffer(&ctx->buf[i].img);
            free(ctx->buf);
        }
        free(ctx);
    }
}


struct lookahead_ctx*
vp8_lookahead_init(unsigned int width,
                   unsigned int height,
                   unsigned int depth)
{
    struct lookahead_ctx *ctx = NULL;
    int i;

    
    if(depth < 1)
        depth = 1;
    else if(depth > MAX_LAG_BUFFERS)
        depth = MAX_LAG_BUFFERS;

    
    width = (width + 15) & ~15;
    height = (height + 15) & ~15;

    
    ctx = calloc(1, sizeof(*ctx));
    if(ctx)
    {
        ctx->max_sz = depth;
        ctx->buf = calloc(depth, sizeof(*ctx->buf));
        if(!ctx->buf)
            goto bail;
        for(i=0; i<depth; i++)
            if (vp8_yv12_alloc_frame_buffer(&ctx->buf[i].img,
                                            width, height, VP8BORDERINPIXELS))
                goto bail;
    }
    return ctx;
bail:
    vp8_lookahead_destroy(ctx);
    return NULL;
}


int
vp8_lookahead_push(struct lookahead_ctx *ctx,
                   YV12_BUFFER_CONFIG   *src,
                   int64_t               ts_start,
                   int64_t               ts_end,
                   unsigned int          flags)
{
    struct lookahead_entry* buf;

    if(ctx->sz + 1 > ctx->max_sz)
        return 1;
    ctx->sz++;
    buf = pop(ctx, &ctx->write_idx);
    vp8_copy_and_extend_frame(src, &buf->img);
    buf->ts_start = ts_start;
    buf->ts_end = ts_end;
    buf->flags = flags;
    return 0;
}


struct lookahead_entry*
vp8_lookahead_pop(struct lookahead_ctx *ctx,
                  int                   drain)
{
    struct lookahead_entry* buf = NULL;

    if(ctx->sz && (drain || ctx->sz == ctx->max_sz))
    {
        buf = pop(ctx, &ctx->read_idx);
        ctx->sz--;
    }
    return buf;
}


struct lookahead_entry*
vp8_lookahead_peek(struct lookahead_ctx *ctx,
                   int                   index)
{
    struct lookahead_entry* buf = NULL;

    assert(index < ctx->max_sz);
    if(index < ctx->sz)
    {
        index += ctx->read_idx;
        if(index >= ctx->max_sz)
            index -= ctx->max_sz;
        buf = ctx->buf + index;
    }
    return buf;
}


unsigned int
vp8_lookahead_depth(struct lookahead_ctx *ctx)
{
    return ctx->sz;
}
