













#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_MAIN_SOURCE_AEC_CORE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_MAIN_SOURCE_AEC_CORE_H_

#ifdef WEBRTC_AEC_DEBUG_DUMP
#include <stdio.h>
#endif

#include "typedefs.h"

#define FRAME_LEN 80
#define PART_LEN 64 // Length of partition
#define PART_LEN1 (PART_LEN + 1) // Unique fft coefficients
#define PART_LEN2 (PART_LEN * 2) // Length of partition * 2
#define NR_PART 12  // Number of partitions in filter.
#define PREF_BAND_SIZE 24


enum { kMaxDelayBlocks = 60 };
enum { kLookaheadBlocks = 15 };
enum { kHistorySizeBlocks = kMaxDelayBlocks + kLookaheadBlocks };

typedef float complex_t[2];









enum {offsetLevel = -100};

typedef struct {
    float sfrsum;
    int sfrcounter;
    float framelevel;
    float frsum;
    int frcounter;
    float minlevel;
    float averagelevel;
} power_level_t;

typedef struct {
    float instant;
    float average;
    float min;
    float max;
    float sum;
    float hisum;
    float himean;
    int counter;
    int hicounter;
} stats_t;

typedef struct {
    int farBufWritePos, farBufReadPos;

    int knownDelay;
    int inSamples, outSamples;
    int delayEstCtr;

    void *nearFrBuf, *outFrBuf;

    void *nearFrBufH;
    void *outFrBufH;

    float dBuf[PART_LEN2]; 
    float eBuf[PART_LEN2]; 

    float dBufH[PART_LEN2]; 

    float xPow[PART_LEN1];
    float dPow[PART_LEN1];
    float dMinPow[PART_LEN1];
    float dInitMinPow[PART_LEN1];
    float *noisePow;

    float xfBuf[2][NR_PART * PART_LEN1]; 
    float wfBuf[2][NR_PART * PART_LEN1]; 
    complex_t sde[PART_LEN1]; 
    complex_t sxd[PART_LEN1]; 
    complex_t xfwBuf[NR_PART * PART_LEN1]; 

    float sx[PART_LEN1], sd[PART_LEN1], se[PART_LEN1]; 
    float hNs[PART_LEN1];
    float hNlFbMin, hNlFbLocalMin;
    float hNlXdAvgMin;
    int hNlNewMin, hNlMinCtr;
    float overDrive, overDriveSm;
    float targetSupp, minOverDrive;
    float outBuf[PART_LEN];
    int delayIdx;

    short stNearState, echoState;
    short divergeState;

    int xfBufBlockPos;

    void* far_buf;
    void* far_buf_windowed;
    int system_delay;  

    int mult;  
    int sampFreq;
    WebRtc_UWord32 seed;

    float mu; 
    float errThresh; 

    int noiseEstCtr;

    power_level_t farlevel;
    power_level_t nearlevel;
    power_level_t linoutlevel;
    power_level_t nlpoutlevel;

    int metricsMode;
    int stateCounter;
    stats_t erl;
    stats_t erle;
    stats_t aNlp;
    stats_t rerl;

    
    int freq_avg_ic;         
    int flag_Hband_cn;      
    float cn_scale_Hband;   

    int delay_histogram[kHistorySizeBlocks];
    int delay_logging_enabled;
    void* delay_estimator;

#ifdef WEBRTC_AEC_DEBUG_DUMP
    void* far_time_buf;
    FILE *farFile;
    FILE *nearFile;
    FILE *outFile;
    FILE *outLinearFile;
#endif
} aec_t;

typedef void (*WebRtcAec_FilterFar_t)(aec_t *aec, float yf[2][PART_LEN1]);
extern WebRtcAec_FilterFar_t WebRtcAec_FilterFar;
typedef void (*WebRtcAec_ScaleErrorSignal_t)(aec_t *aec, float ef[2][PART_LEN1]);
extern WebRtcAec_ScaleErrorSignal_t WebRtcAec_ScaleErrorSignal;
typedef void (*WebRtcAec_FilterAdaptation_t)
  (aec_t *aec, float *fft, float ef[2][PART_LEN1]);
extern WebRtcAec_FilterAdaptation_t WebRtcAec_FilterAdaptation;
typedef void (*WebRtcAec_OverdriveAndSuppress_t)
  (aec_t *aec, float hNl[PART_LEN1], const float hNlFb, float efw[2][PART_LEN1]);
extern WebRtcAec_OverdriveAndSuppress_t WebRtcAec_OverdriveAndSuppress;

int WebRtcAec_CreateAec(aec_t **aec);
int WebRtcAec_FreeAec(aec_t *aec);
int WebRtcAec_InitAec(aec_t *aec, int sampFreq);
void WebRtcAec_InitAec_SSE2(void);

void WebRtcAec_InitMetrics(aec_t *aec);
void WebRtcAec_BufferFarendPartition(aec_t *aec, const float* farend);
void WebRtcAec_ProcessFrame(aec_t* aec,
                            const short *nearend,
                            const short *nearendH,
                            int knownDelay);




int WebRtcAec_MoveFarReadPtr(aec_t* aec, int elements);

#endif  
