









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AGC_MAIN_SOURCE_DIGITAL_AGC_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AGC_MAIN_SOURCE_DIGITAL_AGC_H_

#ifdef AGC_DEBUG
#include <stdio.h>
#endif
#include "typedefs.h"
#include "signal_processing_library.h"


#define AGC_MUL32(A, B)             (((B)>>13)*(A) + ( ((0x00001FFF & (B))*(A)) >> 13 ))

#define AGC_SCALEDIFF32(A, B, C)    ((C) + ((B)>>16)*(A) + ( ((0x0000FFFF & (B))*(A)) >> 16 ))

typedef struct
{
    WebRtc_Word32 downState[8];
    WebRtc_Word16 HPstate;
    WebRtc_Word16 counter;
    WebRtc_Word16 logRatio; 
    WebRtc_Word16 meanLongTerm; 
    WebRtc_Word32 varianceLongTerm; 
    WebRtc_Word16 stdLongTerm; 
    WebRtc_Word16 meanShortTerm; 
    WebRtc_Word32 varianceShortTerm; 
    WebRtc_Word16 stdShortTerm; 
} AgcVad_t; 

typedef struct
{
    WebRtc_Word32 capacitorSlow;
    WebRtc_Word32 capacitorFast;
    WebRtc_Word32 gain;
    WebRtc_Word32 gainTable[32];
    WebRtc_Word16 gatePrevious;
    WebRtc_Word16 agcMode;
    AgcVad_t      vadNearend;
    AgcVad_t      vadFarend;
#ifdef AGC_DEBUG
    FILE*         logFile;
    int           frameCounter;
#endif
} DigitalAgc_t;

WebRtc_Word32 WebRtcAgc_InitDigital(DigitalAgc_t *digitalAgcInst, WebRtc_Word16 agcMode);

WebRtc_Word32 WebRtcAgc_ProcessDigital(DigitalAgc_t *digitalAgcInst, const WebRtc_Word16 *inNear,
                             const WebRtc_Word16 *inNear_H, WebRtc_Word16 *out,
                             WebRtc_Word16 *out_H, WebRtc_UWord32 FS,
                             WebRtc_Word16 lowLevelSignal);

WebRtc_Word32 WebRtcAgc_AddFarendToDigital(DigitalAgc_t *digitalAgcInst, const WebRtc_Word16 *inFar,
                                 WebRtc_Word16 nrSamples);

void WebRtcAgc_InitVad(AgcVad_t *vadInst);

WebRtc_Word16 WebRtcAgc_ProcessVad(AgcVad_t *vadInst, 
                            const WebRtc_Word16 *in, 
                            WebRtc_Word16 nrSamples); 

WebRtc_Word32 WebRtcAgc_CalculateGainTable(WebRtc_Word32 *gainTable, 
                                 WebRtc_Word16 compressionGaindB, 
                                 WebRtc_Word16 targetLevelDbfs,
                                 WebRtc_UWord8 limiterEnable, WebRtc_Word16 analogTarget);

#endif 
