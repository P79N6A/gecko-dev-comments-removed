







































#ifndef _InterpolateShannon_H_
#define _InterpolateShannon_H_

#include "RateTransposer.h"
#include "STTypes.h"

namespace soundtouch
{

class InterpolateShannon : public TransposerBase
{
protected:
    void resetRegisters();
    int transposeMono(SAMPLETYPE *dest, 
                        const SAMPLETYPE *src, 
                        int &srcSamples);
    int transposeStereo(SAMPLETYPE *dest, 
                        const SAMPLETYPE *src, 
                        int &srcSamples);
    int transposeMulti(SAMPLETYPE *dest, 
                        const SAMPLETYPE *src, 
                        int &srcSamples);

    float fract;

public:
    InterpolateShannon();
};

}

#endif
