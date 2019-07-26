

















































#include "STTypes.h"

#ifdef SOUNDTOUCH_ALLOW_MMX


using namespace soundtouch;







#include "TDStretch.h"
#include <mmintrin.h>
#include <limits.h>
#include <math.h>



double TDStretchMMX::calcCrossCorr(const short *pV1, const short *pV2) const
{
    const __m64 *pVec1, *pVec2;
    __m64 shifter;
    __m64 accu, normaccu;
    long corr, norm;
    int i;
   
    pVec1 = (__m64*)pV1;
    pVec2 = (__m64*)pV2;

    shifter = _m_from_int(overlapDividerBits);
    normaccu = accu = _mm_setzero_si64();

    
    
    for (i = 0; i < channels * overlapLength / 16; i ++)
    {
        __m64 temp, temp2;

        
        
        
        

        temp = _mm_add_pi32(_mm_madd_pi16(pVec1[0], pVec2[0]),
                            _mm_madd_pi16(pVec1[1], pVec2[1]));
        temp2 = _mm_add_pi32(_mm_madd_pi16(pVec1[0], pVec1[0]),
                             _mm_madd_pi16(pVec1[1], pVec1[1]));
        accu = _mm_add_pi32(accu, _mm_sra_pi32(temp, shifter));
        normaccu = _mm_add_pi32(normaccu, _mm_sra_pi32(temp2, shifter));

        temp = _mm_add_pi32(_mm_madd_pi16(pVec1[2], pVec2[2]),
                            _mm_madd_pi16(pVec1[3], pVec2[3]));
        temp2 = _mm_add_pi32(_mm_madd_pi16(pVec1[2], pVec1[2]),
                             _mm_madd_pi16(pVec1[3], pVec1[3]));
        accu = _mm_add_pi32(accu, _mm_sra_pi32(temp, shifter));
        normaccu = _mm_add_pi32(normaccu, _mm_sra_pi32(temp2, shifter));

        pVec1 += 4;
        pVec2 += 4;
    }

    
    

    accu = _mm_add_pi32(accu, _mm_srli_si64(accu, 32));
    corr = _m_to_int(accu);

    normaccu = _mm_add_pi32(normaccu, _mm_srli_si64(normaccu, 32));
    norm = _m_to_int(normaccu);

    
    _m_empty();

    
    
    if (norm == 0) norm = 1;    

    return (double)corr / sqrt((double)norm);
    
    
}



void TDStretchMMX::clearCrossCorrState()
{
    
    _m_empty();
    
}




void TDStretchMMX::overlapStereo(short *output, const short *input) const
{
    const __m64 *pVinput, *pVMidBuf;
    __m64 *pVdest;
    __m64 mix1, mix2, adder, shifter;
    int i;

    pVinput  = (const __m64*)input;
    pVMidBuf = (const __m64*)pMidBuffer;
    pVdest   = (__m64*)output;

    
    
    
    
    mix1  = _mm_set_pi16(0, overlapLength,   0, overlapLength);
    adder = _mm_set_pi16(1, -1, 1, -1);
    mix2  = _mm_add_pi16(mix1, adder);
    adder = _mm_add_pi16(adder, adder);

    
    
    shifter = _m_from_int(overlapDividerBits + 1);

    for (i = 0; i < overlapLength / 4; i ++)
    {
        __m64 temp1, temp2;
                
        
        temp1 = _mm_unpacklo_pi16(pVMidBuf[0], pVinput[0]);     
        temp2 = _mm_unpackhi_pi16(pVMidBuf[0], pVinput[0]);     

        
        temp1 = _mm_sra_pi32(_mm_madd_pi16(temp1, mix1), shifter);
        temp2 = _mm_sra_pi32(_mm_madd_pi16(temp2, mix2), shifter);
        pVdest[0] = _mm_packs_pi32(temp1, temp2); 

        
        mix1 = _mm_add_pi16(mix1, adder);
        mix2 = _mm_add_pi16(mix2, adder);

        

        
        temp1 = _mm_unpacklo_pi16(pVMidBuf[1], pVinput[1]);       
        temp2 = _mm_unpackhi_pi16(pVMidBuf[1], pVinput[1]);       

        
        temp1 = _mm_sra_pi32(_mm_madd_pi16(temp1, mix1), shifter);
        temp2 = _mm_sra_pi32(_mm_madd_pi16(temp2, mix2), shifter);
        pVdest[1] = _mm_packs_pi32(temp1, temp2); 

        
        mix1 = _mm_add_pi16(mix1, adder);
        mix2 = _mm_add_pi16(mix2, adder);

        pVinput  += 2;
        pVMidBuf += 2;
        pVdest   += 2;
    }

    _m_empty(); 
}








#include "FIRFilter.h"


FIRFilterMMX::FIRFilterMMX() : FIRFilter()
{
    filterCoeffsUnalign = NULL;
}


FIRFilterMMX::~FIRFilterMMX()
{
    delete[] filterCoeffsUnalign;
}



void FIRFilterMMX::setCoefficients(const short *coeffs, uint newLength, uint uResultDivFactor)
{
    uint i;
    FIRFilter::setCoefficients(coeffs, newLength, uResultDivFactor);

    
    delete[] filterCoeffsUnalign;
    filterCoeffsUnalign = new short[2 * newLength + 8];
    filterCoeffsAlign = (short *)SOUNDTOUCH_ALIGN_POINTER_16(filterCoeffsUnalign);

    
    for (i = 0;i < length; i += 4) 
    {
        filterCoeffsAlign[2 * i + 0] = coeffs[i + 0];
        filterCoeffsAlign[2 * i + 1] = coeffs[i + 2];
        filterCoeffsAlign[2 * i + 2] = coeffs[i + 0];
        filterCoeffsAlign[2 * i + 3] = coeffs[i + 2];

        filterCoeffsAlign[2 * i + 4] = coeffs[i + 1];
        filterCoeffsAlign[2 * i + 5] = coeffs[i + 3];
        filterCoeffsAlign[2 * i + 6] = coeffs[i + 1];
        filterCoeffsAlign[2 * i + 7] = coeffs[i + 3];
    }
}




uint FIRFilterMMX::evaluateFilterStereo(short *dest, const short *src, uint numSamples) const
{
    
    uint i, j;
    __m64 *pVdest = (__m64*)dest;

    if (length < 2) return 0;

    for (i = 0; i < (numSamples - length) / 2; i ++)
    {
        __m64 accu1;
        __m64 accu2;
        const __m64 *pVsrc = (const __m64*)src;
        const __m64 *pVfilter = (const __m64*)filterCoeffsAlign;

        accu1 = accu2 = _mm_setzero_si64();
        for (j = 0; j < lengthDiv8 * 2; j ++)
        {
            __m64 temp1, temp2;

            temp1 = _mm_unpacklo_pi16(pVsrc[0], pVsrc[1]);  
            temp2 = _mm_unpackhi_pi16(pVsrc[0], pVsrc[1]);  

            accu1 = _mm_add_pi32(accu1, _mm_madd_pi16(temp1, pVfilter[0]));  
            accu1 = _mm_add_pi32(accu1, _mm_madd_pi16(temp2, pVfilter[1]));  

            temp1 = _mm_unpacklo_pi16(pVsrc[1], pVsrc[2]);  

            accu2 = _mm_add_pi32(accu2, _mm_madd_pi16(temp2, pVfilter[0]));  
            accu2 = _mm_add_pi32(accu2, _mm_madd_pi16(temp1, pVfilter[1]));  

            
            

            
            

            pVfilter += 2;
            pVsrc += 2;
        }
        
        accu1 = _mm_srai_pi32(accu1, resultDivFactor);
        accu2 = _mm_srai_pi32(accu2, resultDivFactor);

        
        pVdest[0] = _mm_packs_pi32(accu1, accu2);
        src += 4;
        pVdest ++;
    }

   _m_empty();  

    return (numSamples & 0xfffffffe) - length;
}

#endif  
