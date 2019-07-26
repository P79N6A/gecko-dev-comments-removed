



























#ifndef ReverbConvolver_h
#define ReverbConvolver_h

#include "core/platform/audio/AudioArray.h"
#include "core/platform/audio/DirectConvolver.h"
#include "core/platform/audio/FFTConvolver.h"
#include "core/platform/audio/ReverbAccumulationBuffer.h"
#include "core/platform/audio/ReverbConvolverStage.h"
#include "core/platform/audio/ReverbInputBuffer.h"
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

namespace WebCore {

class AudioChannel;

class ReverbConvolver {
public:
    
    
    
    
    ReverbConvolver(AudioChannel* impulseResponse, size_t renderSliceSize, size_t maxFFTSize, size_t convolverRenderPhase, bool useBackgroundThreads);
    ~ReverbConvolver();

    void process(const AudioChannel* sourceChannel, AudioChannel* destinationChannel, size_t framesToProcess);
    void reset();

    size_t impulseResponseLength() const { return m_impulseResponseLength; }

    ReverbInputBuffer* inputBuffer() { return &m_inputBuffer; }

    bool useBackgroundThreads() const { return m_useBackgroundThreads; }
    void backgroundThreadEntry();

    size_t latencyFrames() const;
private:
    Vector<OwnPtr<ReverbConvolverStage> > m_stages;
    Vector<OwnPtr<ReverbConvolverStage> > m_backgroundStages;
    size_t m_impulseResponseLength;

    ReverbAccumulationBuffer m_accumulationBuffer;

    
    ReverbInputBuffer m_inputBuffer;

    
    size_t m_minFFTSize;
    size_t m_maxFFTSize;

    
    size_t m_maxRealtimeFFTSize;

    
    bool m_useBackgroundThreads;
    ThreadIdentifier m_backgroundThread;
    bool m_wantsToExit;
    bool m_moreInputBuffered;
    mutable Mutex m_backgroundThreadLock;
    mutable ThreadCondition m_backgroundThreadCondition;
};

} 

#endif 
