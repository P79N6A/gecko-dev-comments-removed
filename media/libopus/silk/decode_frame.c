


























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "stack_alloc.h"
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
    VARDECL( silk_decoder_control, psDecCtrl );
    opus_int         L, mv_len, ret = 0;
    VARDECL( opus_int, pulses );
    SAVE_STACK;

    L = psDec->frame_length;
    ALLOC( psDecCtrl, 1, silk_decoder_control );
    ALLOC( pulses, (L + SHELL_CODEC_FRAME_LENGTH - 1) &
                   ~(SHELL_CODEC_FRAME_LENGTH - 1), opus_int );
    psDecCtrl->LTP_scale_Q14 = 0;

    
    silk_assert( L > 0 && L <= MAX_FRAME_LENGTH );

    if(   lostFlag == FLAG_DECODE_NORMAL ||
        ( lostFlag == FLAG_DECODE_LBRR && psDec->LBRR_flags[ psDec->nFramesDecoded ] == 1 ) )
    {
        
        
        
        silk_decode_indices( psDec, psRangeDec, psDec->nFramesDecoded, lostFlag, condCoding );

        
        
        
        silk_decode_pulses( psRangeDec, pulses, psDec->indices.signalType,
                psDec->indices.quantOffsetType, psDec->frame_length );

        
        
        
        silk_decode_parameters( psDec, psDecCtrl, condCoding );

        
        
        
        silk_decode_core( psDec, psDecCtrl, pOut, pulses );

        
        
        
        silk_PLC( psDec, psDecCtrl, pOut, 0 );

        psDec->lossCnt = 0;
        psDec->prevSignalType = psDec->indices.signalType;
        silk_assert( psDec->prevSignalType >= 0 && psDec->prevSignalType <= 2 );

        
        psDec->first_frame_after_reset = 0;
    } else {
        
        silk_PLC( psDec, psDecCtrl, pOut, 1 );
    }

    
    
    
    silk_assert( psDec->ltp_mem_length >= psDec->frame_length );
    mv_len = psDec->ltp_mem_length - psDec->frame_length;
    silk_memmove( psDec->outBuf, &psDec->outBuf[ psDec->frame_length ], mv_len * sizeof(opus_int16) );
    silk_memcpy( &psDec->outBuf[ mv_len ], pOut, psDec->frame_length * sizeof( opus_int16 ) );

    
    
    
    silk_PLC_glue_frames( psDec, pOut, L );

    
    
    
    silk_CNG( psDec, psDecCtrl, pOut, L );

    
    psDec->lagPrev = psDecCtrl->pitchL[ psDec->nb_subfr - 1 ];

    
    *pN = L;

    RESTORE_STACK;
    return ret;
}
