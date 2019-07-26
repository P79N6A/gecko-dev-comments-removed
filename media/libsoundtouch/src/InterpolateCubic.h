


































#ifndef _InterpolateCubic_H_
#define _InterpolateCubic_H_

#include "RateTransposer.h"
#include "STTypes.h"

namespace soundtouch
{

class InterpolateCubic : public TransposerBase
{
protected:
    virtual void resetRegisters();
    virtual int transposeMono(SAMPLETYPE *dest, 
                        const SAMPLETYPE *src, 
                        int &srcSamples);
    virtual int transposeStereo(SAMPLETYPE *dest, 
                        const SAMPLETYPE *src, 
                        int &srcSamples);
    virtual int transposeMulti(SAMPLETYPE *dest, 
                        const SAMPLETYPE *src, 
                        int &srcSamples);

    float fract;

public:
    InterpolateCubic();
};

}

#endif
