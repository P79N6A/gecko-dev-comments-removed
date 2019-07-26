



















#ifndef AVUTIL_BPRINT_H
#define AVUTIL_BPRINT_H

#include "attributes.h"





#define FF_PAD_STRUCTURE(size, ...) \
    __VA_ARGS__ \
    char reserved_padding[size - sizeof(struct { __VA_ARGS__ })];









































typedef struct AVBPrint {
    FF_PAD_STRUCTURE(1024,
    char *str;         
    unsigned len;      
    unsigned size;     
    unsigned size_max; 
    char reserved_internal_buffer[1];
    )
} AVBPrint;





#define AV_BPRINT_SIZE_UNLIMITED  ((unsigned)-1)
#define AV_BPRINT_SIZE_AUTOMATIC  1
#define AV_BPRINT_SIZE_COUNT_ONLY 0














void av_bprint_init(AVBPrint *buf, unsigned size_init, unsigned size_max);










void av_bprint_init_for_buffer(AVBPrint *buf, char *buffer, unsigned size);




void av_bprintf(AVBPrint *buf, const char *fmt, ...) av_printf_format(2, 3);




void av_bprint_chars(AVBPrint *buf, char c, unsigned n);










void av_bprint_get_buffer(AVBPrint *buf, unsigned size,
                          unsigned char **mem, unsigned *actual_size);




void av_bprint_clear(AVBPrint *buf);







static inline int av_bprint_is_complete(AVBPrint *buf)
{
    return buf->len < buf->size;
}












int av_bprint_finalize(AVBPrint *buf, char **ret_str);

#endif 
