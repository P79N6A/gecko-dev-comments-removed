






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "typedef.h"
#include "SigProc_FLP.h"


void silk_autocorrelation_FLP(
    silk_float          *results,           
    const silk_float    *inputData,         
    opus_int            inputDataSize,      
    opus_int            correlationCount    
)
{
    opus_int i;

    if( correlationCount > inputDataSize ) {
        correlationCount = inputDataSize;
    }

    for( i = 0; i < correlationCount; i++ ) {
        results[ i ] =  (silk_float)silk_inner_product_FLP( inputData, inputData + i, inputDataSize - i );
    }
}
