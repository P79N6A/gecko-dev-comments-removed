



























#include "ReverbAccumulationBuffer.h"
#include "AudioNodeEngine.h"
#include "mozilla/PodOperations.h"
#include <algorithm>

using namespace mozilla;

namespace WebCore {

ReverbAccumulationBuffer::ReverbAccumulationBuffer(size_t length)
    : m_readIndex(0)
    , m_readTimeFrame(0)
{
  m_buffer.SetLength(length);
  PodZero(m_buffer.Elements(), length);
}

void ReverbAccumulationBuffer::readAndClear(float* destination, size_t numberOfFrames)
{
    size_t bufferLength = m_buffer.Length();
    bool isCopySafe = m_readIndex <= bufferLength && numberOfFrames <= bufferLength;

    MOZ_ASSERT(isCopySafe);
    if (!isCopySafe)
        return;

    size_t framesAvailable = bufferLength - m_readIndex;
    size_t numberOfFrames1 = std::min(numberOfFrames, framesAvailable);
    size_t numberOfFrames2 = numberOfFrames - numberOfFrames1;

    float* source = m_buffer.Elements();
    memcpy(destination, source + m_readIndex, sizeof(float) * numberOfFrames1);
    memset(source + m_readIndex, 0, sizeof(float) * numberOfFrames1);

    
    if (numberOfFrames2 > 0) {
        memcpy(destination + numberOfFrames1, source, sizeof(float) * numberOfFrames2);
        memset(source, 0, sizeof(float) * numberOfFrames2);
    }

    m_readIndex = (m_readIndex + numberOfFrames) % bufferLength;
    m_readTimeFrame += numberOfFrames;
}

void ReverbAccumulationBuffer::updateReadIndex(int* readIndex, size_t numberOfFrames) const
{
    
    *readIndex = (*readIndex + numberOfFrames) % m_buffer.Length();
}

int ReverbAccumulationBuffer::accumulate(float* source, size_t numberOfFrames, int* readIndex, size_t delayFrames)
{
    size_t bufferLength = m_buffer.Length();

    size_t writeIndex = (*readIndex + delayFrames) % bufferLength;

    
    *readIndex = (*readIndex + numberOfFrames) % bufferLength;

    size_t framesAvailable = bufferLength - writeIndex;
    size_t numberOfFrames1 = std::min(numberOfFrames, framesAvailable);
    size_t numberOfFrames2 = numberOfFrames - numberOfFrames1;

    float* destination = m_buffer.Elements();

    bool isSafe = writeIndex <= bufferLength && numberOfFrames1 + writeIndex <= bufferLength && numberOfFrames2 <= bufferLength;
    MOZ_ASSERT(isSafe);
    if (!isSafe)
        return 0;

    AudioBufferAddWithScale(source, 1.0f, destination + writeIndex, numberOfFrames1);

    
    if (numberOfFrames2 > 0) {
        AudioBufferAddWithScale(source + numberOfFrames1, 1.0f, destination, numberOfFrames2);
    }

    return writeIndex;
}

void ReverbAccumulationBuffer::reset()
{
    PodZero(m_buffer.Elements(), m_buffer.Length());
    m_readIndex = 0;
    m_readTimeFrame = 0;
}

} 
