












































#ifndef FIFOSampleBuffer_H
#define FIFOSampleBuffer_H

#include "FIFOSamplePipe.h"

namespace soundtouch
{






class FIFOSampleBuffer : public FIFOSamplePipe
{
private:
    
    SAMPLETYPE *buffer;

    
    
    SAMPLETYPE *bufferUnaligned;

    
    uint sizeInBytes;

    
    uint samplesInBuffer;

    
    uint channels;

    
    
    
    uint bufferPos;

    
    
    void rewind();

    
    void ensureCapacity(uint capacityRequirement);

    
    uint getCapacity() const;

public:

    
    FIFOSampleBuffer(int numChannels = 2     
                                              
                     );

    
    ~FIFOSampleBuffer();

    
    
    
    
    
    
    
    virtual SAMPLETYPE *ptrBegin();

    
    
    
    
    
    
    
    
    SAMPLETYPE *ptrEnd(
                uint slackCapacity   
                                     
                                     
                                     
                );

    
    
    virtual void putSamples(const SAMPLETYPE *samples,  
                            uint numSamples                         
                            );

    
    
    
    
    
    
    virtual void putSamples(uint numSamples   
                            );

    
    
    
    
    
    virtual uint receiveSamples(SAMPLETYPE *output, 
                                uint maxSamples                 
                                );

    
    
    
    
    
    virtual uint receiveSamples(uint maxSamples   
                                );

    
    virtual uint numSamples() const;

    
    void setChannels(int numChannels);

    
    virtual int isEmpty() const;

    
    virtual void clear();

    
    
    uint adjustAmountOfSamples(uint numSamples);
};

}

#endif
