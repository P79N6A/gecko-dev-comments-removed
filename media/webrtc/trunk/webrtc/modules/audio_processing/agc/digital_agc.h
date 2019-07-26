









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
    int32_t downState[8];
    int16_t HPstate;
    int16_t counter;
    int16_t logRatio; 
    int16_t meanLongTerm; 
    int32_t varianceLongTerm; 
    int16_t stdLongTerm; 
    int16_t meanShortTerm; 
    int32_t varianceShortTerm; 
    int16_t stdShortTerm; 
} AgcVad_t; 

typedef struct
{
    int32_t capacitorSlow;
    int32_t capacitorFast;
    int32_t gain;
    int32_t gainTable[32];
    int16_t gatePrevious;
    int16_t agcMode;
    AgcVad_t      vadNearend;
    AgcVad_t      vadFarend;
#ifdef AGC_DEBUG
    FILE*         logFile;
    int           frameCounter;
#endif
} DigitalAgc_t;

int32_t WebRtcAgc_InitDigital(DigitalAgc_t *digitalAgcInst, int16_t agcMode);

int32_t WebRtcAgc_ProcessDigital(DigitalAgc_t *digitalAgcInst,
                                 const int16_t *inNear, const int16_t *inNear_H,
                                 int16_t *out, int16_t *out_H, uint32_t FS,
                                 int16_t lowLevelSignal);

int32_t WebRtcAgc_AddFarendToDigital(DigitalAgc_t *digitalAgcInst,
                                     const int16_t *inFar,
                                     int16_t nrSamples);

void WebRtcAgc_InitVad(AgcVad_t *vadInst);

int16_t WebRtcAgc_ProcessVad(AgcVad_t *vadInst, 
                             const int16_t *in, 
                             int16_t nrSamples); 

int32_t WebRtcAgc_CalculateGainTable(int32_t *gainTable, 
                                     int16_t compressionGaindB, 
                                     int16_t targetLevelDbfs,
                                     uint8_t limiterEnable,
                                     int16_t analogTarget);

#endif 
