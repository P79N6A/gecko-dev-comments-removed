




















































#include "cpu_detect.h"
#include "STTypes.h"

using namespace soundtouch;

#ifdef SOUNDTOUCH_ALLOW_SSE









#include "TDStretch.h"
#include <xmmintrin.h>
#include <math.h>


double TDStretchSSE::calcCrossCorr(const float *pV1, const float *pV2) const
{
    int i;
    const float *pVec1;
    const __m128 *pVec2;
    __m128 vSum, vNorm;

    
    
    
    
    
    
    
    

#ifdef SOUNDTOUCH_ALLOW_NONEXACT_SIMD_OPTIMIZATION
    
    

    #define _MM_LOAD    _mm_load_ps

    if (((ulong)pV1) & 15) return -1e50;    

#else
    
    
    #define _MM_LOAD    _mm_loadu_ps
#endif 

    
    assert((overlapLength % 8) == 0);

    
    
    pVec1 = (const float*)pV1;
    pVec2 = (const __m128*)pV2;
    vSum = vNorm = _mm_setzero_ps();

    
    
    for (i = 0; i < channels * overlapLength / 16; i ++) 
    {
        __m128 vTemp;
        
        vTemp = _MM_LOAD(pVec1);
        vSum  = _mm_add_ps(vSum,  _mm_mul_ps(vTemp ,pVec2[0]));
        vNorm = _mm_add_ps(vNorm, _mm_mul_ps(vTemp ,vTemp));

        
        vTemp = _MM_LOAD(pVec1 + 4);
        vSum  = _mm_add_ps(vSum, _mm_mul_ps(vTemp, pVec2[1]));
        vNorm = _mm_add_ps(vNorm, _mm_mul_ps(vTemp ,vTemp));

        
        vTemp = _MM_LOAD(pVec1 + 8);
        vSum  = _mm_add_ps(vSum, _mm_mul_ps(vTemp, pVec2[2]));
        vNorm = _mm_add_ps(vNorm, _mm_mul_ps(vTemp ,vTemp));

        
        vTemp = _MM_LOAD(pVec1 + 12);
        vSum  = _mm_add_ps(vSum, _mm_mul_ps(vTemp, pVec2[3]));
        vNorm = _mm_add_ps(vNorm, _mm_mul_ps(vTemp ,vTemp));

        pVec1 += 16;
        pVec2 += 4;
    }

    
    float *pvNorm = (float*)&vNorm;
    double norm = sqrt(pvNorm[0] + pvNorm[1] + pvNorm[2] + pvNorm[3]);
    if (norm < 1e-9) norm = 1.0;    

    float *pvSum = (float*)&vSum;
    return (double)(pvSum[0] + pvSum[1] + pvSum[2] + pvSum[3]) / norm;

    































}








#include "FIRFilter.h"

FIRFilterSSE::FIRFilterSSE() : FIRFilter()
{
    filterCoeffsAlign = NULL;
    filterCoeffsUnalign = NULL;
}


FIRFilterSSE::~FIRFilterSSE()
{
    delete[] filterCoeffsUnalign;
    filterCoeffsAlign = NULL;
    filterCoeffsUnalign = NULL;
}



void FIRFilterSSE::setCoefficients(const float *coeffs, uint newLength, uint uResultDivFactor)
{
    uint i;
    float fDivider;

    FIRFilter::setCoefficients(coeffs, newLength, uResultDivFactor);

    
    
    
    delete[] filterCoeffsUnalign;
    filterCoeffsUnalign = new float[2 * newLength + 4];
    filterCoeffsAlign = (float *)(((unsigned long)filterCoeffsUnalign + 15) & (ulong)-16);

    fDivider = (float)resultDivider;

    
    for (i = 0; i < newLength; i ++)
    {
        filterCoeffsAlign[2 * i + 0] =
        filterCoeffsAlign[2 * i + 1] = coeffs[i + 0] / fDivider;
    }
}




uint FIRFilterSSE::evaluateFilterStereo(float *dest, const float *source, uint numSamples) const
{
    int count = (int)((numSamples - length) & (uint)-2);
    int j;

    assert(count % 2 == 0);

    if (count < 2) return 0;

    assert(source != NULL);
    assert(dest != NULL);
    assert((length % 8) == 0);
    assert(filterCoeffsAlign != NULL);
    assert(((ulong)filterCoeffsAlign) % 16 == 0);

    
    for (j = 0; j < count; j += 2)
    {
        const float *pSrc;
        const __m128 *pFil;
        __m128 sum1, sum2;
        uint i;

        pSrc = (const float*)source;              
        pFil = (const __m128*)filterCoeffsAlign;  
                                                  
        sum1 = sum2 = _mm_setzero_ps();

        for (i = 0; i < length / 8; i ++) 
        {
            
            

            
            

            sum1 = _mm_add_ps(sum1, _mm_mul_ps(_mm_loadu_ps(pSrc)    , pFil[0]));
            sum2 = _mm_add_ps(sum2, _mm_mul_ps(_mm_loadu_ps(pSrc + 2), pFil[0]));

            sum1 = _mm_add_ps(sum1, _mm_mul_ps(_mm_loadu_ps(pSrc + 4), pFil[1]));
            sum2 = _mm_add_ps(sum2, _mm_mul_ps(_mm_loadu_ps(pSrc + 6), pFil[1]));

            sum1 = _mm_add_ps(sum1, _mm_mul_ps(_mm_loadu_ps(pSrc + 8) ,  pFil[2]));
            sum2 = _mm_add_ps(sum2, _mm_mul_ps(_mm_loadu_ps(pSrc + 10), pFil[2]));

            sum1 = _mm_add_ps(sum1, _mm_mul_ps(_mm_loadu_ps(pSrc + 12), pFil[3]));
            sum2 = _mm_add_ps(sum2, _mm_mul_ps(_mm_loadu_ps(pSrc + 14), pFil[3]));

            pSrc += 16;
            pFil += 4;
        }

        
        

        
        _mm_storeu_ps(dest, _mm_add_ps(
                    _mm_shuffle_ps(sum1, sum2, _MM_SHUFFLE(1,0,3,2)),   
                    _mm_shuffle_ps(sum1, sum2, _MM_SHUFFLE(3,2,1,0))    
                    ));
        source += 4;
        dest += 4;
    }

    
    
    
    
    

    return (uint)count;

    


















































}

#endif  
