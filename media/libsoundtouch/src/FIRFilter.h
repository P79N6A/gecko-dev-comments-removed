








































#ifndef FIRFilter_H
#define FIRFilter_H

#include <stddef.h>
#include "STTypes.h"

namespace soundtouch
{

class FIRFilter 
{
protected:
    
    uint length;    
    
    uint lengthDiv8;

    
    uint resultDivFactor;

    
    SAMPLETYPE resultDivider;

    
    SAMPLETYPE *filterCoeffs;

    virtual uint evaluateFilterStereo(SAMPLETYPE *dest, 
                                      const SAMPLETYPE *src, 
                                      uint numSamples) const;
    virtual uint evaluateFilterMono(SAMPLETYPE *dest, 
                                    const SAMPLETYPE *src, 
                                    uint numSamples) const;
    virtual uint evaluateFilterMulti(SAMPLETYPE *dest, const SAMPLETYPE *src, uint numSamples, uint numChannels);

public:
    FIRFilter();
    virtual ~FIRFilter();

    
    
    static void * operator new(size_t s);

    static FIRFilter *newInstance();

    
    
    
    
    
    uint evaluate(SAMPLETYPE *dest, 
                  const SAMPLETYPE *src, 
                  uint numSamples, 
                  uint numChannels);

    uint getLength() const;

    virtual void setCoefficients(const SAMPLETYPE *coeffs, 
                                 uint newLength, 
                                 uint uResultDivFactor);
};




#ifdef SOUNDTOUCH_ALLOW_MMX


    class FIRFilterMMX : public FIRFilter
    {
    protected:
        short *filterCoeffsUnalign;
        short *filterCoeffsAlign;

        virtual uint evaluateFilterStereo(short *dest, const short *src, uint numSamples) const;
    public:
        FIRFilterMMX();
        ~FIRFilterMMX();

        virtual void setCoefficients(const short *coeffs, uint newLength, uint uResultDivFactor);
    };

#endif 


#ifdef SOUNDTOUCH_ALLOW_SSE
    
    class FIRFilterSSE : public FIRFilter
    {
    protected:
        float *filterCoeffsUnalign;
        float *filterCoeffsAlign;

        virtual uint evaluateFilterStereo(float *dest, const float *src, uint numSamples) const;
    public:
        FIRFilterSSE();
        ~FIRFilterSSE();

        virtual void setCoefficients(const float *coeffs, uint newLength, uint uResultDivFactor);
    };

#endif 

}

#endif  
