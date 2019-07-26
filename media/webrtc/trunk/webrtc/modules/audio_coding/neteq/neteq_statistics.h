













#include "typedefs.h"

#ifndef NETEQ_STATISTICS_H
#define NETEQ_STATISTICS_H




typedef struct
{

    
    uint32_t expandLength; 
    uint32_t preemptiveLength; 

    uint32_t accelerateLength; 
    int addedSamples; 

    
    uint32_t expandedVoiceSamples; 
    uint32_t expandedNoiseSamples; 


} DSPStats_t;

typedef struct {
  int preemptive_expand_bgn_samples;
  int preemptive_expand_normal_samples;

  int expand_bgn_samples;
  int expand_normal_samples;

  int merge_expand_bgn_samples;
  int merge_expand_normal_samples;

  int accelerate_bgn_samples;
  int accelarate_normal_samples;
} ActivityStats;


#endif

