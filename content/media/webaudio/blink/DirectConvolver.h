



























#ifndef DirectConvolver_h
#define DirectConvolver_h

#include "nsTArray.h"

namespace WebCore {

class DirectConvolver {
public:
    DirectConvolver(size_t inputBlockSize);

    void process(const nsTArray<float>* convolutionKernel, const float* sourceP, float* destP, size_t framesToProcess);

    void reset();

private:
    size_t m_inputBlockSize;

    nsTArray<float> m_buffer;
};

} 

#endif 
