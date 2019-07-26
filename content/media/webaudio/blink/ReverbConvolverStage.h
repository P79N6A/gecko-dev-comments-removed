



























#ifndef ReverbConvolverStage_h
#define ReverbConvolverStage_h

#include "core/platform/audio/AudioArray.h"
#include "core/platform/audio/FFTFrame.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class ReverbAccumulationBuffer;
class ReverbConvolver;
class FFTConvolver;
class DirectConvolver;
    


class ReverbConvolverStage {
public:
    
    
    ReverbConvolverStage(const float* impulseResponse, size_t responseLength, size_t reverbTotalLatency, size_t stageOffset, size_t stageLength, size_t fftSize, size_t renderPhase, size_t renderSliceSize, ReverbAccumulationBuffer*, bool directMode = false);

    
    void process(const float* source, size_t framesToProcess);

    void processInBackground(ReverbConvolver* convolver, size_t framesToProcess);

    void reset();

    
    int inputReadIndex() const { return m_inputReadIndex; }

private:
    OwnPtr<FFTFrame> m_fftKernel;
    OwnPtr<FFTConvolver> m_fftConvolver;

    AudioFloatArray m_preDelayBuffer;

    ReverbAccumulationBuffer* m_accumulationBuffer;
    int m_accumulationReadIndex;
    int m_inputReadIndex;

    size_t m_preDelayLength;
    size_t m_postDelayLength;
    size_t m_preReadWriteIndex;
    size_t m_framesProcessed;

    AudioFloatArray m_temporaryBuffer;

    bool m_directMode;
    OwnPtr<AudioFloatArray> m_directKernel;
    OwnPtr<DirectConvolver> m_directConvolver;
};

} 

#endif 
