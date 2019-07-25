





















































#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pa_ringbuffer.h"
#include <string.h>
#include "pa_memorybarrier.h"





ring_buffer_size_t PaUtil_InitializeRingBuffer( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementSizeBytes, ring_buffer_size_t elementCount, void *dataPtr )
{
    if( ((elementCount-1) & elementCount) != 0) return -1; 
    rbuf->bufferSize = elementCount;
    rbuf->buffer = (char *)dataPtr;
    PaUtil_FlushRingBuffer( rbuf );
    rbuf->bigMask = (elementCount*2)-1;
    rbuf->smallMask = (elementCount)-1;
    rbuf->elementSizeBytes = elementSizeBytes;
    return 0;
}



ring_buffer_size_t PaUtil_GetRingBufferReadAvailable( PaUtilRingBuffer *rbuf )
{
    PaUtil_ReadMemoryBarrier();
    return ( (rbuf->writeIndex - rbuf->readIndex) & rbuf->bigMask );
}


ring_buffer_size_t PaUtil_GetRingBufferWriteAvailable( PaUtilRingBuffer *rbuf )
{
    
    return ( rbuf->bufferSize - PaUtil_GetRingBufferReadAvailable(rbuf));
}



void PaUtil_FlushRingBuffer( PaUtilRingBuffer *rbuf )
{
    rbuf->writeIndex = rbuf->readIndex = 0;
}







ring_buffer_size_t PaUtil_GetRingBufferWriteRegions( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementCount,
                                       void **dataPtr1, ring_buffer_size_t *sizePtr1,
                                       void **dataPtr2, ring_buffer_size_t *sizePtr2 )
{
    ring_buffer_size_t   index;
    ring_buffer_size_t   available = PaUtil_GetRingBufferWriteAvailable( rbuf );
    if( elementCount > available ) elementCount = available;
    
    index = rbuf->writeIndex & rbuf->smallMask;
    if( (index + elementCount) > rbuf->bufferSize )
    {
        
        ring_buffer_size_t   firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index*rbuf->elementSizeBytes];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = elementCount - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index*rbuf->elementSizeBytes];
        *sizePtr1 = elementCount;
        *dataPtr2 = NULL;
        *sizePtr2 = 0;
    }
    return elementCount;
}




ring_buffer_size_t PaUtil_AdvanceRingBufferWriteIndex( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementCount )
{
    
    PaUtil_WriteMemoryBarrier();
    return rbuf->writeIndex = (rbuf->writeIndex + elementCount) & rbuf->bigMask;
}







ring_buffer_size_t PaUtil_GetRingBufferReadRegions( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementCount,
                                void **dataPtr1, ring_buffer_size_t *sizePtr1,
                                void **dataPtr2, ring_buffer_size_t *sizePtr2 )
{
    ring_buffer_size_t   index;
    ring_buffer_size_t   available = PaUtil_GetRingBufferReadAvailable( rbuf );
    if( elementCount > available ) elementCount = available;
    
    index = rbuf->readIndex & rbuf->smallMask;
    if( (index + elementCount) > rbuf->bufferSize )
    {
        
        ring_buffer_size_t firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index*rbuf->elementSizeBytes];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = elementCount - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index*rbuf->elementSizeBytes];
        *sizePtr1 = elementCount;
        *dataPtr2 = NULL;
        *sizePtr2 = 0;
    }
    return elementCount;
}


ring_buffer_size_t PaUtil_AdvanceRingBufferReadIndex( PaUtilRingBuffer *rbuf, ring_buffer_size_t elementCount )
{
    
    PaUtil_WriteMemoryBarrier();
    return rbuf->readIndex = (rbuf->readIndex + elementCount) & rbuf->bigMask;
}



ring_buffer_size_t PaUtil_WriteRingBuffer( PaUtilRingBuffer *rbuf, const void *data, ring_buffer_size_t elementCount )
{
    ring_buffer_size_t size1, size2, numWritten;
    void *data1, *data2;
    numWritten = PaUtil_GetRingBufferWriteRegions( rbuf, elementCount, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {

        memcpy( data1, data, size1*rbuf->elementSizeBytes );
        data = ((char *)data) + size1*rbuf->elementSizeBytes;
        memcpy( data2, data, size2*rbuf->elementSizeBytes );
    }
    else
    {
        memcpy( data1, data, size1*rbuf->elementSizeBytes );
    }
    PaUtil_AdvanceRingBufferWriteIndex( rbuf, numWritten );
    return numWritten;
}



ring_buffer_size_t PaUtil_ReadRingBuffer( PaUtilRingBuffer *rbuf, void *data, ring_buffer_size_t elementCount )
{
    ring_buffer_size_t size1, size2, numRead;
    void *data1, *data2;
    numRead = PaUtil_GetRingBufferReadRegions( rbuf, elementCount, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {
        memcpy( data, data1, size1*rbuf->elementSizeBytes );
        data = ((char *)data) + size1*rbuf->elementSizeBytes;
        memcpy( data, data2, size2*rbuf->elementSizeBytes );
    }
    else
    {
        memcpy( data, data1, size1*rbuf->elementSizeBytes );
    }
    PaUtil_AdvanceRingBufferReadIndex( rbuf, numRead );
    return numRead;
}
