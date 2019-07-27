



























#ifndef ReverbInputBuffer_h
#define ReverbInputBuffer_h

#include "nsTArray.h"
#include "mozilla/MemoryReporting.h"

namespace WebCore {


class ReverbInputBuffer {
public:
    explicit ReverbInputBuffer(size_t length);

    
    
    
    void write(const float* sourceP, size_t numberOfFrames);

    
    size_t writeIndex() const { return m_writeIndex; }

    
    
    
    
    float* directReadFrom(int* readIndex, size_t numberOfFrames);

    void reset();

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    {
        return m_buffer.SizeOfExcludingThis(aMallocSizeOf);
    }


private:
    nsTArray<float> m_buffer;
    size_t m_writeIndex;
};

} 

#endif 
