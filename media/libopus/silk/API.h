






























#ifndef SILK_API_H
#define SILK_API_H

#include "control.h"
#include "typedef.h"
#include "errors.h"
#include "entenc.h"
#include "entdec.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SILK_MAX_FRAMES_PER_PACKET  3


typedef struct {
    opus_int    VADFlag;                                
    opus_int    VADFlags[ SILK_MAX_FRAMES_PER_PACKET ]; 
    opus_int    inbandFECFlag;                          
} silk_TOC_struct;








opus_int silk_Get_Encoder_Size(                         
    opus_int                        *encSizeBytes       
);




opus_int silk_InitEncoder(                              
    void                            *encState,          
    silk_EncControlStruct           *encStatus          
);




opus_int silk_QueryEncoder(                             
    const void                      *encState,          
    silk_EncControlStruct           *encStatus          
);






opus_int silk_Encode(                                   
    void                            *encState,          
    silk_EncControlStruct           *encControl,        
    const opus_int16                *samplesIn,         
    opus_int                        nSamplesIn,         
    ec_enc                          *psRangeEnc,        
    opus_int                        *nBytesOut,         
    const opus_int                  prefillFlag         
);








opus_int silk_Get_Decoder_Size(                         
    opus_int                        *decSizeBytes       
);




opus_int silk_InitDecoder(                              
    void                            *decState           
);




opus_int silk_Decode(                                   
    void*                           decState,           
    silk_DecControlStruct*          decControl,         
    opus_int                        lostFlag,           
    opus_int                        newPacketFlag,      
    ec_dec                          *psRangeDec,        
    opus_int16                      *samplesOut,        
    opus_int32                      *nSamplesOut        
);




opus_int silk_get_TOC(
    const opus_uint8                *payload,           
    const opus_int                  nBytesIn,           
    const opus_int                  nFramesPerPayload,  
    silk_TOC_struct                 *Silk_TOC           
);

#ifdef __cplusplus
}
#endif

#endif
