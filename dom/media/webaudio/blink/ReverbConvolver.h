



























#ifndef ReverbConvolver_h
#define ReverbConvolver_h

#include "ReverbAccumulationBuffer.h"
#include "ReverbInputBuffer.h"
#include "nsAutoPtr.h"
#include "mozilla/MemoryReporting.h"
#ifdef LOG
#undef LOG
#endif
#include "base/condition_variable.h"
#include "base/lock.h"
#include "base/thread.h"

namespace WebCore {

class ReverbConvolverStage;

class ReverbConvolver {
public:
    
    
    
    
    ReverbConvolver(const float* impulseResponseData, size_t impulseResponseLength, size_t renderSliceSize, size_t maxFFTSize, size_t convolverRenderPhase, bool useBackgroundThreads);
    ~ReverbConvolver();

    void process(const float* sourceChannelData, size_t sourceChannelLength,
                 float* destinationChannelData, size_t destinationChannelLength,
                 size_t framesToProcess);
    void reset();

    size_t impulseResponseLength() const { return m_impulseResponseLength; }

    ReverbInputBuffer* inputBuffer() { return &m_inputBuffer; }

    bool useBackgroundThreads() const { return m_useBackgroundThreads; }
    void backgroundThreadEntry();

    size_t latencyFrames() const;

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
private:
    nsTArray<nsAutoPtr<ReverbConvolverStage> > m_stages;
    nsTArray<nsAutoPtr<ReverbConvolverStage> > m_backgroundStages;
    size_t m_impulseResponseLength;

    ReverbAccumulationBuffer m_accumulationBuffer;

    
    ReverbInputBuffer m_inputBuffer;

    
    size_t m_minFFTSize;
    size_t m_maxFFTSize;

    
    size_t m_maxRealtimeFFTSize;

    
    base::Thread m_backgroundThread;
    Lock m_backgroundThreadLock;
    ConditionVariable m_backgroundThreadCondition;
    bool m_useBackgroundThreads;
    bool m_wantsToExit;
    bool m_moreInputBuffered;
};

} 

#endif 
