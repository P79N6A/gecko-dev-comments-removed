






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FLP.h"


void silk_scale_vector_FLP(
    silk_float          *data1,
    silk_float          gain,
    opus_int            dataSize
)
{
    opus_int  i, dataSize4;

    
    dataSize4 = dataSize & 0xFFFC;
    for( i = 0; i < dataSize4; i += 4 ) {
        data1[ i + 0 ] *= gain;
        data1[ i + 1 ] *= gain;
        data1[ i + 2 ] *= gain;
        data1[ i + 3 ] *= gain;
    }

    
    for( ; i < dataSize; i++ ) {
        data1[ i ] *= gain;
    }
}
