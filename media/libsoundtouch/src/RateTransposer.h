











































#ifndef RateTransposer_H
#define RateTransposer_H

#include <stddef.h>
#include "AAFilter.h"
#include "FIFOSamplePipe.h"
#include "FIFOSampleBuffer.h"

#include "STTypes.h"

namespace soundtouch
{


class TransposerBase
{
public:
        enum ALGORITHM {
        LINEAR = 0,
        CUBIC,
        SHANNON
    };

protected:
    virtual void resetRegisters() = 0;

    virtual int transposeMono(SAMPLETYPE *dest, 
                        const SAMPLETYPE *src, 
                        int &srcSamples)  = 0;
    virtual int transposeStereo(SAMPLETYPE *dest, 
                        const SAMPLETYPE *src, 
                        int &srcSamples) = 0;
    virtual int transposeMulti(SAMPLETYPE *dest, 
                        const SAMPLETYPE *src, 
                        int &srcSamples) = 0;

    static ALGORITHM algorithm;

public:
    float rate;
    int numChannels;

    TransposerBase();
    virtual ~TransposerBase();

    virtual int transpose(FIFOSampleBuffer &dest, FIFOSampleBuffer &src);
    virtual void setRate(float newRate);
    virtual void setChannels(int channels);

    
    static TransposerBase *newInstance();

    
    static void setAlgorithm(ALGORITHM a);
};




class RateTransposer : public FIFOProcessor
{
protected:
    
    AAFilter *pAAFilter;
    TransposerBase *pTransposer;

    
    
    FIFOSampleBuffer inputBuffer;

    
    FIFOSampleBuffer midBuffer;

    
    FIFOSampleBuffer outputBuffer;

    bool bUseAAFilter;


    
    
    
    
    void processSamples(const SAMPLETYPE *src, 
                        uint numSamples);

public:
    RateTransposer();
    virtual ~RateTransposer();

    
    


    
    
    


    
    FIFOSamplePipe *getOutput() { return &outputBuffer; };

    


    
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
