






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"




opus_int silk_init_decoder(
    silk_decoder_state          *psDec                          
)
{
    
    silk_memset( psDec, 0, sizeof( silk_decoder_state ) );

    
    psDec->first_frame_after_reset = 1;
    psDec->prev_gain_Q16 = 65536;

    
    silk_CNG_Reset( psDec );

    
    silk_PLC_Reset( psDec );

    return(0);
}

