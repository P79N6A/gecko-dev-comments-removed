



























#include "FFTConvolver.h"
#include "mozilla/PodOperations.h"

using namespace mozilla;

namespace WebCore {

FFTConvolver::FFTConvolver(size_t fftSize)
    : m_frame(fftSize)
    , m_readWriteIndex(0)
{
  m_inputBuffer.SetLength(fftSize);
  PodZero(m_inputBuffer.Elements(), fftSize);
  m_outputBuffer.SetLength(fftSize);
  PodZero(m_outputBuffer.Elements(), fftSize);
  m_lastOverlapBuffer.SetLength(fftSize / 2);
  PodZero(m_lastOverlapBuffer.Elements(), fftSize / 2);
}

size_t FFTConvolver::sizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t amount = 0;
    amount += m_frame.SizeOfExcludingThis(aMallocSizeOf);
    amount += m_inputBuffer.SizeOfExcludingThis(aMallocSizeOf);
    amount += m_outputBuffer.SizeOfExcludingThis(aMallocSizeOf);
    amount += m_lastOverlapBuffer.SizeOfExcludingThis(aMallocSizeOf);
    return amount;
}

size_t FFTConvolver::sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + sizeOfExcludingThis(aMallocSizeOf);
}

void FFTConvolver::process(FFTBlock* fftKernel, const float* sourceP, float* destP, size_t framesToProcess)
{
    size_t halfSize = fftSize() / 2;

    
    
    bool isGood = !(halfSize % framesToProcess && framesToProcess % halfSize);
    MOZ_ASSERT(isGood);
    if (!isGood)
        return;

    size_t numberOfDivisions = halfSize <= framesToProcess ? (framesToProcess / halfSize) : 1;
    size_t divisionSize = numberOfDivisions == 1 ? framesToProcess : halfSize;

    for (size_t i = 0; i < numberOfDivisions; ++i, sourceP += divisionSize, destP += divisionSize) {
        
        float* inputP = m_inputBuffer.Elements();

        
        bool isCopyGood1 = sourceP && inputP && m_readWriteIndex + divisionSize <= m_inputBuffer.Length();
        MOZ_ASSERT(isCopyGood1);
        if (!isCopyGood1)
            return;

        memcpy(inputP + m_readWriteIndex, sourceP, sizeof(float) * divisionSize);

        
        float* outputP = m_outputBuffer.Elements();

        
        bool isCopyGood2 = destP && outputP && m_readWriteIndex + divisionSize <= m_outputBuffer.Length();
        MOZ_ASSERT(isCopyGood2);
        if (!isCopyGood2)
            return;

        memcpy(destP, outputP + m_readWriteIndex, sizeof(float) * divisionSize);
        m_readWriteIndex += divisionSize;

        
        if (m_readWriteIndex == halfSize) {
            
            m_frame.PerformFFT(m_inputBuffer.Elements());
            m_frame.Multiply(*fftKernel);
            m_frame.GetInverseWithoutScaling(m_outputBuffer.Elements());

            
            AudioBufferAddWithScale(m_lastOverlapBuffer.Elements(), 1.0f,
                                    m_outputBuffer.Elements(), halfSize);

            
            bool isCopyGood3 = m_outputBuffer.Length() == 2 * halfSize && m_lastOverlapBuffer.Length() == halfSize;
            MOZ_ASSERT(isCopyGood3);
            if (!isCopyGood3)
                return;

            memcpy(m_lastOverlapBuffer.Elements(), m_outputBuffer.Elements() + halfSize, sizeof(float) * halfSize);

            
            m_readWriteIndex = 0;
        }
    }
}

void FFTConvolver::reset()
{
    PodZero(m_lastOverlapBuffer.Elements(), m_lastOverlapBuffer.Length());
    m_readWriteIndex = 0;
}

} 
