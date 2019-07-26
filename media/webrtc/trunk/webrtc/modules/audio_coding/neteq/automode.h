













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

    
    WebRtc_UWord16 levelFiltFact; 
    WebRtc_UWord16 buffLevelFilt; 

    
    WebRtc_Word32 iatProb[MAX_IAT + 1]; 
    WebRtc_Word16 iatProbFact; 
    WebRtc_UWord32 packetIatCountSamp; 

    WebRtc_UWord16 optBufLevel; 

    
    WebRtc_Word16 packetSpeechLenSamp; 
    WebRtc_Word16 lastPackCNGorDTMF; 

    WebRtc_UWord16 lastSeqNo; 
    WebRtc_UWord32 lastTimeStamp; 
    WebRtc_Word32 sampleMemory; 

    WebRtc_Word16 prevTimeScale; 

    WebRtc_UWord32 timescaleHoldOff; 


    WebRtc_Word16 extraDelayMs; 

    
    
    WebRtc_UWord32 peakPeriodSamp[NUM_PEAKS];
    
    WebRtc_Word16 peakHeightPkt[NUM_PEAKS];
    WebRtc_Word16 peakIndex; 

    WebRtc_UWord16 peakThresholdPkt; 

    WebRtc_UWord32 peakIatCountSamp; 
    WebRtc_UWord32 curPeakPeriod; 
    WebRtc_Word16 curPeakHeight; 

    WebRtc_Word16 peakModeDisabled; 
    uint16_t peakFound; 


    
    WebRtc_UWord32 countIAT500ms; 
    WebRtc_UWord32 countIAT1000ms; 
    WebRtc_UWord32 countIAT2000ms; 
    WebRtc_UWord32 longestIATms; 

    WebRtc_Word16 cSumIatQ8; 
    WebRtc_Word16 maxCSumIatQ8; 
    WebRtc_UWord32 maxCSumUpdateTimer;

} AutomodeInst_t;






























int WebRtcNetEQ_UpdateIatStatistics(AutomodeInst_t *inst, int maxBufLen,
                                    WebRtc_UWord16 seqNumber, WebRtc_UWord32 timeStamp,
                                    WebRtc_Word32 fsHz, int mdCodec, int streamingMode);






















WebRtc_Word16 WebRtcNetEQ_CalcOptimalBufLvl(AutomodeInst_t *inst, WebRtc_Word32 fsHz,
                                            int mdCodec, WebRtc_UWord32 timeIatPkts,
                                            int streamingMode);






















int WebRtcNetEQ_BufferLevelFilter(WebRtc_Word32 curSizeMs8, AutomodeInst_t *inst,
                                  int sampPerCall, WebRtc_Word16 fsMult);





















int WebRtcNetEQ_SetPacketSpeechLen(AutomodeInst_t *inst, WebRtc_Word16 newLenSamp,
                                   WebRtc_Word32 fsHz);


















int WebRtcNetEQ_ResetAutomode(AutomodeInst_t *inst, int maxBufLenPackets);


















int32_t WebRtcNetEQ_AverageIAT(const AutomodeInst_t *inst);

#endif 
