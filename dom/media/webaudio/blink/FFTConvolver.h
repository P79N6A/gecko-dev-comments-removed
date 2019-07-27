



























#ifndef FFTConvolver_h
#define FFTConvolver_h

#include "nsTArray.h"
#include "mozilla/FFTBlock.h"
#include "mozilla/MemoryReporting.h"

namespace WebCore {

typedef nsTArray<float> AudioFloatArray;
using mozilla::FFTBlock;

class FFTConvolver {
public:
    
    explicit FFTConvolver(size_t fftSize);

    
    
    
    
    
    
    
    
    
    void process(FFTBlock* fftKernel, const float* sourceP, float* destP, size_t framesToProcess);

    void reset();

    size_t fftSize() const { return m_frame.FFTSize(); }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    FFTBlock m_frame;

    
    size_t m_readWriteIndex;
    AudioFloatArray m_inputBuffer;

    
    AudioFloatArray m_outputBuffer;

    
    AudioFloatArray m_lastOverlapBuffer;
};

} 

#endif 
