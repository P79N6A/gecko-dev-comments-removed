






























#ifndef SILK_PLC_H
#define SILK_PLC_H

#include "main.h"

#define BWE_COEF                        0.99
#define V_PITCH_GAIN_START_MIN_Q14      11469               /* 0.7 in Q14               */
#define V_PITCH_GAIN_START_MAX_Q14      15565               /* 0.95 in Q14              */
#define MAX_PITCH_LAG_MS                18
#define SA_THRES_Q8                     50
#define RAND_BUF_SIZE                   128
#define RAND_BUF_MASK                   ( RAND_BUF_SIZE - 1 )
#define LOG2_INV_LPC_GAIN_HIGH_THRES    3                   /* 2^3 = 8 dB LPC gain      */
#define LOG2_INV_LPC_GAIN_LOW_THRES     8                   /* 2^8 = 24 dB LPC gain     */
#define PITCH_DRIFT_FAC_Q16             655                 /* 0.01 in Q16              */

void silk_PLC_Reset(
    silk_decoder_state                  *psDec              
);

void silk_PLC(
    silk_decoder_state                  *psDec,             
    silk_decoder_control                *psDecCtrl,         
    opus_int16                          frame[],            
    opus_int                            lost                
);

void silk_PLC_glue_frames(
    silk_decoder_state                  *psDec,             
    opus_int16                          frame[],            
    opus_int                            length              
);

#endif

