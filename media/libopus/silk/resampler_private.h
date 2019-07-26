






























#ifndef SILK_RESAMPLER_PRIVATE_H
#define SILK_RESAMPLER_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "SigProc_FIX.h"
#include "resampler_structs.h"
#include "resampler_rom.h"


#define RESAMPLER_MAX_BATCH_SIZE_MS             10
#define RESAMPLER_MAX_FS_KHZ                    48
#define RESAMPLER_MAX_BATCH_SIZE_IN             ( RESAMPLER_MAX_BATCH_SIZE_MS * RESAMPLER_MAX_FS_KHZ )


void silk_resampler_private_IIR_FIR(
    void                            *SS,            
    opus_int16                      out[],          
    const opus_int16                in[],           
    opus_int32                      inLen           
);


void silk_resampler_private_down_FIR(
    void                            *SS,            
    opus_int16                      out[],          
    const opus_int16                in[],           
    opus_int32                      inLen           
);


void silk_resampler_private_up2_HQ_wrapper(
    void                            *SS,            
    opus_int16                      *out,           
    const opus_int16                *in,            
    opus_int32                      len             
);


void silk_resampler_private_up2_HQ(
    opus_int32                      *S,             
    opus_int16                      *out,           
    const opus_int16                *in,            
    opus_int32                      len             
);


void silk_resampler_private_AR2(
    opus_int32                      S[],            
    opus_int32                      out_Q8[],       
    const opus_int16                in[],           
    const opus_int16                A_Q14[],        
    opus_int32                      len             
);

#ifdef __cplusplus
}
#endif
#endif
