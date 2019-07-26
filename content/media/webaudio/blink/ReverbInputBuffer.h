



























#ifndef ReverbInputBuffer_h
#define ReverbInputBuffer_h

#include "core/platform/audio/AudioArray.h"

namespace WebCore {


class ReverbInputBuffer {
public:
    ReverbInputBuffer(size_t length);

    
    
    
    void write(const float* sourceP, size_t numberOfFrames);

    
    size_t writeIndex() const { return m_writeIndex; }

    
    
    
    
    float* directReadFrom(int* readIndex, size_t numberOfFrames);

    void reset();

private:
    AudioFloatArray m_buffer;
    size_t m_writeIndex;
};

} 

#endif 
