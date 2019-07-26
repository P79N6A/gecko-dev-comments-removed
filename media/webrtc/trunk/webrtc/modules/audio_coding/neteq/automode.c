













#include "automode.h"

#include <assert.h>

#include "signal_processing_library.h"

#include "neteq_defines.h"

#ifdef NETEQ_DELAY_LOGGING

#include <stdio.h>
#include "delay_logging.h"

extern FILE *delay_fid2; 
#endif 


int WebRtcNetEQ_IsNewerSequenceNumber(uint16_t sequence_number,
                                      uint16_t prev_sequence_number) {
  return sequence_number != prev_sequence_number &&
         ((uint16_t) (sequence_number - prev_sequence_number)) < 0x8000;
}

int WebRtcNetEQ_IsNewerTimestamp(uint32_t timestamp, uint32_t prev_timestamp) {
  return timestamp != prev_timestamp &&
         ((uint32_t) (timestamp - prev_timestamp)) < 0x80000000;
}

int WebRtcNetEQ_UpdateIatStatistics(AutomodeInst_t *inst, int maxBufLen,
                                    uint16_t seqNumber, uint32_t timeStamp,
                                    int32_t fsHz, int mdCodec, int streamingMode)
{
    uint32_t timeIat; 
    int i;
    int32_t tempsum = 0; 
    int32_t tempvar; 
    int retval = 0; 
    int16_t packetLenSamp; 

    
    
    

    if (maxBufLen <= 1 || fsHz <= 0)
    {
        
        return -1;
    }

    
    
    

    
    if (!WebRtcNetEQ_IsNewerTimestamp(timeStamp, inst->lastTimeStamp) ||
        !WebRtcNetEQ_IsNewerSequenceNumber(seqNumber, inst->lastSeqNo))
    {
        
        packetLenSamp = inst->packetSpeechLenSamp; 
    }
    else
    {
        
        packetLenSamp = (int16_t) WebRtcSpl_DivU32U16(timeStamp - inst->lastTimeStamp,
            seqNumber - inst->lastSeqNo);
    }

    
    if (inst->firstPacketReceived && packetLenSamp > 0)
    { 

        
        timeIat = WebRtcSpl_DivW32W16(inst->packetIatCountSamp, packetLenSamp);

        
        if (streamingMode != 0)
        {
            



            int16_t timeIatQ8 = (int16_t) WebRtcSpl_DivW32W16(
                WEBRTC_SPL_LSHIFT_W32(inst->packetIatCountSamp, 8), packetLenSamp);

            



            inst->cSumIatQ8 += (timeIatQ8
                - WEBRTC_SPL_LSHIFT_W32(seqNumber - inst->lastSeqNo, 8));

            
            inst->cSumIatQ8 -= CSUM_IAT_DRIFT;

            
            inst->cSumIatQ8 = WEBRTC_SPL_MAX(inst->cSumIatQ8, 0);

            
            if (inst->cSumIatQ8 > inst->maxCSumIatQ8)
            {
                inst->maxCSumIatQ8 = inst->cSumIatQ8;
                inst->maxCSumUpdateTimer = 0;
            }

            
            if (inst->maxCSumUpdateTimer > (uint32_t) WEBRTC_SPL_MUL_32_16(fsHz,
                MAX_STREAMING_PEAK_PERIOD))
            {
                inst->maxCSumIatQ8 -= 4; 
            }
        } 

        
        if (WebRtcNetEQ_IsNewerSequenceNumber(seqNumber, inst->lastSeqNo + 1))
        {
            



            timeIat -= WEBRTC_SPL_MIN(timeIat,
                (uint16_t) (seqNumber - (uint16_t) (inst->lastSeqNo + 1)));
        }
        else if (!WebRtcNetEQ_IsNewerSequenceNumber(seqNumber, inst->lastSeqNo))
        {
            
            timeIat += (uint16_t) (inst->lastSeqNo + 1 - seqNumber);
        }

        
        timeIat = WEBRTC_SPL_MIN( timeIat, MAX_IAT );

        
        for (i = 0; i <= MAX_IAT; i++)
        {
            int32_t tempHi, tempLo; 

            




            





            tempHi = WEBRTC_SPL_MUL_16_16(inst->iatProbFact,
                (int16_t) WEBRTC_SPL_RSHIFT_W32(inst->iatProb[i], 16));
            tempHi = WEBRTC_SPL_LSHIFT_W32(tempHi, 1); 

            



            tempLo = inst->iatProb[i] & 0x0000FFFF; 
            tempLo = WEBRTC_SPL_MUL_16_U16(inst->iatProbFact,
                (uint16_t) tempLo);
            tempLo = WEBRTC_SPL_RSHIFT_W32(tempLo, 15);

            
            inst->iatProb[i] = tempHi + tempLo;

            
            tempsum += inst->iatProb[i];
        }

        




        inst->iatProb[timeIat] += (32768 - inst->iatProbFact) << 15;

        tempsum += (32768 - inst->iatProbFact) << 15; 

        



        inst->iatProbFact += (IAT_PROB_FACT - inst->iatProbFact + 3) >> 2;

        
        tempsum -= 1 << 30; 

        
        if (tempsum > 0)
        {
            
            i = 0;
            while (i <= MAX_IAT && tempsum > 0)
            {
                
                tempvar = WEBRTC_SPL_MIN(tempsum, inst->iatProb[i] >> 4);
                inst->iatProb[i++] -= tempvar;
                tempsum -= tempvar;
            }
        }
        else if (tempsum < 0)
        {
            
            i = 0;
            while (i <= MAX_IAT && tempsum < 0)
            {
                
                tempvar = WEBRTC_SPL_MIN(-tempsum, inst->iatProb[i] >> 4);
                inst->iatProb[i++] += tempvar;
                tempsum += tempvar;
            }
        }

        
        tempvar = (int32_t) WebRtcNetEQ_CalcOptimalBufLvl(inst, fsHz, mdCodec, timeIat,
            streamingMode);
        if (tempvar > 0)
        {
            int high_lim_delay;
            




            int32_t minimum_delay_q8 = ((inst->minimum_delay_ms *
                (fsHz / 1000)) << 8) / packetLenSamp;

            int32_t maximum_delay_q8 = ((inst->maximum_delay_ms *
              (fsHz / 1000)) << 8) / packetLenSamp;

            inst->optBufLevel = tempvar;

            if (streamingMode != 0)
            {
                inst->optBufLevel = WEBRTC_SPL_MAX(inst->optBufLevel,
                    inst->maxCSumIatQ8);
            }

            
            inst->required_delay_q8 = inst->optBufLevel;

            
            inst->optBufLevel = WEBRTC_SPL_MAX(inst->optBufLevel,
                                               minimum_delay_q8);

            if (maximum_delay_q8 > 0) {
              
              maximum_delay_q8 = WEBRTC_SPL_MAX(maximum_delay_q8, (1 << 8));
              inst->optBufLevel = WEBRTC_SPL_MIN(inst->optBufLevel,
                                                 maximum_delay_q8);
            }
            
            
            

            
            if (inst->extraDelayMs > 0 && inst->packetSpeechLenSamp > 0)
            {
                maxBufLen -= inst->extraDelayMs / inst->packetSpeechLenSamp * fsHz / 1000;
                maxBufLen = WEBRTC_SPL_MAX(maxBufLen, 1); 
            }

            maxBufLen = WEBRTC_SPL_LSHIFT_W32(maxBufLen, 8); 

            
            
            high_lim_delay = (maxBufLen >> 1) + (maxBufLen >> 2);
            inst->optBufLevel = WEBRTC_SPL_MIN(inst->optBufLevel,
                                               high_lim_delay);
            inst->required_delay_q8 = WEBRTC_SPL_MIN(inst->required_delay_q8,
                                                     high_lim_delay);
        }
        else
        {
            retval = (int) tempvar;
        }

    } 

    
    
    

    
    timeIat = WEBRTC_SPL_UDIV(
        WEBRTC_SPL_UMUL_32_16(inst->packetIatCountSamp, (int16_t) 1000),
        (uint32_t) fsHz);

    
    if (timeIat > 2000)
    {
        inst->countIAT2000ms++;
    }
    else if (timeIat > 1000)
    {
        inst->countIAT1000ms++;
    }
    else if (timeIat > 500)
    {
        inst->countIAT500ms++;
    }

    if (timeIat > inst->longestIATms)
    {
        
        inst->longestIATms = timeIat;
    }

    
    
    

    inst->packetIatCountSamp = 0; 

    inst->lastSeqNo = seqNumber; 

    inst->lastTimeStamp = timeStamp; 

    inst->firstPacketReceived = 1;

    return retval;
}


int16_t WebRtcNetEQ_CalcOptimalBufLvl(AutomodeInst_t *inst, int32_t fsHz,
                                      int mdCodec, uint32_t timeIatPkts,
                                      int streamingMode)
{

    int32_t sum1 = 1 << 30; 
    int16_t B;
    uint16_t Bopt;
    int i;
    int32_t betaInv; 

#ifdef NETEQ_DELAY_LOGGING
    
    int temp_var;
#endif

    
    
    

    if (fsHz <= 0)
    {
        
        return -1;
    }

    
    
    

    if (streamingMode)
    {
        
        betaInv = AUTOMODE_STREAMING_BETA_INV_Q30;
    }
    else
    {
        
        betaInv = AUTOMODE_BETA_INV_Q30;
    }

    
    
    

    



    B = 0; 
    sum1 -= inst->iatProb[B]; 

    do
    {
        



        sum1 -= inst->iatProb[++B];
    }
    while ((sum1 > betaInv) && (B < MAX_IAT));

    Bopt = B; 

    if (mdCodec)
    {
        




        int32_t sum2 = sum1; 

        while ((sum2 <= betaInv + inst->iatProb[Bopt]) && (Bopt > 0))
        {
            
            sum2 += inst->iatProb[Bopt--];
        }

        Bopt++; 

        
    }

#ifdef NETEQ_DELAY_LOGGING
    
    temp_var = NETEQ_DELAY_LOGGING_SIGNAL_OPTBUF;
    if (fwrite( &temp_var, sizeof(int), 1, delay_fid2 ) != 1) {
      return -1;
    }
    temp_var = (int) (Bopt * inst->packetSpeechLenSamp);
#endif

    
    
    

    switch (B)
    {
        case 0:
        case 1:
        {
            inst->levelFiltFact = 251;
            break;
        }
        case 2:
        case 3:
        {
            inst->levelFiltFact = 252;
            break;
        }
        case 4:
        case 5:
        case 6:
        case 7:
        {
            inst->levelFiltFact = 253;
            break;
        }
        default: 
        {
            inst->levelFiltFact = 254;
            break;
        }
    }

    
    
    

    




    if (timeIatPkts > (uint32_t) (Bopt + inst->peakThresholdPkt + (mdCodec != 0))
        || timeIatPkts > (uint32_t) WEBRTC_SPL_LSHIFT_U16(Bopt, 1))
    {
        

        if (inst->peakIndex == -1)
        {
            
            inst->peakIndex = 0;
            
            inst->peakModeDisabled = WEBRTC_SPL_LSHIFT_W16(1, NUM_PEAKS_REQUIRED-2);
        }
        else if (inst->peakIatCountSamp
            <=
            (uint32_t) WEBRTC_SPL_MUL_32_16(fsHz, MAX_PEAK_PERIOD))
        {
            

            
            inst->peakPeriodSamp[inst->peakIndex] = inst->peakIatCountSamp;

            
            inst->peakHeightPkt[inst->peakIndex]
                =
                (int16_t) WEBRTC_SPL_MIN(timeIatPkts, WEBRTC_SPL_WORD16_MAX);

            
            inst->peakIndex = (inst->peakIndex + 1) & PEAK_INDEX_MASK;

            
            inst->curPeakHeight = 0;
            inst->curPeakPeriod = 0;

            for (i = 0; i < NUM_PEAKS; i++)
            {
                
                inst->curPeakHeight
                    = WEBRTC_SPL_MAX(inst->curPeakHeight, inst->peakHeightPkt[i]);
                inst->curPeakPeriod
                    = WEBRTC_SPL_MAX(inst->curPeakPeriod, inst->peakPeriodSamp[i]);

            }

            inst->peakModeDisabled >>= 1; 

        }
        else if (inst->peakIatCountSamp > (uint32_t) WEBRTC_SPL_MUL_32_16(fsHz,
            WEBRTC_SPL_LSHIFT_W16(MAX_PEAK_PERIOD, 1)))
        {
            



            inst->curPeakHeight = 0;
            inst->curPeakPeriod = 0;
            for (i = 0; i < NUM_PEAKS; i++)
            {
                inst->peakHeightPkt[i] = 0;
                inst->peakPeriodSamp[i] = 0;
            }

            inst->peakIndex = -1; 
            inst->peakIatCountSamp = 0;
        }

        inst->peakIatCountSamp = 0; 
    } 

    

    



    inst->peakFound = 0;
    if ((!inst->peakModeDisabled) && (inst->peakIatCountSamp
        <= WEBRTC_SPL_LSHIFT_W32(inst->curPeakPeriod , 1)))
    {
        
        inst->peakFound = 1;
        
        Bopt = WEBRTC_SPL_MAX(Bopt, inst->curPeakHeight);

#ifdef NETEQ_DELAY_LOGGING
        
        temp_var = (int) -(Bopt * inst->packetSpeechLenSamp);
#endif
    }

    
    Bopt = WEBRTC_SPL_LSHIFT_U16(Bopt,8);

#ifdef NETEQ_DELAY_LOGGING
    
    if (fwrite( &temp_var, sizeof(int), 1, delay_fid2 ) != 1) {
      return -1;
    }
#endif

    
    if (Bopt <= 0)
    {
        Bopt = WEBRTC_SPL_LSHIFT_W16(1, 8); 
    }

    return Bopt; 
}


int WebRtcNetEQ_BufferLevelFilter(int32_t curSizeMs8, AutomodeInst_t *inst,
                                  int sampPerCall, int16_t fsMult)
{

    int16_t curSizeFrames;

    
    
    

    if (sampPerCall <= 0 || fsMult <= 0)
    {
        
        return -1;
    }

    
    if (inst->packetSpeechLenSamp > 0)
    {
        



        curSizeFrames = (int16_t) WebRtcSpl_DivW32W16(
            WEBRTC_SPL_MUL_32_16(curSizeMs8, fsMult), inst->packetSpeechLenSamp);
    }
    else
    {
        curSizeFrames = 0;
    }

    
    if (inst->levelFiltFact > 0) 
    {
        





        inst->buffLevelFilt = ((inst->levelFiltFact * inst->buffLevelFilt) >> 8) +
            (256 - inst->levelFiltFact) * curSizeFrames;
    }

    
    if (inst->prevTimeScale)
    {
        





        inst->buffLevelFilt = WEBRTC_SPL_MAX( inst->buffLevelFilt -
            WebRtcSpl_DivW32W16(
                WEBRTC_SPL_LSHIFT_W32(inst->sampleMemory, 8), 
                inst->packetSpeechLenSamp ), 
            0);

        



        inst->prevTimeScale = 0;
        inst->timescaleHoldOff = AUTOMODE_TIMESCALE_LIMIT;
    }

    
    inst->packetIatCountSamp += sampPerCall; 
    inst->peakIatCountSamp += sampPerCall; 
    inst->timescaleHoldOff >>= 1; 
    inst->maxCSumUpdateTimer += sampPerCall; 

    return 0;

}


int WebRtcNetEQ_SetPacketSpeechLen(AutomodeInst_t *inst, int16_t newLenSamp,
                                   int32_t fsHz)
{

    
    if (newLenSamp <= 0 || fsHz <= 0)
    {
        return -1;
    }

    inst->packetSpeechLenSamp = newLenSamp; 

    
    inst->lastPackCNGorDTMF = 1;

    inst->packetIatCountSamp = 0; 

    




    inst->peakThresholdPkt = (uint16_t) WebRtcSpl_DivW32W16ResW16(
        WEBRTC_SPL_MUL_16_16_RSFT(PEAK_HEIGHT,
            (int16_t) WEBRTC_SPL_RSHIFT_W32(fsHz, 6), 2), inst->packetSpeechLenSamp);

    return 0;
}


int WebRtcNetEQ_ResetAutomode(AutomodeInst_t *inst, int maxBufLenPackets)
{

    int i;
    uint16_t tempprob = 0x4002; 

    
    if (maxBufLenPackets <= 1)
    {
        
        maxBufLenPackets = 10;
    }

    
    inst->buffLevelFilt = 0;

    
    inst->packetSpeechLenSamp = 0;

    



    inst->lastPackCNGorDTMF = 1;

    
    inst->peakModeDisabled = 1; 
    inst->peakIatCountSamp = 0;
    inst->peakIndex = -1; 
    inst->curPeakHeight = 0;
    inst->curPeakPeriod = 0;
    for (i = 0; i < NUM_PEAKS; i++)
    {
        inst->peakHeightPkt[i] = 0;
        inst->peakPeriodSamp[i] = 0;
    }

    




    for (i = 0; i <= MAX_IAT; i++)
    {
        
        tempprob = WEBRTC_SPL_RSHIFT_U16(tempprob, 1);
        
        inst->iatProb[i] = WEBRTC_SPL_LSHIFT_W32((int32_t) tempprob, 16);
    }

    




    inst->optBufLevel = WEBRTC_SPL_MIN(4,
        (maxBufLenPackets >> 1) + (maxBufLenPackets >> 1)); 
    inst->required_delay_q8 = inst->optBufLevel;
    inst->levelFiltFact = 253;

    



    inst->iatProbFact = 0;

    
    inst->packetIatCountSamp = 0;

    
    inst->prevTimeScale = 0;
    inst->timescaleHoldOff = AUTOMODE_TIMESCALE_LIMIT; 

    inst->cSumIatQ8 = 0;
    inst->maxCSumIatQ8 = 0;

    return 0;
}

int32_t WebRtcNetEQ_AverageIAT(const AutomodeInst_t *inst) {
  int i;
  int32_t sum_q24 = 0;
  assert(inst);
  for (i = 0; i <= MAX_IAT; ++i) {
    
    sum_q24 += (inst->iatProb[i] >> 6) * i;
  }
  
  sum_q24 -= (1 << 24);
  



  return ((sum_q24 >> 7) * 15625) >> 11;
}
