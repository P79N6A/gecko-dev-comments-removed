























#ifndef AVUTIL_BUFFER_H
#define AVUTIL_BUFFER_H

#include <stdint.h>












































typedef struct AVBuffer AVBuffer;







typedef struct AVBufferRef {
    AVBuffer *buffer;

    




    uint8_t *data;
    


    int      size;
} AVBufferRef;






AVBufferRef *av_buffer_alloc(int size);





AVBufferRef *av_buffer_allocz(int size);





#define AV_BUFFER_FLAG_READONLY (1 << 0)
















AVBufferRef *av_buffer_create(uint8_t *data, int size,
                              void (*free)(void *opaque, uint8_t *data),
                              void *opaque, int flags);






void av_buffer_default_free(void *opaque, uint8_t *data);







AVBufferRef *av_buffer_ref(AVBufferRef *buf);







void av_buffer_unref(AVBufferRef **buf);







int av_buffer_is_writable(const AVBufferRef *buf);










int av_buffer_make_writable(AVBufferRef **buf);
















int av_buffer_realloc(AVBufferRef **buf, int size);







































typedef struct AVBufferPool AVBufferPool;










AVBufferPool *av_buffer_pool_init(int size, AVBufferRef* (*alloc)(int size));










void av_buffer_pool_uninit(AVBufferPool **pool);







AVBufferRef *av_buffer_pool_get(AVBufferPool *pool);





#endif 
