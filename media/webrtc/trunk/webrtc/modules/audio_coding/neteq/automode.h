













#ifndef AUTOMODE_H
#define AUTOMODE_H

#include "typedefs.h"







#define AUTOMODE_BETA_INV_Q30 53687091  /* 1/20 in Q30 */
#define AUTOMODE_STREAMING_BETA_INV_Q30 536871 /* 1/2000 in Q30 */


#define IAT_PROB_FACT 32745       /* 0.9993 in Q15 */


#define MAX_IAT 64
#define PEAK_HEIGHT 20            /* 0.08s in Q8 */


#define AUTOMODE_TIMESCALE_LIMIT (1<<5)



#define NUM_PEAKS 8


#define PEAK_INDEX_MASK 0x0007


#define MAX_PEAK_PERIOD 10
#define MAX_STREAMING_PEAK_PERIOD 600 /* 10 minutes */


#define NUM_PEAKS_REQUIRED 3


#define CSUM_IAT_DRIFT 2








typedef struct
{

    
    uint16_t levelFiltFact; 
    int buffLevelFilt; 

    
    int32_t iatProb[MAX_IAT + 1]; 
    int16_t iatProbFact; 
    uint32_t packetIatCountSamp; 

    int optBufLevel; 

    
    int16_t packetSpeechLenSamp; 
    int16_t lastPackCNGorDTMF; 

    uint16_t lastSeqNo; 
    uint32_t lastTimeStamp; 
    int firstPacketReceived; 

    int32_t sampleMemory; 

    int16_t prevTimeScale; 

    uint32_t timescaleHoldOff; 


    int16_t extraDelayMs; 

    int minimum_delay_ms; 

    int maximum_delay_ms; 


    int required_delay_q8; 



    
    
    uint32_t peakPeriodSamp[NUM_PEAKS];
    
    int16_t peakHeightPkt[NUM_PEAKS];
    int16_t peakIndex; 

    uint16_t peakThresholdPkt; 

    uint32_t peakIatCountSamp; 
    uint32_t curPeakPeriod; 
    int16_t curPeakHeight; 

    int16_t peakModeDisabled; 
    uint16_t peakFound; 


    
    uint32_t countIAT500ms; 
    uint32_t countIAT1000ms; 
    uint32_t countIAT2000ms; 
    uint32_t longestIATms; 

    int16_t cSumIatQ8; 
    int16_t maxCSumIatQ8; 
    uint32_t maxCSumUpdateTimer;
} AutomodeInst_t;






























int WebRtcNetEQ_UpdateIatStatistics(AutomodeInst_t *inst, int maxBufLen,
                                    uint16_t seqNumber, uint32_t timeStamp,
                                    int32_t fsHz, int mdCodec, int streamingMode);






















int16_t WebRtcNetEQ_CalcOptimalBufLvl(AutomodeInst_t *inst, int32_t fsHz,
                                      int mdCodec, uint32_t timeIatPkts,
                                      int streamingMode);






















int WebRtcNetEQ_BufferLevelFilter(int32_t curSizeMs8, AutomodeInst_t *inst,
                                  int sampPerCall, int16_t fsMult);





















int WebRtcNetEQ_SetPacketSpeechLen(AutomodeInst_t *inst, int16_t newLenSamp,
                                   int32_t fsHz);


















int WebRtcNetEQ_ResetAutomode(AutomodeInst_t *inst, int maxBufLenPackets);


















int32_t WebRtcNetEQ_AverageIAT(const AutomodeInst_t *inst);

#endif 
