



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/ReverbConvolver.h"

#include "core/platform/audio/AudioBus.h"
#include "core/platform/audio/VectorMath.h"

namespace WebCore {

using namespace VectorMath;

const int InputBufferSize = 8 * 16384;








const size_t RealtimeFrameLimit = 8192  + 4096; 

const size_t MinFFTSize = 128;
const size_t MaxRealtimeFFTSize = 2048;

static void backgroundThreadEntry(void* threadData)
{
    ReverbConvolver* reverbConvolver = static_cast<ReverbConvolver*>(threadData);
    reverbConvolver->backgroundThreadEntry();
}

ReverbConvolver::ReverbConvolver(AudioChannel* impulseResponse, size_t renderSliceSize, size_t maxFFTSize, size_t convolverRenderPhase, bool useBackgroundThreads)
    : m_impulseResponseLength(impulseResponse->length())
    , m_accumulationBuffer(impulseResponse->length() + renderSliceSize)
    , m_inputBuffer(InputBufferSize)
    , m_minFFTSize(MinFFTSize) 
    , m_maxFFTSize(maxFFTSize) 
    , m_useBackgroundThreads(useBackgroundThreads)
    , m_backgroundThread(0)
    , m_wantsToExit(false)
    , m_moreInputBuffered(false)
{
    
    
    
    
    m_maxRealtimeFFTSize = MaxRealtimeFFTSize;

    
    
    bool hasRealtimeConstraint = useBackgroundThreads;

    const float* response = impulseResponse->data();
    size_t totalResponseLength = impulseResponse->length();

    
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

        OwnPtr<ReverbConvolverStage> stage = adoptPtr(new ReverbConvolverStage(response, totalResponseLength, reverbTotalLatency, stageOffset, stageSize, fftSize, renderPhase, renderSliceSize, &m_accumulationBuffer, useDirectConvolver));

        bool isBackgroundStage = false;

        if (this->useBackgroundThreads() && stageOffset > RealtimeFrameLimit) {
            m_backgroundStages.append(stage.release());
            isBackgroundStage = true;
        } else
            m_stages.append(stage.release());

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

    
    
    if (this->useBackgroundThreads() && m_backgroundStages.size() > 0)
        m_backgroundThread = createThread(WebCore::backgroundThreadEntry, this, "convolution background thread");
}

ReverbConvolver::~ReverbConvolver()
{
    
    if (useBackgroundThreads() && m_backgroundThread) {
        m_wantsToExit = true;

        
        {
            MutexLocker locker(m_backgroundThreadLock);
            m_moreInputBuffered = true;
            m_backgroundThreadCondition.signal();
        }

        waitForThreadCompletion(m_backgroundThread);
    }
}

void ReverbConvolver::backgroundThreadEntry()
{
    while (!m_wantsToExit) {
        
        m_moreInputBuffered = false;        
        {
            MutexLocker locker(m_backgroundThreadLock);
            while (!m_moreInputBuffered && !m_wantsToExit)
                m_backgroundThreadCondition.wait(m_backgroundThreadLock);
        }

        
        int writeIndex = m_inputBuffer.writeIndex();

        
        
        int readIndex;

        while ((readIndex = m_backgroundStages[0]->inputReadIndex()) != writeIndex) { 
            
            const int SliceSize = MinFFTSize / 2;

            
            for (size_t i = 0; i < m_backgroundStages.size(); ++i)
                m_backgroundStages[i]->processInBackground(this, SliceSize);
        }
    }
}

void ReverbConvolver::process(const AudioChannel* sourceChannel, AudioChannel* destinationChannel, size_t framesToProcess)
{
    bool isSafe = sourceChannel && destinationChannel && sourceChannel->length() >= framesToProcess && destinationChannel->length() >= framesToProcess;
    ASSERT(isSafe);
    if (!isSafe)
        return;
        
    const float* source = sourceChannel->data();
    float* destination = destinationChannel->mutableData();
    bool isDataSafe = source && destination;
    ASSERT(isDataSafe);
    if (!isDataSafe)
        return;

    
    m_inputBuffer.write(source, framesToProcess);

    
    for (size_t i = 0; i < m_stages.size(); ++i)
        m_stages[i]->process(source, framesToProcess);

    
    m_accumulationBuffer.readAndClear(destination, framesToProcess);
        
    
    
    
    
    
    
    
    if (m_backgroundThreadLock.tryLock()) {
        m_moreInputBuffered = true;
        m_backgroundThreadCondition.signal();
        m_backgroundThreadLock.unlock();
    }
}

void ReverbConvolver::reset()
{
    for (size_t i = 0; i < m_stages.size(); ++i)
        m_stages[i]->reset();

    for (size_t i = 0; i < m_backgroundStages.size(); ++i)
        m_backgroundStages[i]->reset();

    m_accumulationBuffer.reset();
    m_inputBuffer.reset();
}

size_t ReverbConvolver::latencyFrames() const
{
    return 0;
}

} 

#endif 
