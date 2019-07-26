



























#ifndef FFTConvolver_h
#define FFTConvolver_h

#include "core/platform/audio/AudioArray.h"
#include "core/platform/audio/FFTFrame.h"

namespace WebCore {

class FFTConvolver {
public:
    
    FFTConvolver(size_t fftSize);

    
    
    
    
    
    
    
    void process(FFTFrame* fftKernel, const float* sourceP, float* destP, size_t framesToProcess);

    void reset();

    size_t fftSize() const { return m_frame.fftSize(); }

private:
    FFTFrame m_frame;

    
    size_t m_readWriteIndex;
    AudioFloatArray m_inputBuffer;

    
    AudioFloatArray m_outputBuffer;

    
    AudioFloatArray m_lastOverlapBuffer;
};

} 

#endif 
