








#ifndef LOOKAHEAD_H
#define LOOKAHEAD_H
#include "vpx_scale/yv12config.h"
#include "vpx/vpx_integer.h"

struct lookahead_entry
{
    YV12_BUFFER_CONFIG  img;
    int64_t             ts_start;
    int64_t             ts_end;
    unsigned int        flags;
};


struct lookahead_ctx;








struct lookahead_ctx* vp8_lookahead_init(unsigned int width,
                                         unsigned int height,
                                         unsigned int depth
                                         );





void vp8_lookahead_destroy(struct lookahead_ctx *ctx);













int
vp8_lookahead_push(struct lookahead_ctx *ctx,
                   YV12_BUFFER_CONFIG   *src,
                   int64_t               ts_start,
                   int64_t               ts_end,
                   unsigned int          flags);













struct lookahead_entry*
vp8_lookahead_pop(struct lookahead_ctx *ctx,
                  int                   drain);










struct lookahead_entry*
vp8_lookahead_peek(struct lookahead_ctx *ctx,
                   int                   index);






unsigned int
vp8_lookahead_depth(struct lookahead_ctx *ctx);


#endif
