






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "tuning_parameters.h"


opus_int silk_control_audio_bandwidth(
    silk_encoder_state          *psEncC,                        
    silk_EncControlStruct       *encControl                     
)
{
    opus_int   fs_kHz;
    opus_int32 fs_Hz;

    fs_kHz = psEncC->fs_kHz;
    fs_Hz = silk_SMULBB( fs_kHz, 1000 );
    if( fs_Hz == 0 ) {
        
        fs_Hz  = silk_min( psEncC->desiredInternal_fs_Hz, psEncC->API_fs_Hz );
        fs_kHz = silk_DIV32_16( fs_Hz, 1000 );
    } else if( fs_Hz > psEncC->API_fs_Hz || fs_Hz > psEncC->maxInternal_fs_Hz || fs_Hz < psEncC->minInternal_fs_Hz ) {
        
        fs_Hz  = psEncC->API_fs_Hz;
        fs_Hz  = silk_min( fs_Hz, psEncC->maxInternal_fs_Hz );
        fs_Hz  = silk_max( fs_Hz, psEncC->minInternal_fs_Hz );
        fs_kHz = silk_DIV32_16( fs_Hz, 1000 );
    } else {
        
        if( psEncC->sLP.transition_frame_no >= TRANSITION_FRAMES ) {
            
            psEncC->sLP.mode = 0;
        }
        if( psEncC->allow_bandwidth_switch || encControl->opusCanSwitch ) {
            
            if( silk_SMULBB( psEncC->fs_kHz, 1000 ) > psEncC->desiredInternal_fs_Hz )
            {
                
                if( psEncC->sLP.mode == 0 ) {
                    
                    psEncC->sLP.transition_frame_no = TRANSITION_FRAMES;

                    
                    silk_memset( psEncC->sLP.In_LP_State, 0, sizeof( psEncC->sLP.In_LP_State ) );
                }
                if( encControl->opusCanSwitch ) {
                    
                    psEncC->sLP.mode = 0;

                    
                    fs_kHz = psEncC->fs_kHz == 16 ? 12 : 8;
                } else {
                   if( psEncC->sLP.transition_frame_no <= 0 ) {
                       encControl->switchReady = 1;
                   } else {
                       
                       psEncC->sLP.mode = -2;
                   }
                }
            }
            else
            
            if( silk_SMULBB( psEncC->fs_kHz, 1000 ) < psEncC->desiredInternal_fs_Hz )
            {
                
                if( encControl->opusCanSwitch ) {
                    
                    fs_kHz = psEncC->fs_kHz == 8 ? 12 : 16;

                    
                    psEncC->sLP.transition_frame_no = 0;

                    
                    silk_memset( psEncC->sLP.In_LP_State, 0, sizeof( psEncC->sLP.In_LP_State ) );

                    
                    psEncC->sLP.mode = 1;
                } else {
                   if( psEncC->sLP.mode == 0 ) {
                       encControl->switchReady = 1;
                   } else {
                       
                       psEncC->sLP.mode = 1;
                   }
                }
            }
        }
    }

    return fs_kHz;
}
