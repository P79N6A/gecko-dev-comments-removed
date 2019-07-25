
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ARITH_ROUTINES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_ARITH_ROUTINES_H_

#include "structs.h"


int WebRtcIsac_EncLogisticMulti2(
    Bitstr *streamdata,              
    WebRtc_Word16 *dataQ7,           
    const WebRtc_UWord16 *env,       
    const int N,                     
    const WebRtc_Word16 isSWB12kHz); 


int WebRtcIsac_EncTerminate(Bitstr *streamdata); 


int WebRtcIsac_DecLogisticMulti2(
    WebRtc_Word16 *data,             
    Bitstr *streamdata,              
    const WebRtc_UWord16 *env,       
    const WebRtc_Word16 *dither,     
    const int N,                     
    const WebRtc_Word16 isSWB12kHz); 

void WebRtcIsac_EncHistMulti(
    Bitstr *streamdata,         
    const int *data,            
    const WebRtc_UWord16 **cdf, 
    const int N);               

int WebRtcIsac_DecHistBisectMulti(
    int *data,                      
    Bitstr *streamdata,             
    const WebRtc_UWord16 **cdf,     
    const WebRtc_UWord16 *cdf_size, 
    const int N);                   

int WebRtcIsac_DecHistOneStepMulti(
    int *data,                       
    Bitstr *streamdata,              
    const WebRtc_UWord16 **cdf,      
    const WebRtc_UWord16 *init_index,
    const int N);                    

#endif 
