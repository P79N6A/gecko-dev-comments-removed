



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/FFTConvolver.h"

#include "core/platform/audio/VectorMath.h"

namespace WebCore {

using namespace VectorMath;
    
FFTConvolver::FFTConvolver(size_t fftSize)
    : m_frame(fftSize)
    , m_readWriteIndex(0)
    , m_inputBuffer(fftSize) 
    , m_outputBuffer(fftSize)
    , m_lastOverlapBuffer(fftSize / 2)
{
}

void FFTConvolver::process(FFTFrame* fftKernel, const float* sourceP, float* destP, size_t framesToProcess)
{
    size_t halfSize = fftSize() / 2;

    
    
    bool isGood = !(halfSize % framesToProcess && framesToProcess % halfSize);
    ASSERT(isGood);
    if (!isGood)
        return;

    size_t numberOfDivisions = halfSize <= framesToProcess ? (framesToProcess / halfSize) : 1;
    size_t divisionSize = numberOfDivisions == 1 ? framesToProcess : halfSize;

    for (size_t i = 0; i < numberOfDivisions; ++i, sourceP += divisionSize, destP += divisionSize) {
        
        float* inputP = m_inputBuffer.data();

        
        bool isCopyGood1 = sourceP && inputP && m_readWriteIndex + divisionSize <= m_inputBuffer.size();
        ASSERT(isCopyGood1);
        if (!isCopyGood1)
            return;

        memcpy(inputP + m_readWriteIndex, sourceP, sizeof(float) * divisionSize);

        
        float* outputP = m_outputBuffer.data();

        
        bool isCopyGood2 = destP && outputP && m_readWriteIndex + divisionSize <= m_outputBuffer.size();
        ASSERT(isCopyGood2);
        if (!isCopyGood2)
            return;

        memcpy(destP, outputP + m_readWriteIndex, sizeof(float) * divisionSize);
        m_readWriteIndex += divisionSize;

        
        if (m_readWriteIndex == halfSize) {
            
            m_frame.doFFT(m_inputBuffer.data());
            m_frame.multiply(*fftKernel);
            m_frame.doInverseFFT(m_outputBuffer.data());

            
            vadd(m_outputBuffer.data(), 1, m_lastOverlapBuffer.data(), 1, m_outputBuffer.data(), 1, halfSize);

            
            bool isCopyGood3 = m_outputBuffer.size() == 2 * halfSize && m_lastOverlapBuffer.size() == halfSize;
            ASSERT(isCopyGood3);
            if (!isCopyGood3)
                return;

            memcpy(m_lastOverlapBuffer.data(), m_outputBuffer.data() + halfSize, sizeof(float) * halfSize);

            
            m_readWriteIndex = 0;
        }
    }
}

void FFTConvolver::reset()
{
    m_lastOverlapBuffer.zero();
    m_readWriteIndex = 0;
}

} 

#endif 
