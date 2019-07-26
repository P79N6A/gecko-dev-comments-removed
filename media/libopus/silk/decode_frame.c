






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "PLC.h"




opus_int silk_decode_frame(
    silk_decoder_state          *psDec,                         
    ec_dec                      *psRangeDec,                    
    opus_int16                  pOut[],                         
    opus_int32                  *pN,                            
    opus_int                    lostFlag,                       
    opus_int                    condCoding                      
)
{
    silk_decoder_control sDecCtrl;
    opus_int         L, mv_len, ret = 0;
    opus_int         pulses[ MAX_FRAME_LENGTH ];

    L = psDec->frame_length;
    sDecCtrl.LTP_scale_Q14 = 0;

    
    silk_assert( L > 0 && L <= MAX_FRAME_LENGTH );

    if(   lostFlag == FLAG_DECODE_NORMAL ||
        ( lostFlag == FLAG_DECODE_LBRR && psDec->LBRR_flags[ psDec->nFramesDecoded ] == 1 ) )
    {
        
        
        
        silk_decode_indices( psDec, psRangeDec, psDec->nFramesDecoded, lostFlag, condCoding );

        
        
        
        silk_decode_pulses( psRangeDec, pulses, psDec->indices.signalType,
                psDec->indices.quantOffsetType, psDec->frame_length );

        
        
        
        silk_decode_parameters( psDec, &sDecCtrl, condCoding );

        
        L = psDec->frame_length;

        
        
        
        silk_decode_core( psDec, &sDecCtrl, pOut, pulses );

        
        
        
        silk_PLC( psDec, &sDecCtrl, pOut, 0 );

        psDec->lossCnt = 0;
        psDec->prevSignalType = psDec->indices.signalType;
        silk_assert( psDec->prevSignalType >= 0 && psDec->prevSignalType <= 2 );

        
        psDec->first_frame_after_reset = 0;
    } else {
        
        silk_PLC( psDec, &sDecCtrl, pOut, 1 );
    }

    
    
    
    silk_assert( psDec->ltp_mem_length >= psDec->frame_length );
    mv_len = psDec->ltp_mem_length - psDec->frame_length;
    silk_memmove( psDec->outBuf, &psDec->outBuf[ psDec->frame_length ], mv_len * sizeof(opus_int16) );
    silk_memcpy( &psDec->outBuf[ mv_len ], pOut, psDec->frame_length * sizeof( opus_int16 ) );

    
    
    
    silk_PLC_glue_frames( psDec, pOut, L );

    
    
    
    silk_CNG( psDec, &sDecCtrl, pOut, L );

    
    psDec->lagPrev = sDecCtrl.pitchL[ psDec->nb_subfr - 1 ];

    
    *pN = L;

    return ret;
}
