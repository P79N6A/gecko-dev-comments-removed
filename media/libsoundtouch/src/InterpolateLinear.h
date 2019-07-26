


































#ifndef _InterpolateLinear_H_
#define _InterpolateLinear_H_

#include "RateTransposer.h"
#include "STTypes.h"

namespace soundtouch
{


class InterpolateLinearInteger : public TransposerBase
{
protected:
    int iFract;
    int iRate;

    virtual void resetRegisters();

    virtual int transposeMono(SAMPLETYPE *dest, 
                       const SAMPLETYPE *src, 
                       int &srcSamples);
    virtual int transposeStereo(SAMPLETYPE *dest, 
                         const SAMPLETYPE *src, 
                         int &srcSamples);
    virtual int transposeMulti(SAMPLETYPE *dest, const SAMPLETYPE *src, int &srcSamples);
public:
    InterpolateLinearInteger();

    
    
    virtual void setRate(float newRate);
};



class InterpolateLinearFloat : public TransposerBase
{
protected:
    float fract;

    virtual void resetRegisters();

    virtual int transposeMono(SAMPLETYPE *dest, 
                       const SAMPLETYPE *src, 
                       int &srcSamples);
    virtual int transposeStereo(SAMPLETYPE *dest, 
                         const SAMPLETYPE *src, 
                         int &srcSamples);
    virtual int transposeMulti(SAMPLETYPE *dest, const SAMPLETYPE *src, int &srcSamples);

public:
    InterpolateLinearFloat();
};

}

#endif
