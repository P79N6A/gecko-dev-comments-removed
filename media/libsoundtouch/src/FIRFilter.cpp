








































#include <memory.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "FIRFilter.h"
#include "cpu_detect.h"

#ifdef _MSC_VER
#include <malloc.h>
#define alloca _alloca
#endif

using namespace soundtouch;







FIRFilter::FIRFilter()
{
    resultDivFactor = 0;
    resultDivider = 0;
    length = 0;
    lengthDiv8 = 0;
    filterCoeffs = NULL;
}


FIRFilter::~FIRFilter()
{
    delete[] filterCoeffs;
}


uint FIRFilter::evaluateFilterStereo(SAMPLETYPE *dest, const SAMPLETYPE *src, uint numSamples) const
{
    uint i, j, end;
    LONG_SAMPLETYPE suml, sumr;
#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    
    
    double dScaler = 1.0 / (double)resultDivider;
#endif

    assert(length != 0);
    assert(src != NULL);
    assert(dest != NULL);
    assert(filterCoeffs != NULL);

    end = 2 * (numSamples - length);

    for (j = 0; j < end; j += 2) 
    {
        const SAMPLETYPE *ptr;

        suml = sumr = 0;
        ptr = src + j;

        for (i = 0; i < length; i += 4) 
        {
            
            suml += ptr[2 * i + 0] * filterCoeffs[i + 0] +
                    ptr[2 * i + 2] * filterCoeffs[i + 1] +
                    ptr[2 * i + 4] * filterCoeffs[i + 2] +
                    ptr[2 * i + 6] * filterCoeffs[i + 3];
            sumr += ptr[2 * i + 1] * filterCoeffs[i + 0] +
                    ptr[2 * i + 3] * filterCoeffs[i + 1] +
                    ptr[2 * i + 5] * filterCoeffs[i + 2] +
                    ptr[2 * i + 7] * filterCoeffs[i + 3];
        }

#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        suml >>= resultDivFactor;
        sumr >>= resultDivFactor;
        
        suml = (suml < -32768) ? -32768 : (suml > 32767) ? 32767 : suml;
        
        sumr = (sumr < -32768) ? -32768 : (sumr > 32767) ? 32767 : sumr;
#else
        suml *= dScaler;
        sumr *= dScaler;
#endif 
        dest[j] = (SAMPLETYPE)suml;
        dest[j + 1] = (SAMPLETYPE)sumr;
    }
    return numSamples - length;
}





uint FIRFilter::evaluateFilterMono(SAMPLETYPE *dest, const SAMPLETYPE *src, uint numSamples) const
{
    uint i, j, end;
    LONG_SAMPLETYPE sum;
#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    
    
    double dScaler = 1.0 / (double)resultDivider;
#endif


    assert(length != 0);

    end = numSamples - length;
    for (j = 0; j < end; j ++) 
    {
        sum = 0;
        for (i = 0; i < length; i += 4) 
        {
            
            sum += src[i + 0] * filterCoeffs[i + 0] + 
                   src[i + 1] * filterCoeffs[i + 1] + 
                   src[i + 2] * filterCoeffs[i + 2] + 
                   src[i + 3] * filterCoeffs[i + 3];
        }
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        sum >>= resultDivFactor;
        
        sum = (sum < -32768) ? -32768 : (sum > 32767) ? 32767 : sum;
#else
        sum *= dScaler;
#endif 
        dest[j] = (SAMPLETYPE)sum;
        src ++;
    }
    return end;
}


uint FIRFilter::evaluateFilterMulti(SAMPLETYPE *dest, const SAMPLETYPE *src, uint numSamples, uint numChannels) const
{
    uint i, j, end, c;
    LONG_SAMPLETYPE *sum=(LONG_SAMPLETYPE*)alloca(numChannels*sizeof(*sum));
#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    
    
    double dScaler = 1.0 / (double)resultDivider;
#endif

    assert(length != 0);
    assert(src != NULL);
    assert(dest != NULL);
    assert(filterCoeffs != NULL);

    end = numChannels * (numSamples - length);

    for (c = 0; c < numChannels; c ++)
    {
        sum[c] = 0;
    }

    for (j = 0; j < end; j += numChannels)
    {
        const SAMPLETYPE *ptr;

        ptr = src + j;

        for (i = 0; i < length; i ++)
        {
            SAMPLETYPE coef=filterCoeffs[i];
            for (c = 0; c < numChannels; c ++)
            {
                sum[c] += ptr[0] * coef;
                ptr ++;
            }
        }
        
        for (c = 0; c < numChannels; c ++)
        {
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
            sum[c] >>= resultDivFactor;
#else
            sum[c] *= dScaler;
#endif 
            *dest = (SAMPLETYPE)sum[c];
            dest++;
            sum[c] = 0;
        }
    }
    return numSamples - length;
}





void FIRFilter::setCoefficients(const SAMPLETYPE *coeffs, uint newLength, uint uResultDivFactor)
{
    assert(newLength > 0);
    if (newLength % 8) ST_THROW_RT_ERROR("FIR filter length not divisible by 8");

    lengthDiv8 = newLength / 8;
    length = lengthDiv8 * 8;
    assert(length == newLength);

    resultDivFactor = uResultDivFactor;
    resultDivider = (SAMPLETYPE)::pow(2.0, (int)resultDivFactor);

    delete[] filterCoeffs;
    filterCoeffs = new SAMPLETYPE[length];
    memcpy(filterCoeffs, coeffs, length * sizeof(SAMPLETYPE));
}


uint FIRFilter::getLength() const
{
    return length;
}







uint FIRFilter::evaluate(SAMPLETYPE *dest, const SAMPLETYPE *src, uint numSamples, uint numChannels) const
{
    assert(length > 0);
    assert(lengthDiv8 * 8 == length);

    if (numSamples < length) return 0;

#ifndef USE_MULTICH_ALWAYS
    if (numChannels == 1)
    {
        return evaluateFilterMono(dest, src, numSamples);
    } 
    else if (numChannels == 2)
    {
        return evaluateFilterStereo(dest, src, numSamples);
    }
    else
#endif 
    {
        assert(numChannels > 0);
        return evaluateFilterMulti(dest, src, numSamples, numChannels);
    }
}





void * FIRFilter::operator new(size_t s)
{
    
    ST_THROW_RT_ERROR("Error in FIRFilter::new: Don't use 'new FIRFilter', use 'newInstance' member instead!");
    return newInstance();
}


FIRFilter * FIRFilter::newInstance()
{
#if defined(SOUNDTOUCH_ALLOW_MMX) || defined(SOUNDTOUCH_ALLOW_SSE)
    uint uExtensions;

    uExtensions = detectCPUextensions();
#endif

    

#ifdef SOUNDTOUCH_ALLOW_MMX
    
    if (uExtensions & SUPPORT_MMX)
    {
        return ::new FIRFilterMMX;
    }
    else
#endif 

#ifdef SOUNDTOUCH_ALLOW_SSE
    if (uExtensions & SUPPORT_SSE)
    {
        
        return ::new FIRFilterSSE;
    }
    else
#endif 

    {
        
        return ::new FIRFilter;
    }
}
