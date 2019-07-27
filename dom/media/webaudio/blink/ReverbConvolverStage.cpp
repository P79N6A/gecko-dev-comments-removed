



























#include "ReverbConvolverStage.h"

#include "ReverbAccumulationBuffer.h"
#include "ReverbConvolver.h"
#include "ReverbInputBuffer.h"
#include "mozilla/PodOperations.h"

using namespace mozilla;

namespace WebCore {

ReverbConvolverStage::ReverbConvolverStage(const float* impulseResponse, size_t, size_t reverbTotalLatency, size_t stageOffset, size_t stageLength,
                                           size_t fftSize, size_t renderPhase, size_t renderSliceSize, ReverbAccumulationBuffer* accumulationBuffer, bool directMode)
    : m_accumulationBuffer(accumulationBuffer)
    , m_accumulationReadIndex(0)
    , m_inputReadIndex(0)
    , m_directMode(directMode)
{
    MOZ_ASSERT(impulseResponse);
    MOZ_ASSERT(accumulationBuffer);

    if (!m_directMode) {
        m_fftKernel = new FFTBlock(fftSize);
        m_fftKernel->PadAndMakeScaledDFT(impulseResponse + stageOffset, stageLength);
        m_fftConvolver = new FFTConvolver(fftSize);
    } else {
        m_directKernel.SetLength(fftSize / 2);
        PodCopy(m_directKernel.Elements(), impulseResponse + stageOffset, fftSize / 2);
        m_directConvolver = new DirectConvolver(renderSliceSize);
    }
    m_temporaryBuffer.SetLength(renderSliceSize);
    PodZero(m_temporaryBuffer.Elements(), m_temporaryBuffer.Length());

    
    size_t totalDelay = stageOffset + reverbTotalLatency;

    
    size_t halfSize = fftSize / 2;
    if (!m_directMode) {
        MOZ_ASSERT(totalDelay >= halfSize);
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
    m_preDelayBuffer.SetLength(delayBufferSize);
    PodZero(m_preDelayBuffer.Elements(), m_preDelayBuffer.Length());
}

size_t ReverbConvolverStage::sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t amount = aMallocSizeOf(this);

    if (m_fftKernel) {
        amount += m_fftKernel->SizeOfIncludingThis(aMallocSizeOf);
    }

    if (m_fftConvolver) {
        amount += m_fftConvolver->sizeOfIncludingThis(aMallocSizeOf);
    }

    amount += m_preDelayBuffer.SizeOfExcludingThis(aMallocSizeOf);
    amount += m_temporaryBuffer.SizeOfExcludingThis(aMallocSizeOf);
    amount += m_directKernel.SizeOfExcludingThis(aMallocSizeOf);

    if (m_directConvolver) {
        amount += m_directConvolver->sizeOfIncludingThis(aMallocSizeOf);
    }

    return amount;
}

void ReverbConvolverStage::processInBackground(ReverbConvolver* convolver, size_t framesToProcess)
{
    ReverbInputBuffer* inputBuffer = convolver->inputBuffer();
    float* source = inputBuffer->directReadFrom(&m_inputReadIndex, framesToProcess);
    process(source, framesToProcess);
}

void ReverbConvolverStage::process(const float* source, size_t framesToProcess)
{
    MOZ_ASSERT(source);
    if (!source)
        return;
    
    

    const float* preDelayedSource;
    float* preDelayedDestination;
    float* temporaryBuffer;
    bool isTemporaryBufferSafe = false;
    if (m_preDelayLength > 0) {
        
        bool isPreDelaySafe = m_preReadWriteIndex + framesToProcess <= m_preDelayBuffer.Length();
        MOZ_ASSERT(isPreDelaySafe);
        if (!isPreDelaySafe)
            return;

        isTemporaryBufferSafe = framesToProcess <= m_temporaryBuffer.Length();

        preDelayedDestination = m_preDelayBuffer.Elements() + m_preReadWriteIndex;
        preDelayedSource = preDelayedDestination;
        temporaryBuffer = m_temporaryBuffer.Elements();
    } else {
        
        preDelayedDestination = 0;
        preDelayedSource = source;
        temporaryBuffer = m_preDelayBuffer.Elements();
        
        isTemporaryBufferSafe = framesToProcess <= m_preDelayBuffer.Length();
    }
    
    MOZ_ASSERT(isTemporaryBufferSafe);
    if (!isTemporaryBufferSafe)
        return;

    if (m_framesProcessed < m_preDelayLength) {
        
        
        m_accumulationBuffer->updateReadIndex(&m_accumulationReadIndex, framesToProcess);
    } else {
        
        
        
        if (!m_directMode)
            m_fftConvolver->process(m_fftKernel, preDelayedSource, temporaryBuffer, framesToProcess);
        else
            m_directConvolver->process(&m_directKernel, preDelayedSource, temporaryBuffer, framesToProcess);

        
        m_accumulationBuffer->accumulate(temporaryBuffer, framesToProcess, &m_accumulationReadIndex, m_postDelayLength);
    }

    
    if (m_preDelayLength > 0) {
        memcpy(preDelayedDestination, source, sizeof(float) * framesToProcess);
        m_preReadWriteIndex += framesToProcess;

        MOZ_ASSERT(m_preReadWriteIndex <= m_preDelayLength);
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
    PodZero(m_preDelayBuffer.Elements(), m_preDelayBuffer.Length());
    m_accumulationReadIndex = 0;
    m_inputReadIndex = 0;
    m_framesProcessed = 0;
}

} 
