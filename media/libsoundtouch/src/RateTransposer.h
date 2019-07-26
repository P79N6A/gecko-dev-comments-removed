











































#ifndef RateTransposer_H
#define RateTransposer_H

#include <stddef.h>
#include "AAFilter.h"
#include "FIFOSamplePipe.h"
#include "FIFOSampleBuffer.h"

#include "STTypes.h"

namespace soundtouch
{







class RateTransposer : public FIFOProcessor
{
protected:
    
    AAFilter *pAAFilter;

    float fRate;

    int numChannels;

    
    
    FIFOSampleBuffer storeBuffer;

    
    FIFOSampleBuffer tempBuffer;

    
    FIFOSampleBuffer outputBuffer;

    bool bUseAAFilter;

    virtual void resetRegisters() = 0;

    virtual uint transposeStereo(SAMPLETYPE *dest, 
                         const SAMPLETYPE *src, 
                         uint numSamples) = 0;
    virtual uint transposeMono(SAMPLETYPE *dest, 
                       const SAMPLETYPE *src, 
                       uint numSamples) = 0;
    inline uint transpose(SAMPLETYPE *dest, 
                   const SAMPLETYPE *src, 
                   uint numSamples);

    void downsample(const SAMPLETYPE *src, 
                    uint numSamples);
    void upsample(const SAMPLETYPE *src, 
                 uint numSamples);

    
    
    
    
    void processSamples(const SAMPLETYPE *src, 
                        uint numSamples);


public:
    RateTransposer();
    virtual ~RateTransposer();

    
    
    static void *operator new(size_t s);

    
    
    
    static RateTransposer *newInstance();

    
    FIFOSamplePipe *getOutput() { return &outputBuffer; };

    
    FIFOSamplePipe *getStore() { return &storeBuffer; };

    
    AAFilter *getAAFilter();

    
    void enableAAFilter(bool newMode);

    
    bool isAAFilterEnabled() const;

    
    
    virtual void setRate(float newRate);

    
    void setChannels(int channels);

    
    
    void putSamples(const SAMPLETYPE *samples, uint numSamples);

    
    void clear();

    
    int isEmpty() const;
};

}

#endif
