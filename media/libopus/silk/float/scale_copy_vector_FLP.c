






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FLP.h"


void silk_scale_copy_vector_FLP(
    silk_float          *data_out,
    const silk_float    *data_in,
    silk_float          gain,
    opus_int            dataSize
)
{
    opus_int  i, dataSize4;

    
    dataSize4 = dataSize & 0xFFFC;
    for( i = 0; i < dataSize4; i += 4 ) {
        data_out[ i + 0 ] = gain * data_in[ i + 0 ];
        data_out[ i + 1 ] = gain * data_in[ i + 1 ];
        data_out[ i + 2 ] = gain * data_in[ i + 2 ];
        data_out[ i + 3 ] = gain * data_in[ i + 3 ];
    }

    
    for( ; i < dataSize; i++ ) {
        data_out[ i ] = gain * data_in[ i ];
    }
}
