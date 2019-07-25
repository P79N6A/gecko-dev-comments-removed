









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_TEST_DEBUGUTILITY_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_TEST_DEBUGUTILITY_H_

#include <stdio.h>
#include <string.h>
#include "utility.h"

typedef struct 
{
    FILE*  res0to4FilePtr;
    FILE*  res4to8FilePtr;
    FILE*  res8to12FilePtr;
    FILE*  res8to16FilePtr;

    FILE*  res0to4DecFilePtr;
    FILE*  res4to8DecFilePtr;
    FILE*  res8to12DecFilePtr;
    FILE*  res8to16DecFilePtr;

    FILE*  in0to4FilePtr;
    FILE*  in4to8FilePtr;
    FILE*  in8to12FilePtr;
    FILE*  in8to16FilePtr;

    FILE*  out0to4FilePtr;
    FILE*  out4to8FilePtr;
    FILE*  out8to12FilePtr;
    FILE*  out8to16FilePtr;

    FILE*  fftFilePtr;
    FILE*  fftDecFilePtr;

    FILE*  arrivalTime;
    
    float  lastArrivalTime;

    int    prevPacketLost;
    int    currPacketLost;
    int    nextPacketLost;

    
    int    packetLossPercent;

    int maxPayloadLB;
    int maxPayloadUB;
    int lbBytes;
    int ubBytes;
    

}debugStruct;


#define PRINT_ENTROPY_INFO(obj)                                         \
    do                                                                  \
    {                                                                   \
        printf("%10u, %u; ",                                            \
            obj->bitstr_obj.streamval, obj->bitstr_obj.stream_index);   \
    } while(0)  

int setupDebugStruct(debugStruct* str);

#endif