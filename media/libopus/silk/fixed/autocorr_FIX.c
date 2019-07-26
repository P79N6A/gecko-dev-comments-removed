


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FIX.h"
#include "celt_lpc.h"


void silk_autocorr(
    opus_int32                  *results,           
    opus_int                    *scale,             
    const opus_int16            *inputData,         
    const opus_int              inputDataSize,      
    const opus_int              correlationCount    
)
{
    opus_int   corrCount;
    corrCount = silk_min_int( inputDataSize, correlationCount );
    *scale = _celt_autocorr(inputData, results, NULL, 0, corrCount-1, inputDataSize);
}
