








































#ifndef OPUS_MULTISTREAM_H
#define OPUS_MULTISTREAM_H

#include "opus.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OpusMSEncoder OpusMSEncoder;
typedef struct OpusMSDecoder OpusMSDecoder;

#define __opus_check_encstate_ptr(ptr) ((ptr) + ((ptr) - (OpusEncoder**)(ptr)))
#define __opus_check_decstate_ptr(ptr) ((ptr) + ((ptr) - (OpusDecoder**)(ptr)))

#define OPUS_MULTISTREAM_GET_ENCODER_STATE_REQUEST 5120
#define OPUS_MULTISTREAM_GET_DECODER_STATE_REQUEST 5122

#define OPUS_MULTISTREAM_GET_ENCODER_STATE(x,y) OPUS_MULTISTREAM_GET_ENCODER_STATE_REQUEST, __opus_check_int(x), __opus_check_encstate_ptr(y)
#define OPUS_MULTISTREAM_GET_DECODER_STATE(x,y) OPUS_MULTISTREAM_GET_DECODER_STATE_REQUEST, __opus_check_int(x), __opus_check_decstate_ptr(y)




OPUS_EXPORT OpusMSEncoder *opus_multistream_encoder_create(
      opus_int32 Fs,            
      int channels,             
      int streams,              
      int coupled_streams,      
      unsigned char *mapping,   
      int application,          
      int *error                
);


OPUS_EXPORT int opus_multistream_encoder_init(
      OpusMSEncoder *st,        
      opus_int32 Fs,            
      int channels,             
      int streams,              
      int coupled_streams,      
      unsigned char *mapping,   
      int application           
);


OPUS_EXPORT int opus_multistream_encode(
    OpusMSEncoder *st,          
    const opus_int16 *pcm,      
    int frame_size,             
    unsigned char *data,        
    int max_data_bytes          
);


OPUS_EXPORT int opus_multistream_encode_float(
      OpusMSEncoder *st,        
      const float *pcm,         
      int frame_size,           
      unsigned char *data,      
      int max_data_bytes        
  );




OPUS_EXPORT opus_int32 opus_multistream_encoder_get_size(
      int streams,              
      int coupled_streams       
);


OPUS_EXPORT void opus_multistream_encoder_destroy(OpusMSEncoder *st);


OPUS_EXPORT int opus_multistream_encoder_ctl(OpusMSEncoder *st, int request, ...);




OPUS_EXPORT OpusMSDecoder *opus_multistream_decoder_create(
      opus_int32 Fs,            
      int channels,             
      int streams,              
      int coupled_streams,      
      unsigned char *mapping,   
      int *error                
);


OPUS_EXPORT int opus_multistream_decoder_init(
      OpusMSDecoder *st,        
      opus_int32 Fs,            
      int channels,             
      int streams,              
      int coupled_streams,      
      unsigned char *mapping    
);


OPUS_EXPORT int opus_multistream_decode(
    OpusMSDecoder *st,          
    const unsigned char *data,  
    int len,                    
    opus_int16 *pcm,            
    int frame_size,             
    int decode_fec              
                                
);


OPUS_EXPORT int opus_multistream_decode_float(
    OpusMSDecoder *st,          
    const unsigned char *data,  
    int len,                    
    float *pcm,                 
    int frame_size,             
    int decode_fec              
                                
);




OPUS_EXPORT opus_int32 opus_multistream_decoder_get_size(
      int streams,              
      int coupled_streams       
);


OPUS_EXPORT int opus_multistream_decoder_ctl(OpusMSDecoder *st, int request, ...);


OPUS_EXPORT void opus_multistream_decoder_destroy(OpusMSDecoder *st);

#ifdef __cplusplus
}
#endif

#endif
