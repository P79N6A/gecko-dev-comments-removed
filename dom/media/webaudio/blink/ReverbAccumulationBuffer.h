



























#ifndef ReverbAccumulationBuffer_h
#define ReverbAccumulationBuffer_h

#include "nsTArray.h"
#include "mozilla/MemoryReporting.h"

namespace WebCore {

typedef nsTArray<float> AudioFloatArray;




class ReverbAccumulationBuffer {
public:
    explicit ReverbAccumulationBuffer(size_t length);

    
    void readAndClear(float* destination, size_t numberOfFrames);

    
    
    
    
    int accumulate(float* source, size_t numberOfFrames, int* readIndex, size_t delayFrames);

    size_t readIndex() const { return m_readIndex; }
    void updateReadIndex(int* readIndex, size_t numberOfFrames) const;

    size_t readTimeFrame() const { return m_readTimeFrame; }

    void reset();

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    {
        return m_buffer.SizeOfExcludingThis(aMallocSizeOf);
    }

private:
    AudioFloatArray m_buffer;
    size_t m_readIndex;
    size_t m_readTimeFrame; 
};

} 

#endif 
