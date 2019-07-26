













#include "typedefs.h"

#ifndef NETEQ_STATISTICS_H
#define NETEQ_STATISTICS_H




typedef struct
{

    
    WebRtc_UWord32 expandLength; 
    WebRtc_UWord32 preemptiveLength; 

    WebRtc_UWord32 accelerateLength; 
    int addedSamples; 

    
    WebRtc_UWord32 expandedVoiceSamples; 
    WebRtc_UWord32 expandedNoiseSamples; 


} DSPStats_t;

#endif

