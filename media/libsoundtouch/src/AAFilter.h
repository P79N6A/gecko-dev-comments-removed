










































#ifndef AAFilter_H
#define AAFilter_H

#include "STTypes.h"

namespace soundtouch
{

class AAFilter
{
protected:
    class FIRFilter *pFIR;

    
    double cutoffFreq;

    
    uint length;

    
    void calculateCoeffs();
public:
    AAFilter(uint length);

    ~AAFilter();

    
    
    
    void setCutoffFreq(double newCutoffFreq);

    
    void setLength(uint newLength);

    uint getLength() const;

    
    
    
    uint evaluate(SAMPLETYPE *dest, 
                  const SAMPLETYPE *src, 
                  uint numSamples, 
                  uint numChannels) const;
};

}

#endif
