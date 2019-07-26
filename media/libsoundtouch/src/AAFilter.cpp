









































#include <memory.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "AAFilter.h"
#include "FIRFilter.h"

using namespace soundtouch;

#define PI        3.141592655357989
#define TWOPI    (2 * PI)







AAFilter::AAFilter(uint len)
{
    pFIR = FIRFilter::newInstance();
    cutoffFreq = 0.5;
    setLength(len);
}



AAFilter::~AAFilter()
{
    delete pFIR;
}






void AAFilter::setCutoffFreq(double newCutoffFreq)
{
    cutoffFreq = newCutoffFreq;
    calculateCoeffs();
}




void AAFilter::setLength(uint newLength)
{
    length = newLength;
    calculateCoeffs();
}




void AAFilter::calculateCoeffs()
{
    uint i;
    double cntTemp, temp, tempCoeff,h, w;
    double fc2, wc;
    double scaleCoeff, sum;
    double *work;
    SAMPLETYPE *coeffs;

    assert(length >= 2);
    assert(length % 4 == 0);
    assert(cutoffFreq >= 0);
    assert(cutoffFreq <= 0.5);

    work = new double[length];
    coeffs = new SAMPLETYPE[length];

    fc2 = 2.0 * cutoffFreq; 
    wc = PI * fc2;
    tempCoeff = TWOPI / (double)length;

    sum = 0;
    for (i = 0; i < length; i ++) 
    {
        cntTemp = (double)i - (double)(length / 2);

        temp = cntTemp * wc;
        if (temp != 0) 
        {
            h = fc2 * sin(temp) / temp;                     
        } 
        else 
        {
            h = 1.0;
        }
        w = 0.54 + 0.46 * cos(tempCoeff * cntTemp);       

        temp = w * h;
        work[i] = temp;

        
        sum += temp;
    }

    
    assert(sum > 0);

    
    assert(work[length/2] > 0);
    assert(work[length/2 + 1] > -1e-6);
    assert(work[length/2 - 1] > -1e-6);

    
    
    scaleCoeff = 16384.0f / sum;

    for (i = 0; i < length; i ++) 
    {
        
        temp = work[i] * scaleCoeff;
        temp += (temp >= 0) ? 0.5 : -0.5;
        
        assert(temp >= -32768 && temp <= 32767);
        coeffs[i] = (SAMPLETYPE)temp;
    }

    
    pFIR->setCoefficients(coeffs, length, 14);

    delete[] work;
    delete[] coeffs;
}





uint AAFilter::evaluate(SAMPLETYPE *dest, const SAMPLETYPE *src, uint numSamples, uint numChannels) const
{
    return pFIR->evaluate(dest, src, numSamples, numChannels);
}


uint AAFilter::getLength() const
{
    return pFIR->getLength();
}
