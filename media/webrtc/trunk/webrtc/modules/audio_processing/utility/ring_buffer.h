












#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_RING_BUFFER_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_UTILITY_RING_BUFFER_H_

#include <stddef.h> 

int WebRtc_CreateBuffer(void** handle,
                        size_t element_count,
                        size_t element_size);
int WebRtc_InitBuffer(void* handle);
int WebRtc_FreeBuffer(void* handle);








size_t WebRtc_ReadBuffer(void* handle,
                         void** data_ptr,
                         void* data,
                         size_t element_count);


size_t WebRtc_WriteBuffer(void* handle, const void* data, size_t element_count);






int WebRtc_MoveReadPtr(void* handle, int element_count);


size_t WebRtc_available_read(const void* handle);


size_t WebRtc_available_write(const void* handle);

#endif 
