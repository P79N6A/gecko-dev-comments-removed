













#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_CORE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_CORE_H_

#include "webrtc/typedefs.h"

#define FRAME_LEN 80
#define PART_LEN 64               // Length of partition
#define PART_LEN1 (PART_LEN + 1)  // Unique fft coefficients
#define PART_LEN2 (PART_LEN * 2)  // Length of partition * 2


enum {
  kMaxDelayBlocks = 60
};
enum {
  kLookaheadBlocks = 15
};
enum {
  kHistorySizeBlocks = kMaxDelayBlocks + kLookaheadBlocks
};

typedef float complex_t[2];









enum {
  kOffsetLevel = -100
};

typedef struct Stats {
  float instant;
  float average;
  float min;
  float max;
  float sum;
  float hisum;
  float himean;
  int counter;
  int hicounter;
} Stats;

typedef struct AecCore AecCore;

int WebRtcAec_CreateAec(AecCore** aec);
int WebRtcAec_FreeAec(AecCore* aec);
int WebRtcAec_InitAec(AecCore* aec, int sampFreq);
void WebRtcAec_InitAec_SSE2(void);

void WebRtcAec_BufferFarendPartition(AecCore* aec, const float* farend);
void WebRtcAec_ProcessFrame(AecCore* aec,
                            const short* nearend,
                            const short* nearendH,
                            int knownDelay,
                            int16_t* out,
                            int16_t* outH);




int WebRtcAec_MoveFarReadPtr(AecCore* aec, int elements);



int WebRtcAec_GetDelayMetricsCore(AecCore* self, int* median, int* std);


int WebRtcAec_echo_state(AecCore* self);


void WebRtcAec_GetEchoStats(AecCore* self,
                            Stats* erl,
                            Stats* erle,
                            Stats* a_nlp);
#ifdef WEBRTC_AEC_DEBUG_DUMP
void* WebRtcAec_far_time_buf(AecCore* self);
#endif


void WebRtcAec_SetConfigCore(AecCore* self,
                             int nlp_mode,
                             int metrics_mode,
                             int delay_logging);





void WebRtcAec_enable_delay_correction(AecCore* self, int enable);


int WebRtcAec_delay_correction_enabled(AecCore* self);



int WebRtcAec_system_delay(AecCore* self);




void WebRtcAec_SetSystemDelay(AecCore* self, int delay);

#endif  
