



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/ReverbConvolverStage.h"

#include "core/platform/audio/ReverbAccumulationBuffer.h"
#include "core/platform/audio/ReverbConvolver.h"
#include "core/platform/audio/ReverbInputBuffer.h"
#include "core/platform/audio/VectorMath.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace VectorMath;

ReverbConvolverStage::ReverbConvolverStage(const float* impulseResponse, size_t, size_t reverbTotalLatency, size_t stageOffset, size_t stageLength,
                                           size_t fftSize, size_t renderPhase, size_t renderSliceSize, ReverbAccumulationBuffer* accumulationBuffer, bool directMode)
    : m_accumulationBuffer(accumulationBuffer)
    , m_accumulationReadIndex(0)
    , m_inputReadIndex(0)
    , m_directMode(directMode)
{
    ASSERT(impulseResponse);
    ASSERT(accumulationBuffer);

    if (!m_directMode) {
        m_fftKernel = adoptPtr(new FFTFrame(fftSize));
        m_fftKernel->doPaddedFFT(impulseResponse + stageOffset, stageLength);
        m_fftConvolver = adoptPtr(new FFTConvolver(fftSize));
    } else {
        m_directKernel = adoptPtr(new AudioFloatArray(fftSize / 2));
        m_directKernel->copyToRange(impulseResponse + stageOffset, 0, fftSize / 2);
        m_directConvolver = adoptPtr(new DirectConvolver(renderSliceSize));
    }
    m_temporaryBuffer.allocate(renderSliceSize);

    
    size_t totalDelay = stageOffset + reverbTotalLatency;

    
    size_t halfSize = fftSize / 2;
    if (!m_directMode) {
        ASSERT(totalDelay >= halfSize);
        if (totalDelay >= halfSize)
            totalDelay -= halfSize;
    }

    
    
    int maxPreDelayLength = std::min(halfSize, totalDelay);
    m_preDelayLength = totalDelay > 0 ? renderPhase % maxPreDelayLength : 0;
    if (m_preDelayLength > totalDelay)
        m_preDelayLength = 0;

    m_postDelayLength = totalDelay - m_preDelayLength;
    m_preReadWriteIndex = 0;
    m_framesProcessed = 0; 

    size_t delayBufferSize = m_preDelayLength < fftSize ? fftSize : m_preDelayLength;
    delayBufferSize = delayBufferSize < renderSliceSize ? renderSliceSize : delayBufferSize;
    m_preDelayBuffer.allocate(delayBufferSize);
}

void ReverbConvolverStage::processInBackground(ReverbConvolver* convolver, size_t framesToProcess)
{
    ReverbInputBuffer* inputBuffer = convolver->inputBuffer();
    float* source = inputBuffer->directReadFrom(&m_inputReadIndex, framesToProcess);
    process(source, framesToProcess);
}

void ReverbConvolverStage::process(const float* source, size_t framesToProcess)
{
    ASSERT(source);
    if (!source)
        return;
    
    

    const float* preDelayedSource;
    float* preDelayedDestination;
    float* temporaryBuffer;
    bool isTemporaryBufferSafe = false;
    if (m_preDelayLength > 0) {
        
        bool isPreDelaySafe = m_preReadWriteIndex + framesToProcess <= m_preDelayBuffer.size();
        ASSERT(isPreDelaySafe);
        if (!isPreDelaySafe)
            return;

        isTemporaryBufferSafe = framesToProcess <= m_temporaryBuffer.size();

        preDelayedDestination = m_preDelayBuffer.data() + m_preReadWriteIndex;
        preDelayedSource = preDelayedDestination;
        temporaryBuffer = m_temporaryBuffer.data();        
    } else {
        
        preDelayedDestination = 0;
        preDelayedSource = source;
        temporaryBuffer = m_preDelayBuffer.data();
        
        isTemporaryBufferSafe = framesToProcess <= m_preDelayBuffer.size();
    }
    
    ASSERT(isTemporaryBufferSafe);
    if (!isTemporaryBufferSafe)
        return;

    if (m_framesProcessed < m_preDelayLength) {
        
        
        m_accumulationBuffer->updateReadIndex(&m_accumulationReadIndex, framesToProcess);
    } else {
        
        
        
        if (!m_directMode)
            m_fftConvolver->process(m_fftKernel.get(), preDelayedSource, temporaryBuffer, framesToProcess);
        else
            m_directConvolver->process(m_directKernel.get(), preDelayedSource, temporaryBuffer, framesToProcess);

        
        m_accumulationBuffer->accumulate(temporaryBuffer, framesToProcess, &m_accumulationReadIndex, m_postDelayLength);
    }

    
    if (m_preDelayLength > 0) {
        memcpy(preDelayedDestination, source, sizeof(float) * framesToProcess);
        m_preReadWriteIndex += framesToProcess;

        ASSERT(m_preReadWriteIndex <= m_preDelayLength);
        if (m_preReadWriteIndex >= m_preDelayLength)
            m_preReadWriteIndex = 0;
    }

    m_framesProcessed += framesToProcess;
}

void ReverbConvolverStage::reset()
{
    if (!m_directMode)
        m_fftConvolver->reset();
    else
        m_directConvolver->reset();
    m_preDelayBuffer.zero();
    m_accumulationReadIndex = 0;
    m_inputReadIndex = 0;
    m_framesProcessed = 0;
}

} 

#endif 
