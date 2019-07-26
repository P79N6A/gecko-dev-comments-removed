#ifndef WEBRTC_AUDIO_DEVICE_PA_RINGBUFFER_H
#define WEBRTC_AUDIO_DEVICE_PA_RINGBUFFER_H



































































#if defined(__APPLE__)
#include <sys/types.h>
typedef int32_t ring_buffer_size_t;
#elif defined( __GNUC__ )
typedef long ring_buffer_size_t;
#elif (_MSC_VER >= 1400)
typedef long ring_buffer_size_t;
#elif defined(_MSC_VER) || defined(__BORLANDC__)
typedef long ring_buffer_size_t;
#else
typedef long ring_buffer_size_t;
#endif



#ifdef __cplusplus
extern "C"
{
#endif

typedef struct PaUtilRingBuffer
{
    ring_buffer_size_t  bufferSize; 
    ring_buffer_size_t  writeIndex; 
    ring_buffer_size_t  readIndex;  
    ring_buffer_size_t  bigMask;    
    ring_buffer_size_t  smallMask;  
    ring_buffer_size_t  elementSizeBytes; 
    char  *buffer;    
}PaUtilRingBuffer;














ring_buffer_size_t PaUtil_InitializeRingBuffer( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementSizeBytes, ring_buffer_size_t elementCount, void *dataPtr );





void PaUtil_FlushRingBuffer( PaUtilRingBuffer *rbuf );







ring_buffer_size_t PaUtil_GetRingBufferWriteAvailable( PaUtilRingBuffer *rbuf );







ring_buffer_size_t PaUtil_GetRingBufferReadAvailable( PaUtilRingBuffer *rbuf );











ring_buffer_size_t PaUtil_WriteRingBuffer( PaUtilRingBuffer *rbuf, const void *data, ring_buffer_size_t elementCount );











ring_buffer_size_t PaUtil_ReadRingBuffer( PaUtilRingBuffer *rbuf, void *data, ring_buffer_size_t elementCount );





















ring_buffer_size_t PaUtil_GetRingBufferWriteRegions( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementCount,
                                       void **dataPtr1, ring_buffer_size_t *sizePtr1,
                                       void **dataPtr2, ring_buffer_size_t *sizePtr2 );









ring_buffer_size_t PaUtil_AdvanceRingBufferWriteIndex( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementCount );





















ring_buffer_size_t PaUtil_GetRingBufferReadRegions( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementCount,
                                      void **dataPtr1, ring_buffer_size_t *sizePtr1,
                                      void **dataPtr2, ring_buffer_size_t *sizePtr2 );









ring_buffer_size_t PaUtil_AdvanceRingBufferReadIndex( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementCount );

#ifdef __cplusplus
}
#endif 
#endif
