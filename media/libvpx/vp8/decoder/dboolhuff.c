










#include "dboolhuff.h"
#include "vpx_ports/mem.h"
#include "vpx_mem/vpx_mem.h"

int vp8dx_start_decode(BOOL_DECODER *br,
                       const unsigned char *source,
                       unsigned int source_sz)
{
    br->user_buffer_end = source+source_sz;
    br->user_buffer     = source;
    br->value    = 0;
    br->count    = -8;
    br->range    = 255;

    if (source_sz && !source)
        return 1;

    
    vp8dx_bool_decoder_fill(br);

    return 0;
}


void vp8dx_bool_decoder_fill(BOOL_DECODER *br)
{
    const unsigned char *bufptr;
    const unsigned char *bufend;
    VP8_BD_VALUE         value;
    int                  count;
    bufend = br->user_buffer_end;
    bufptr = br->user_buffer;
    value = br->value;
    count = br->count;

    VP8DX_BOOL_DECODER_FILL(count, value, bufptr, bufend);

    br->user_buffer = bufptr;
    br->value = value;
    br->count = count;
}
