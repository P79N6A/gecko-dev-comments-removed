














#include "buffer_stats.h"

#include <assert.h>

#include "signal_processing_library.h"

#include "automode.h"
#include "neteq_defines.h"
#include "neteq_error_codes.h"
#include "webrtc_neteq.h"

#define NETEQ_BUFSTAT_20MS_Q7 2560 /* = 20 ms in Q7  */

uint16_t WebRtcNetEQ_BufstatsDecision(BufstatsInst_t *inst, int16_t frameSize,
                                      int32_t cur_size, uint32_t targetTS,
                                      uint32_t availableTS, int noPacket,
                                      int cngPacket, int prevPlayMode,
                                      enum WebRtcNetEQPlayoutMode playoutMode,
                                      int timestampsPerCall, int NoOfExpandCalls,
                                      int16_t fs_mult,
                                      int16_t lastModeBGNonly, int playDtmf)
{

    int currentDelayMs;
    int32_t currSizeSamples = cur_size;
    int extraDelayPacketsQ8 = 0;

    
    int32_t curr_sizeQ7 = WEBRTC_SPL_LSHIFT_W32(cur_size, 4);
    int level_limit_hi, level_limit_lo;

    inst->Automode_inst.prevTimeScale &= (prevPlayMode == MODE_SUCCESS_ACCELERATE
        || prevPlayMode == MODE_LOWEN_ACCELERATE || prevPlayMode == MODE_SUCCESS_PREEMPTIVE
        || prevPlayMode == MODE_LOWEN_PREEMPTIVE);

    if ((prevPlayMode != MODE_RFC3389CNG) && (prevPlayMode != MODE_CODEC_INTERNAL_CNG))
    {
        



        WebRtcNetEQ_BufferLevelFilter(cur_size, &(inst->Automode_inst), timestampsPerCall,
            fs_mult);
    }
    else
    {
        
        inst->Automode_inst.packetIatCountSamp += timestampsPerCall; 
        inst->Automode_inst.peakIatCountSamp += timestampsPerCall; 
        inst->Automode_inst.timescaleHoldOff >>= 1; 
    }
    cur_size = WEBRTC_SPL_MIN(curr_sizeQ7, WEBRTC_SPL_WORD16_MAX);

    
    
    inst->avgDelayMsQ8 = (int16_t) (WEBRTC_SPL_MUL_16_16_RSFT(inst->avgDelayMsQ8,511,9)
        + (cur_size >> 9));

    
    currentDelayMs = (curr_sizeQ7 >> 7);
    if (currentDelayMs > inst->maxDelayMs)
    {
        inst->maxDelayMs = currentDelayMs;
    }

    
    if (playoutMode == kPlayoutOn || playoutMode == kPlayoutStreaming)
    {
        
        if (prevPlayMode == MODE_ERROR)
        {
            if (noPacket)
            {
                return BUFSTATS_DO_EXPAND;
            }
            else
            {
                return BUFSTAT_REINIT;
            }
        }

        if (prevPlayMode != MODE_EXPAND && prevPlayMode != MODE_FADE_TO_BGN)
        {
            inst->w16_noExpand = 1;
        }
        else
        {
            inst->w16_noExpand = 0;
        }

        if (cngPacket)
        {
            
            int32_t diffTS = (inst->uw32_CNGplayedTS + targetTS) - availableTS;
            int32_t optimal_level_samp = (inst->Automode_inst.optBufLevel *
                inst->Automode_inst.packetSpeechLenSamp) >> 8;
            int32_t excess_waiting_time_samp = -diffTS - optimal_level_samp;

            if (excess_waiting_time_samp > optimal_level_samp / 2)
            {
                



                inst->uw32_CNGplayedTS += excess_waiting_time_samp;
                diffTS += excess_waiting_time_samp;
            }

            if ((diffTS) < 0 && (prevPlayMode == MODE_RFC3389CNG))
            {
                

                return BUFSTATS_DO_RFC3389CNG_NOPACKET;
            }

            
            return BUFSTATS_DO_RFC3389CNG_PACKET;
        }

        
        if (noPacket)
        {
            if (inst->w16_cngOn == CNG_RFC3389_ON)
            {
                
                return BUFSTATS_DO_RFC3389CNG_NOPACKET;
            }
            else if (inst->w16_cngOn == CNG_INTERNAL_ON)
            {
                
                return BUFSTATS_DO_INTERNAL_CNG_NOPACKET;
            }
            else if (playDtmf == 1)
            {
                
                return BUFSTATS_DO_DTMF_ONLY;
            }
            else
            {
                
                return BUFSTATS_DO_EXPAND;
            }
        }

        



        if (NoOfExpandCalls > REINIT_AFTER_EXPANDS) return BUFSTAT_REINIT_DECODER;

        
        if (inst->Automode_inst.extraDelayMs > 0 && inst->Automode_inst.packetSpeechLenSamp
            > 0)
        {

            
            extraDelayPacketsQ8 =
                ((inst->Automode_inst.extraDelayMs * 8 * fs_mult) << 8) /
                inst->Automode_inst.packetSpeechLenSamp;
        }

        
        if (targetTS == availableTS)
        {

            
            if (inst->w16_noExpand == 1 && playDtmf == 0)
            {
                

                level_limit_lo = ((inst->Automode_inst.optBufLevel) >> 1) 
                    + ((inst->Automode_inst.optBufLevel) >> 2); 

                
                level_limit_hi = WEBRTC_SPL_MAX(inst->Automode_inst.optBufLevel,
                    level_limit_lo +
                    WebRtcSpl_DivW32W16ResW16((WEBRTC_SPL_MUL(20*8, fs_mult) << 8),
                        inst->Automode_inst.packetSpeechLenSamp));

                
                if (extraDelayPacketsQ8 > 0)
                {
                    level_limit_hi += extraDelayPacketsQ8;
                    level_limit_lo += extraDelayPacketsQ8;
                }

                if (((inst->Automode_inst.buffLevelFilt >= level_limit_hi) &&
                    (inst->Automode_inst.timescaleHoldOff == 0)) ||
                    (inst->Automode_inst.buffLevelFilt >= level_limit_hi << 2))
                {
                    



                    return BUFSTATS_DO_ACCELERATE;
                }
                else if ((inst->Automode_inst.buffLevelFilt < level_limit_lo)
                    && (inst->Automode_inst.timescaleHoldOff == 0))
                {
                    return BUFSTATS_DO_PREEMPTIVE_EXPAND;
                }
            }
            return BUFSTATS_DO_NORMAL;
        }

        
        else if (availableTS > targetTS)
        {

            
            if ((prevPlayMode == MODE_EXPAND)
                && (availableTS - targetTS
                    < (uint32_t) WEBRTC_SPL_MUL_16_16((int16_t)timestampsPerCall,
                        (int16_t)REINIT_AFTER_EXPANDS))
                && (NoOfExpandCalls < MAX_WAIT_FOR_PACKET)
                && (availableTS
                    > targetTS
                        + WEBRTC_SPL_MUL_16_16((int16_t)timestampsPerCall,
                            (int16_t)NoOfExpandCalls))
                && (inst->Automode_inst.buffLevelFilt <= inst->Automode_inst.optBufLevel
                    + extraDelayPacketsQ8))
            {
                if (playDtmf == 1)
                {
                    
                    return BUFSTATS_DO_DTMF_ONLY;
                }
                else
                {
                    
                    return BUFSTATS_DO_EXPAND;
                }
            }

            
            if ((prevPlayMode == MODE_RFC3389CNG) || (prevPlayMode == MODE_CODEC_INTERNAL_CNG)
                || lastModeBGNonly)
            {
                




                int32_t diffTS = (inst->uw32_CNGplayedTS + targetTS) - availableTS;
                int val = ((inst->Automode_inst.optBufLevel +
                    extraDelayPacketsQ8) *
                    inst->Automode_inst.packetSpeechLenSamp) >> 6;
                if (diffTS >= 0 || val < currSizeSamples)
                {
                    
                    return BUFSTATS_DO_NORMAL;
                }
                else
                {
                    
                    if (prevPlayMode == MODE_RFC3389CNG)
                    {
                        return BUFSTATS_DO_RFC3389CNG_NOPACKET;
                    }
                    else if (prevPlayMode == MODE_CODEC_INTERNAL_CNG)
                    {
                        return BUFSTATS_DO_INTERNAL_CNG_NOPACKET;
                    }
                    else if (playDtmf == 1)
                    {
                        
                        return BUFSTATS_DO_DTMF_ONLY;
                    }
                    else 
                    {
                        
                        return BUFSTATS_DO_EXPAND;
                    }
                }
            }

            
            if ((inst->w16_noExpand == 0) || ((frameSize < timestampsPerCall) && (cur_size
                > NETEQ_BUFSTAT_20MS_Q7)))
            {
                return BUFSTATS_DO_MERGE;
            }
            else if (playDtmf == 1)
            {
                
                return BUFSTATS_DO_DTMF_ONLY;
            }
            else
            {
                return BUFSTATS_DO_EXPAND;
            }
        }
    }
    else
    { 
        if (cngPacket)
        {
            if (((int32_t) ((inst->uw32_CNGplayedTS + targetTS) - availableTS)) >= 0)
            {
                
                return BUFSTATS_DO_RFC3389CNG_PACKET;
            }
            else
            {
                
                return BUFSTATS_DO_RFC3389CNG_NOPACKET;
            }
        }
        if (noPacket)
        {
            




            if (inst->w16_cngOn == CNG_RFC3389_ON)
            {
                
                return BUFSTATS_DO_RFC3389CNG_NOPACKET;
            }
            else if (inst->w16_cngOn == CNG_INTERNAL_ON)
            {
                
                return BUFSTATS_DO_INTERNAL_CNG_NOPACKET;
            }
            else
            {
                
                if (playoutMode == kPlayoutOff)
                {
                    return BUFSTATS_DO_ALTERNATIVE_PLC;
                }
                else if (playoutMode == kPlayoutFax)
                {
                    return BUFSTATS_DO_AUDIO_REPETITION;
                }
                else
                {
                    
                    assert(0);
                    return BUFSTAT_REINIT;
                }
            }
        }
        else if (targetTS == availableTS)
        {
            return BUFSTATS_DO_NORMAL;
        }
        else
        {
            if (((int32_t) ((inst->uw32_CNGplayedTS + targetTS) - availableTS)) >= 0)
            {
                return BUFSTATS_DO_NORMAL;
            }
            else if (playoutMode == kPlayoutOff)
            {
                



                if (inst->w16_cngOn == CNG_RFC3389_ON)
                {
                    return BUFSTATS_DO_RFC3389CNG_NOPACKET;
                }
                else if (inst->w16_cngOn == CNG_INTERNAL_ON)
                {
                    return BUFSTATS_DO_INTERNAL_CNG_NOPACKET;
                }
                else
                {
                    



                    return BUFSTATS_DO_ALTERNATIVE_PLC_INC_TS;
                }
            }
            else if (playoutMode == kPlayoutFax)
            {
                



                if (inst->w16_cngOn == CNG_RFC3389_ON)
                {
                    return BUFSTATS_DO_RFC3389CNG_NOPACKET;
                }
                else if (inst->w16_cngOn == CNG_INTERNAL_ON)
                {
                    return BUFSTATS_DO_INTERNAL_CNG_NOPACKET;
                }
                else
                {
                    



                    return BUFSTATS_DO_AUDIO_REPETITION_INC_TS;
                }
            }
            else
            {
                
                assert(0);
                return BUFSTAT_REINIT;
            }
        }
    }
    
    return BUFSTAT_REINIT;
}

