












































#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#include "FIFOSampleBuffer.h"

using namespace soundtouch;


FIFOSampleBuffer::FIFOSampleBuffer(int numChannels)
{
    assert(numChannels > 0);
    sizeInBytes = 0; 
    buffer = NULL;
    bufferUnaligned = NULL;
    samplesInBuffer = 0;
    bufferPos = 0;
    channels = (uint)numChannels;
    ensureCapacity(32);     
}



FIFOSampleBuffer::~FIFOSampleBuffer()
{
    delete[] bufferUnaligned;
    bufferUnaligned = NULL;
    buffer = NULL;
}



void FIFOSampleBuffer::setChannels(int numChannels)
{
    uint usedBytes;

    assert(numChannels > 0);
    usedBytes = channels * samplesInBuffer;
    channels = (uint)numChannels;
    samplesInBuffer = usedBytes / channels;
}





void FIFOSampleBuffer::rewind()
{
    if (buffer && bufferPos) 
    {
        memmove(buffer, ptrBegin(), sizeof(SAMPLETYPE) * channels * samplesInBuffer);
        bufferPos = 0;
    }
}




void FIFOSampleBuffer::putSamples(const SAMPLETYPE *samples, uint nSamples)
{
    memcpy(ptrEnd(nSamples), samples, sizeof(SAMPLETYPE) * nSamples * channels);
    samplesInBuffer += nSamples;
}








void FIFOSampleBuffer::putSamples(uint nSamples)
{
    uint req;

    req = samplesInBuffer + nSamples;
    ensureCapacity(req);
    samplesInBuffer += nSamples;
}














SAMPLETYPE *FIFOSampleBuffer::ptrEnd(uint slackCapacity) 
{
    ensureCapacity(samplesInBuffer + slackCapacity);
    return buffer + samplesInBuffer * channels;
}









SAMPLETYPE *FIFOSampleBuffer::ptrBegin()
{
    assert(buffer);
    return buffer + bufferPos * channels;
}






void FIFOSampleBuffer::ensureCapacity(uint capacityRequirement)
{
    SAMPLETYPE *tempUnaligned, *temp;

    if (capacityRequirement > getCapacity()) 
    {
        
        sizeInBytes = (capacityRequirement * channels * sizeof(SAMPLETYPE) + 4095) & (uint)-4096;
        assert(sizeInBytes % 2 == 0);
        tempUnaligned = new SAMPLETYPE[sizeInBytes / sizeof(SAMPLETYPE) + 16 / sizeof(SAMPLETYPE)];
        if (tempUnaligned == NULL)
        {
            ST_THROW_RT_ERROR("Couldn't allocate memory!\n");
        }
        
        temp = (SAMPLETYPE *)(((ulong)tempUnaligned + 15) & (ulong)-16);
        if (samplesInBuffer)
        {
            memcpy(temp, ptrBegin(), samplesInBuffer * channels * sizeof(SAMPLETYPE));
        }
        delete[] bufferUnaligned;
        buffer = temp;
        bufferUnaligned = tempUnaligned;
        bufferPos = 0;
    } 
    else 
    {
        
        rewind();
    }
}



uint FIFOSampleBuffer::getCapacity() const
{
    return sizeInBytes / (channels * sizeof(SAMPLETYPE));
}



uint FIFOSampleBuffer::numSamples() const
{
    return samplesInBuffer;
}







uint FIFOSampleBuffer::receiveSamples(SAMPLETYPE *output, uint maxSamples)
{
    uint num;

    num = (maxSamples > samplesInBuffer) ? samplesInBuffer : maxSamples;

    memcpy(output, ptrBegin(), channels * sizeof(SAMPLETYPE) * num);
    return receiveSamples(num);
}





uint FIFOSampleBuffer::receiveSamples(uint maxSamples)
{
    if (maxSamples >= samplesInBuffer)
    {
        uint temp;

        temp = samplesInBuffer;
        samplesInBuffer = 0;
        return temp;
    }

    samplesInBuffer -= maxSamples;
    bufferPos += maxSamples;

    return maxSamples;
}



int FIFOSampleBuffer::isEmpty() const
{
    return (samplesInBuffer == 0) ? 1 : 0;
}



void FIFOSampleBuffer::clear()
{
    samplesInBuffer = 0;
    bufferPos = 0;
}




uint FIFOSampleBuffer::adjustAmountOfSamples(uint numSamples)
{
    if (numSamples < samplesInBuffer)
    {
        samplesInBuffer = numSamples;
    }
    return samplesInBuffer;
}

