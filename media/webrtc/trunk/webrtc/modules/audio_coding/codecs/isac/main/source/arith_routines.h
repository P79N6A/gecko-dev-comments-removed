
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ARITH_ROUTINES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ARITH_ROUTINES_H_

#include "structs.h"


int WebRtcIsac_EncLogisticMulti2(
    Bitstr *streamdata,              
    int16_t *dataQ7,           
    const uint16_t *env,       
    const int N,                     
    const int16_t isSWB12kHz); 


int WebRtcIsac_EncTerminate(Bitstr *streamdata); 


int WebRtcIsac_DecLogisticMulti2(
    int16_t *data,             
    Bitstr *streamdata,              
    const uint16_t *env,       
    const int16_t *dither,     
    const int N,                     
    const int16_t isSWB12kHz); 

void WebRtcIsac_EncHistMulti(
    Bitstr *streamdata,         
    const int *data,            
    const uint16_t **cdf, 
    const int N);               

int WebRtcIsac_DecHistBisectMulti(
    int *data,                      
    Bitstr *streamdata,             
    const uint16_t **cdf,     
    const uint16_t *cdf_size, 
    const int N);                   

int WebRtcIsac_DecHistOneStepMulti(
    int *data,                       
    Bitstr *streamdata,              
    const uint16_t **cdf,      
    const uint16_t *init_index,
    const int N);                    

#endif 
