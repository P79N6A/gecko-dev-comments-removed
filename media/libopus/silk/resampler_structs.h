






























#ifndef SILK_RESAMPLER_STRUCTS_H
#define SILK_RESAMPLER_STRUCTS_H

#ifdef __cplusplus
extern "C" {
#endif

#define SILK_RESAMPLER_MAX_FIR_ORDER                 36
#define SILK_RESAMPLER_MAX_IIR_ORDER                 6

typedef struct _silk_resampler_state_struct{
    opus_int32       sIIR[ SILK_RESAMPLER_MAX_IIR_ORDER ]; 
    opus_int32       sFIR[ SILK_RESAMPLER_MAX_FIR_ORDER ];
    opus_int16       delayBuf[ 48 ];
    opus_int         resampler_function;
    opus_int         batchSize;
    opus_int32       invRatio_Q16;
    opus_int         FIR_Order;
    opus_int         FIR_Fracs;
    opus_int         Fs_in_kHz;
    opus_int         Fs_out_kHz;
    opus_int         inputDelay;
    const opus_int16 *Coefs;
} silk_resampler_state_struct;

#ifdef __cplusplus
}
#endif
#endif

