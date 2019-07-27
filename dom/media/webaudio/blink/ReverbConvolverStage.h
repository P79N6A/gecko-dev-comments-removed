



























#ifndef ReverbConvolverStage_h
#define ReverbConvolverStage_h

#include "DirectConvolver.h"
#include "FFTConvolver.h"

#include "nsTArray.h"
#include "mozilla/FFTBlock.h"
#include "mozilla/MemoryReporting.h"

namespace WebCore {

using mozilla::FFTBlock;

class ReverbAccumulationBuffer;
class ReverbConvolver;



class ReverbConvolverStage {
public:
    
    
    ReverbConvolverStage(const float* impulseResponse, size_t responseLength, size_t reverbTotalLatency, size_t stageOffset, size_t stageLength, size_t fftSize, size_t renderPhase, size_t renderSliceSize, ReverbAccumulationBuffer*, bool directMode = false);

    
    void process(const float* source, size_t framesToProcess);

    void processInBackground(ReverbConvolver* convolver, size_t framesToProcess);

    void reset();

    
    int inputReadIndex() const { return m_inputReadIndex; }

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    nsAutoPtr<FFTBlock> m_fftKernel;
    nsAutoPtr<FFTConvolver> m_fftConvolver;

    nsTArray<float> m_preDelayBuffer;

    ReverbAccumulationBuffer* m_accumulationBuffer;
    int m_accumulationReadIndex;
    int m_inputReadIndex;

    size_t m_preDelayLength;
    size_t m_postDelayLength;
    size_t m_preReadWriteIndex;
    size_t m_framesProcessed;

    nsTArray<float> m_temporaryBuffer;

    bool m_directMode;
    nsTArray<float> m_directKernel;
    nsAutoPtr<DirectConvolver> m_directConvolver;
};

} 

#endif 
