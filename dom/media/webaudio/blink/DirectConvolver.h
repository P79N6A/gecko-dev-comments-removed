



























#ifndef DirectConvolver_h
#define DirectConvolver_h

#include "nsTArray.h"
#include "mozilla/MemoryReporting.h"

namespace WebCore {

class DirectConvolver {
public:
    explicit DirectConvolver(size_t inputBlockSize);

    void process(const nsTArray<float>* convolutionKernel, const float* sourceP, float* destP, size_t framesToProcess);

    void reset();

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    {
        size_t amount = aMallocSizeOf(this);
        amount += m_buffer.SizeOfExcludingThis(aMallocSizeOf);
        return amount;
    }


private:
    size_t m_inputBlockSize;

    nsTArray<float> m_buffer;
};

} 

#endif 
