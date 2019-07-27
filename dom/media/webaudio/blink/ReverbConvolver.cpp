



























#include "ReverbConvolver.h"
#include "ReverbConvolverStage.h"

using namespace mozilla;

template<>
struct RunnableMethodTraits<WebCore::ReverbConvolver>
{
  static void RetainCallee(WebCore::ReverbConvolver* obj) {}
  static void ReleaseCallee(WebCore::ReverbConvolver* obj) {}
};

namespace WebCore {

const int InputBufferSize = 8 * 16384;








const size_t RealtimeFrameLimit = 8192  + 4096; 

const size_t MinFFTSize = 128;
const size_t MaxRealtimeFFTSize = 2048;

ReverbConvolver::ReverbConvolver(const float* impulseResponseData, size_t impulseResponseLength, size_t renderSliceSize, size_t maxFFTSize, size_t convolverRenderPhase, bool useBackgroundThreads)
    : m_impulseResponseLength(impulseResponseLength)
    , m_accumulationBuffer(impulseResponseLength + renderSliceSize)
    , m_inputBuffer(InputBufferSize)
    , m_minFFTSize(MinFFTSize) 
    , m_maxFFTSize(maxFFTSize) 
    , m_backgroundThread("ConvolverWorker")
    , m_backgroundThreadCondition(&m_backgroundThreadLock)
    , m_useBackgroundThreads(useBackgroundThreads)
    , m_wantsToExit(false)
    , m_moreInputBuffered(false)
{
    
    
    
    
    m_maxRealtimeFFTSize = MaxRealtimeFFTSize;

    
    
    bool hasRealtimeConstraint = useBackgroundThreads;

    const float* response = impulseResponseData;
    size_t totalResponseLength = impulseResponseLength;

    
    size_t reverbTotalLatency = 0;

    size_t stageOffset = 0;
    int i = 0;
    size_t fftSize = m_minFFTSize;
    while (stageOffset < totalResponseLength) {
        size_t stageSize = fftSize / 2;

        
        
        if (stageSize + stageOffset > totalResponseLength)
            stageSize = totalResponseLength - stageOffset;

        
        int renderPhase = convolverRenderPhase + i * renderSliceSize;

        bool useDirectConvolver = !stageOffset;

        nsAutoPtr<ReverbConvolverStage> stage(new ReverbConvolverStage(response, totalResponseLength, reverbTotalLatency, stageOffset, stageSize, fftSize, renderPhase, renderSliceSize, &m_accumulationBuffer, useDirectConvolver));

        bool isBackgroundStage = false;

        if (this->useBackgroundThreads() && stageOffset > RealtimeFrameLimit) {
            m_backgroundStages.AppendElement(stage.forget());
            isBackgroundStage = true;
        } else
            m_stages.AppendElement(stage.forget());

        stageOffset += stageSize;
        ++i;

        if (!useDirectConvolver) {
            
            fftSize *= 2;
        }

        if (hasRealtimeConstraint && !isBackgroundStage && fftSize > m_maxRealtimeFFTSize)
            fftSize = m_maxRealtimeFFTSize;
        if (fftSize > m_maxFFTSize)
            fftSize = m_maxFFTSize;
    }

    
    
    if (this->useBackgroundThreads() && m_backgroundStages.Length() > 0) {
        if (!m_backgroundThread.Start()) {
          NS_WARNING("Cannot start convolver thread.");
          return;
        }
        CancelableTask* task = NewRunnableMethod(this, &ReverbConvolver::backgroundThreadEntry);
        m_backgroundThread.message_loop()->PostTask(FROM_HERE, task);
    }
}

ReverbConvolver::~ReverbConvolver()
{
    
    if (useBackgroundThreads() && m_backgroundThread.IsRunning()) {
        m_wantsToExit = true;

        
        {
            AutoLock locker(m_backgroundThreadLock);
            m_moreInputBuffered = true;
            m_backgroundThreadCondition.Signal();
        }

        m_backgroundThread.Stop();
    }
}

size_t ReverbConvolver::sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t amount = aMallocSizeOf(this);
    amount += m_stages.SizeOfExcludingThis(aMallocSizeOf);
    for (size_t i = 0; i < m_stages.Length(); i++) {
        if (m_stages[i]) {
            amount += m_stages[i]->sizeOfIncludingThis(aMallocSizeOf);
        }
    }

    amount += m_backgroundStages.SizeOfExcludingThis(aMallocSizeOf);
    for (size_t i = 0; i < m_backgroundStages.Length(); i++) {
        if (m_backgroundStages[i]) {
            amount += m_backgroundStages[i]->sizeOfIncludingThis(aMallocSizeOf);
        }
    }

    
    
    amount += m_accumulationBuffer.sizeOfExcludingThis(aMallocSizeOf);
    amount += m_inputBuffer.sizeOfExcludingThis(aMallocSizeOf);

    
    
    
    
    return amount;
}

void ReverbConvolver::backgroundThreadEntry()
{
    while (!m_wantsToExit) {
        
        m_moreInputBuffered = false;
        {
            AutoLock locker(m_backgroundThreadLock);
            while (!m_moreInputBuffered && !m_wantsToExit)
                m_backgroundThreadCondition.Wait();
        }

        
        int writeIndex = m_inputBuffer.writeIndex();

        
        
        int readIndex;

        while ((readIndex = m_backgroundStages[0]->inputReadIndex()) != writeIndex) { 
            
            const int SliceSize = MinFFTSize / 2;

            
            for (size_t i = 0; i < m_backgroundStages.Length(); ++i)
                m_backgroundStages[i]->processInBackground(this, SliceSize);
        }
    }
}

void ReverbConvolver::process(const float* sourceChannelData, size_t sourceChannelLength,
                              float* destinationChannelData, size_t destinationChannelLength,
                              size_t framesToProcess)
{
    bool isSafe = sourceChannelData && destinationChannelData && sourceChannelLength >= framesToProcess && destinationChannelLength >= framesToProcess;
    MOZ_ASSERT(isSafe);
    if (!isSafe)
        return;

    const float* source = sourceChannelData;
    float* destination = destinationChannelData;
    bool isDataSafe = source && destination;
    MOZ_ASSERT(isDataSafe);
    if (!isDataSafe)
        return;

    
    m_inputBuffer.write(source, framesToProcess);

    
    for (size_t i = 0; i < m_stages.Length(); ++i)
        m_stages[i]->process(source, framesToProcess);

    
    m_accumulationBuffer.readAndClear(destination, framesToProcess);

    

    
    
    
    
    
    if (m_backgroundThreadLock.Try()) {
        m_moreInputBuffered = true;
        m_backgroundThreadCondition.Signal();
        m_backgroundThreadLock.Release();
    }
}

void ReverbConvolver::reset()
{
    for (size_t i = 0; i < m_stages.Length(); ++i)
        m_stages[i]->reset();

    for (size_t i = 0; i < m_backgroundStages.Length(); ++i)
        m_backgroundStages[i]->reset();

    m_accumulationBuffer.reset();
    m_inputBuffer.reset();
}

size_t ReverbConvolver::latencyFrames() const
{
    return 0;
}

} 
