












#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_RING_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_RING_BUFFER_H_

#include <stddef.h>  

typedef struct RingBuffer RingBuffer;


RingBuffer* WebRtc_CreateBuffer(size_t element_count, size_t element_size);
int WebRtc_InitBuffer(RingBuffer* handle);
void WebRtc_FreeBuffer(void* handle);











size_t WebRtc_ReadBuffer(RingBuffer* handle,
                         void** data_ptr,
                         void* data,
                         size_t element_count);


size_t WebRtc_WriteBuffer(RingBuffer* handle, const void* data,
                          size_t element_count);






int WebRtc_MoveReadPtr(RingBuffer* handle, int element_count);


size_t WebRtc_available_read(const RingBuffer* handle);


size_t WebRtc_available_write(const RingBuffer* handle);

#endif  
