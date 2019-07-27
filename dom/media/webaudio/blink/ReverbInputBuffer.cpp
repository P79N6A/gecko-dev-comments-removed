



























#include "ReverbInputBuffer.h"
#include "mozilla/PodOperations.h"

using namespace mozilla;

namespace WebCore {

ReverbInputBuffer::ReverbInputBuffer(size_t length)
    : m_writeIndex(0)
{
  m_buffer.SetLength(length);
  PodZero(m_buffer.Elements(), length);
}

void ReverbInputBuffer::write(const float* sourceP, size_t numberOfFrames)
{
    size_t bufferLength = m_buffer.Length();
    bool isCopySafe = m_writeIndex + numberOfFrames <= bufferLength;
    MOZ_ASSERT(isCopySafe);
    if (!isCopySafe)
        return;

    memcpy(m_buffer.Elements() + m_writeIndex, sourceP, sizeof(float) * numberOfFrames);

    m_writeIndex += numberOfFrames;
    MOZ_ASSERT(m_writeIndex <= bufferLength);

    if (m_writeIndex >= bufferLength)
        m_writeIndex = 0;
}

float* ReverbInputBuffer::directReadFrom(int* readIndex, size_t numberOfFrames)
{
    size_t bufferLength = m_buffer.Length();
    bool isPointerGood = readIndex && *readIndex >= 0 && *readIndex + numberOfFrames <= bufferLength;
    MOZ_ASSERT(isPointerGood);
    if (!isPointerGood) {
        
        if (readIndex)
            *readIndex = 0;
        return m_buffer.Elements();
    }

    float* sourceP = m_buffer.Elements();
    float* p = sourceP + *readIndex;

    
    *readIndex = (*readIndex + numberOfFrames) % bufferLength;

    return p;
}

void ReverbInputBuffer::reset()
{
    PodZero(m_buffer.Elements(), m_buffer.Length());
    m_writeIndex = 0;
}

} 
