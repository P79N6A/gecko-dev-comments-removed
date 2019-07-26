






























#ifndef SILK_CONTROL_H
#define SILK_CONTROL_H

#include "typedef.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define FLAG_DECODE_NORMAL                      0
#define FLAG_PACKET_LOST                        1
#define FLAG_DECODE_LBRR                        2




typedef struct {
    
    opus_int32 nChannelsAPI;

    
    opus_int32 nChannelsInternal;

    
    opus_int32 API_sampleRate;

    
    opus_int32 maxInternalSampleRate;

    
    opus_int32 minInternalSampleRate;

    
    opus_int32 desiredInternalSampleRate;

    
    opus_int payloadSize_ms;

    
    opus_int32 bitRate;

    
    opus_int packetLossPercentage;

    
    opus_int complexity;

    
    opus_int useInBandFEC;

    
    opus_int useDTX;

    
    opus_int useCBR;

    
    opus_int maxBits;

    
    opus_int toMono;

    
    opus_int opusCanSwitch;

    
    opus_int32 internalSampleRate;

    
    opus_int allowBandwidthSwitch;

    
    opus_int inWBmodeWithoutVariableLP;

    
    opus_int stereoWidth_Q14;

    
    opus_int switchReady;

} silk_EncControlStruct;




typedef struct {
    
    opus_int32 nChannelsAPI;

    
    opus_int32 nChannelsInternal;

    
    opus_int32 API_sampleRate;

    
    opus_int32 internalSampleRate;

    
    opus_int payloadSize_ms;

    
    opus_int prevPitchLag;
} silk_DecControlStruct;

#ifdef __cplusplus
}
#endif

#endif
