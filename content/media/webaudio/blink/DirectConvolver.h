



























#ifndef DirectConvolver_h
#define DirectConvolver_h

#include "core/platform/audio/AudioArray.h"

#if USE(WEBAUDIO_IPP)
#include <ipps.h>
#endif 

namespace WebCore {

class DirectConvolver {
public:
    DirectConvolver(size_t inputBlockSize);

    void process(AudioFloatArray* convolutionKernel, const float* sourceP, float* destP, size_t framesToProcess);

    void reset();

private:
    size_t m_inputBlockSize;

#if USE(WEBAUDIO_IPP)
    AudioFloatArray m_overlayBuffer;
#endif 
    AudioFloatArray m_buffer;
};

} 

#endif 
