













#include "mcu.h"

#include <string.h>

#include "signal_processing_library.h"

#include "automode.h"
#include "dtmf_buffer.h"
#include "mcu_dsp_common.h"
#include "neteq_error_codes.h"

#ifdef NETEQ_DELAY_LOGGING
#include "delay_logging.h"
#include <stdio.h>

extern FILE *delay_fid2; 
#endif





static int WebRtcNetEQ_UpdatePackSizeSamples(MCUInst_t* inst, int buffer_pos,
                                             int payload_type,
                                             int pack_size_samples) {
  if (buffer_pos >= 0) {
    int codec_pos;
    codec_pos = WebRtcNetEQ_DbGetCodec(&inst->codec_DB_inst, payload_type);
    if (codec_pos >= 0) {
      codec_pos = inst->codec_DB_inst.position[codec_pos];
      if (codec_pos >= 0) {
        return WebRtcNetEQ_PacketBufferGetPacketSize(
          &inst->PacketBuffer_inst, buffer_pos,
          &inst->codec_DB_inst, codec_pos, pack_size_samples);
      }
    }
  }
  return pack_size_samples;
}




int WebRtcNetEQ_SignalMcu(MCUInst_t *inst)
{

    int i_bufferpos, i_res;
    WebRtc_UWord16 uw16_instr;
    DSP2MCU_info_t dspInfo;
    WebRtc_Word16 *blockPtr, blockLen;
    WebRtc_UWord32 uw32_availableTS;
    RTPPacket_t temp_pkt;
    WebRtc_Word32 w32_bufsize, w32_tmp;
    WebRtc_Word16 payloadType = -1;
    WebRtc_Word16 wantedNoOfTimeStamps;
    WebRtc_Word32 totalTS;
    WebRtc_Word16 oldPT, latePacketExist = 0;
    WebRtc_UWord32 oldTS, prevTS, uw32_tmp;
    WebRtc_UWord16 prevSeqNo;
    WebRtc_Word16 nextSeqNoAvail;
    WebRtc_Word16 fs_mult, w16_tmp;
    WebRtc_Word16 lastModeBGNonly = 0;
#ifdef NETEQ_DELAY_LOGGING
    int temp_var;
#endif
    int playDtmf = 0;

    fs_mult = WebRtcSpl_DivW32W16ResW16(inst->fs, 8000);

    
    inst->lastReportTS += inst->timestampsPerCall;

    
    WebRtcNetEQ_IncrementWaitingTimes(&inst->PacketBuffer_inst);

    

    WEBRTC_SPL_MEMCPY_W8(&dspInfo,inst->pw16_readAddress,sizeof(DSP2MCU_info_t));

    
    blockPtr = &inst->pw16_writeAddress[3];

    
    inst->pw16_writeAddress[0] = 0;
    inst->pw16_writeAddress[1] = 0;
    inst->pw16_writeAddress[2] = 0;

    if ((dspInfo.lastMode & MODE_AWAITING_CODEC_PTR) != 0)
    {
        



        if (inst->new_codec != 1)
        {
            inst->current_Codec = -1;
        }
        dspInfo.lastMode = (dspInfo.lastMode ^ MODE_AWAITING_CODEC_PTR);
    }

#ifdef NETEQ_STEREO
    if ((dspInfo.lastMode & MODE_MASTER_DTMF_SIGNAL) != 0)
    {
        playDtmf = 1; 
        dspInfo.lastMode = (dspInfo.lastMode ^ MODE_MASTER_DTMF_SIGNAL);
    }

    if ((dspInfo.lastMode & MODE_USING_STEREO) != 0)
    {
        if (inst->usingStereo == 0)
        {
            
            WebRtcNetEQ_ResetAutomode(&(inst->BufferStat_inst.Automode_inst),
                inst->PacketBuffer_inst.maxInsertPositions);
        }
        inst->usingStereo = 1;
        dspInfo.lastMode = (dspInfo.lastMode ^ MODE_USING_STEREO);
    }
    else
    {
        inst->usingStereo = 0;
    }
#endif

    
    if ((dspInfo.lastMode & MODE_BGN_ONLY) != 0)
    {
        lastModeBGNonly = 1; 
        dspInfo.lastMode ^= MODE_BGN_ONLY; 
    }

    if ((dspInfo.lastMode == MODE_RFC3389CNG) || (dspInfo.lastMode == MODE_CODEC_INTERNAL_CNG)
        || (dspInfo.lastMode == MODE_EXPAND))
    {
        



        inst->BufferStat_inst.uw32_CNGplayedTS += inst->timestampsPerCall;

        if (dspInfo.lastMode == MODE_RFC3389CNG)
        {
            
            inst->BufferStat_inst.w16_cngOn = CNG_RFC3389_ON;
        }
        else if (dspInfo.lastMode == MODE_CODEC_INTERNAL_CNG)
        {
            
            inst->BufferStat_inst.w16_cngOn = CNG_INTERNAL_ON;
        }

    }

    
    if (dspInfo.frameLen > 0)
    {
        inst->PacketBuffer_inst.packSizeSamples = dspInfo.frameLen;
    }

    
    if (inst->new_codec != 1)
    {
        if (WebRtcNetEQ_DbIsMDCodec((enum WebRtcNetEQDecoder) inst->current_Codec))
        {
            WebRtcNetEQ_PacketBufferFindLowestTimestamp(&inst->PacketBuffer_inst,
                inst->timeStamp, &uw32_availableTS, &i_bufferpos, 1, &payloadType);
            if ((inst->new_codec != 1) && (inst->timeStamp == uw32_availableTS)
                && (inst->timeStamp < dspInfo.playedOutTS) && (i_bufferpos != -1)
                && (WebRtcNetEQ_DbGetPayload(&(inst->codec_DB_inst),
                    (enum WebRtcNetEQDecoder) inst->current_Codec) == payloadType))
            {
                int waitingTime;
                temp_pkt.payload = blockPtr + 1;
                i_res = WebRtcNetEQ_PacketBufferExtract(&inst->PacketBuffer_inst, &temp_pkt,
                    i_bufferpos, &waitingTime);
                if (i_res < 0)
                { 
                    return i_res;
                }
                WebRtcNetEQ_StoreWaitingTime(inst, waitingTime);
                *blockPtr = temp_pkt.payloadLen;
                
                if (temp_pkt.rcuPlCntr > 0)
                {
                    *blockPtr = (*blockPtr) | (DSP_CODEC_RED_FLAG);
                }
                blockPtr += ((temp_pkt.payloadLen + 1) >> 1) + 1;

                



                *blockPtr = 0;
                inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0xf0ff)
                        | DSP_CODEC_ADD_LATE_PKT;
                latePacketExist = 1;
            }
        }
    }

    i_res = WebRtcNetEQ_PacketBufferFindLowestTimestamp(&inst->PacketBuffer_inst,
        dspInfo.playedOutTS, &uw32_availableTS, &i_bufferpos, (inst->new_codec == 0),
        &payloadType);
    if (i_res < 0)
    { 
        return i_res;
    }

    if (inst->BufferStat_inst.w16_cngOn == CNG_RFC3389_ON)
    {
        




        while (i_bufferpos != -1 && WebRtcNetEQ_DbIsCNGPayload(&inst->codec_DB_inst,
            payloadType) && dspInfo.playedOutTS >= uw32_availableTS)
        {

            
            inst->PacketBuffer_inst.payloadType[i_bufferpos] = -1;
            inst->PacketBuffer_inst.payloadLengthBytes[i_bufferpos] = 0;
            inst->PacketBuffer_inst.numPacketsInBuffer--;

            
            WebRtcNetEQ_PacketBufferFindLowestTimestamp(&inst->PacketBuffer_inst,
                dspInfo.playedOutTS, &uw32_availableTS, &i_bufferpos, (inst->new_codec == 0),
                &payloadType);
        }
    }

    
    w32_bufsize = WebRtcNetEQ_PacketBufferGetSize(&inst->PacketBuffer_inst,
        &inst->codec_DB_inst);

    if (dspInfo.lastMode == MODE_SUCCESS_ACCELERATE || dspInfo.lastMode
        == MODE_LOWEN_ACCELERATE || dspInfo.lastMode == MODE_SUCCESS_PREEMPTIVE
        || dspInfo.lastMode == MODE_LOWEN_PREEMPTIVE)
    {
        
        inst->BufferStat_inst.Automode_inst.sampleMemory -= dspInfo.samplesLeft
            + inst->timestampsPerCall;
    }

    
    w32_bufsize = WebRtcSpl_DivW32W16((w32_bufsize + dspInfo.samplesLeft), fs_mult);

#ifdef NETEQ_ATEVENT_DECODE
    
    if (WebRtcNetEQ_DtmfDecode(&inst->DTMF_inst, blockPtr + 1, blockPtr + 2,
        dspInfo.playedOutTS + inst->BufferStat_inst.uw32_CNGplayedTS) > 0)
    {
        playDtmf = 1;

        
        inst->pw16_writeAddress[0] = inst->pw16_writeAddress[0] | DSP_DTMF_PAYLOAD;

        
        blockPtr[0] = 4;
        
        blockPtr += 3;
    }
#endif

    
    inst->PacketBuffer_inst.packSizeSamples =
        WebRtcNetEQ_UpdatePackSizeSamples(inst, i_bufferpos, payloadType,
            inst->PacketBuffer_inst.packSizeSamples);
    
    uw16_instr = WebRtcNetEQ_BufstatsDecision(&inst->BufferStat_inst,
        inst->PacketBuffer_inst.packSizeSamples, w32_bufsize, dspInfo.playedOutTS,
        uw32_availableTS, i_bufferpos == -1,
        WebRtcNetEQ_DbIsCNGPayload(&inst->codec_DB_inst, payloadType), dspInfo.lastMode,
        inst->NetEqPlayoutMode, inst->timestampsPerCall, inst->NoOfExpandCalls, fs_mult,
        lastModeBGNonly, playDtmf);

    
    if (inst->lastReportTS > WEBRTC_SPL_UMUL(inst->fs, MAX_LOSS_REPORT_PERIOD))
    {
        
        WebRtcNetEQ_ResetMcuInCallStats(inst);
    }

    
    if ((dspInfo.samplesLeft >= inst->timestampsPerCall) && (uw16_instr
        != BUFSTATS_DO_ACCELERATE) && (uw16_instr != BUFSTATS_DO_MERGE) && (uw16_instr
            != BUFSTATS_DO_PREEMPTIVE_EXPAND))
    {
        *blockPtr = 0;
        inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff) | DSP_INSTR_NORMAL;
        return 0;
    }

    if (uw16_instr == BUFSTATS_DO_EXPAND)
    {
        inst->NoOfExpandCalls++;
    }
    else
    {
        
        inst->NoOfExpandCalls = 0;
    }

    
    if ((inst->new_codec) || (uw16_instr == BUFSTAT_REINIT))
    {
        CodecFuncInst_t cinst;

        
        blockPtr = &inst->pw16_writeAddress[3];
        
        inst->pw16_writeAddress[0] = 0;

        inst->timeStamp = uw32_availableTS;
        dspInfo.playedOutTS = uw32_availableTS;
        if (inst->current_Codec != -1)
        {
            i_res = WebRtcNetEQ_DbGetPtrs(&inst->codec_DB_inst,
                (enum WebRtcNetEQDecoder) inst->current_Codec, &cinst);
            if (i_res < 0)
            { 
                return i_res;
            }
        }
        else
        {
            
            if (WebRtcNetEQ_DbIsCNGPayload(&inst->codec_DB_inst, payloadType))
            {
                
                WebRtc_UWord16 tempFs;

                tempFs = WebRtcNetEQ_DbGetSampleRate(&inst->codec_DB_inst, payloadType);
                

                if (tempFs > 32000)
                {
                    inst->fs = 32000;
                }
                else if (tempFs > 0)
                {
                    inst->fs = tempFs;
                }
            }
            WebRtcSpl_MemSetW16((WebRtc_Word16*) &cinst, 0,
                                sizeof(CodecFuncInst_t) / sizeof(WebRtc_Word16));
            cinst.codec_fs = inst->fs;
        }
        cinst.timeStamp = inst->timeStamp;
        blockLen = (sizeof(CodecFuncInst_t)) >> (sizeof(WebRtc_Word16) - 1); 
        *blockPtr = blockLen * 2;
        blockPtr++;
        WEBRTC_SPL_MEMCPY_W8(blockPtr,&cinst,sizeof(CodecFuncInst_t));
        blockPtr += blockLen;
        inst->new_codec = 0;

        
        i_res = WebRtcNetEQ_McuSetFs(inst, cinst.codec_fs);
        if (i_res < 0)
        { 
            return i_res;
        }

        
        inst->PacketBuffer_inst.packSizeSamples =
            WebRtcNetEQ_UpdatePackSizeSamples(inst, i_bufferpos, payloadType,
                inst->timestampsPerCall * 3);

        WebRtcNetEQ_ResetAutomode(&(inst->BufferStat_inst.Automode_inst),
                                  inst->PacketBuffer_inst.maxInsertPositions);

#ifdef NETEQ_CNG_CODEC
        
        i_res = WebRtcNetEQ_DbGetPtrs(&inst->codec_DB_inst, kDecoderCNG, &cinst);
        if ((i_res < 0) && (i_res != CODEC_DB_NOT_EXIST1))
        {
            
            
            return i_res;
        }
        else
        {
            
            blockLen = (sizeof(cinst.codec_state)) >> (sizeof(WebRtc_Word16) - 1);
            *blockPtr = blockLen * 2;
            blockPtr++;
            WEBRTC_SPL_MEMCPY_W8(blockPtr,&cinst.codec_state,sizeof(cinst.codec_state));
            blockPtr += blockLen;
        }
#endif

        inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0xf0ff)
                | DSP_CODEC_NEW_CODEC;

        if (uw16_instr == BUFSTATS_DO_RFC3389CNG_NOPACKET)
        {
            



            uw16_instr = BUFSTATS_DO_RFC3389CNG_PACKET;
        }
        else if (uw16_instr != BUFSTATS_DO_RFC3389CNG_PACKET)
        {
            uw16_instr = BUFSTATS_DO_NORMAL;
        }

        
        WebRtcNetEQ_ResetMcuInCallStats(inst);
    }

    
    if (uw16_instr == BUFSTAT_REINIT_DECODER)
    {
        
        uw16_instr = BUFSTATS_DO_NORMAL;
        inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0xf0ff) | DSP_CODEC_RESET;
    }

    
    if (uw16_instr == BUFSTATS_DO_EXPAND)
    {

        inst->timeStamp = dspInfo.playedOutTS;

        
        if (WebRtcNetEQ_DbIsMDCodec((enum WebRtcNetEQDecoder) inst->current_Codec)
            && (dspInfo.MD || latePacketExist))
        {

            if (dspInfo.lastMode != MODE_ONE_DESCRIPTOR)
            {
                
                inst->one_desc = 0;
            }
            if (inst->one_desc < MAX_ONE_DESC)
            {
                
                inst->one_desc++; 
                inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                        | DSP_INSTR_NORMAL_ONE_DESC;

                
                inst->NoOfExpandCalls = WEBRTC_SPL_MAX(inst->NoOfExpandCalls - 1, 0);
                return 0;
            }
            else
            {
                
                inst->one_desc = 0; 
            }

        }

        inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff) | DSP_INSTR_EXPAND;
        return 0;
    }

    
    if ((uw16_instr == BUFSTATS_DO_MERGE) && (dspInfo.MD != 0))
    {
        inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                | DSP_INSTR_NORMAL_ONE_DESC;
        *blockPtr = 0;
        return 0;
    }

    
    if (uw16_instr == BUFSTATS_DO_RFC3389CNG_NOPACKET)
    {
        inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                | DSP_INSTR_DO_RFC3389CNG;
        *blockPtr = 0;
        return 0;
    }

    
    if (uw16_instr == BUFSTATS_DO_INTERNAL_CNG_NOPACKET)
    {
        inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                | DSP_INSTR_DO_CODEC_INTERNAL_CNG;
        *blockPtr = 0;
        return 0;
    }

    
    if (uw16_instr == BUFSTATS_DO_DTMF_ONLY)
    {
        WebRtc_UWord32 timeStampJump = 0;

        
        if ((inst->BufferStat_inst.uw32_CNGplayedTS > 0) && (dspInfo.lastMode != MODE_DTMF))
        {
            
            timeStampJump = inst->BufferStat_inst.uw32_CNGplayedTS;
            inst->pw16_writeAddress[1] = (WebRtc_UWord16) (timeStampJump >> 16);
            inst->pw16_writeAddress[2] = (WebRtc_UWord16) (timeStampJump & 0xFFFF);
        }

        inst->timeStamp = dspInfo.playedOutTS + timeStampJump;

        inst->BufferStat_inst.uw32_CNGplayedTS = 0;
        inst->NoOfExpandCalls = 0;

        inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                | DSP_INSTR_DTMF_GENERATE;
        *blockPtr = 0;
        return 0;
    }

    if (uw16_instr == BUFSTATS_DO_ACCELERATE)
    {
        
        if (dspInfo.samplesLeft >= (3 * 80 * fs_mult))
        {
            
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                    | DSP_INSTR_ACCELERATE;
            *blockPtr = 0;
            inst->BufferStat_inst.Automode_inst.sampleMemory
            = (WebRtc_Word32) dspInfo.samplesLeft;
            inst->BufferStat_inst.Automode_inst.prevTimeScale = 1;
            return 0;
        }
        else if ((dspInfo.samplesLeft >= (1 * 80 * fs_mult))
            && (inst->PacketBuffer_inst.packSizeSamples >= (240 * fs_mult)))
        {
            
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                    | DSP_INSTR_NORMAL;
            *blockPtr = 0;
            return 0;
        }
        else if ((dspInfo.samplesLeft < (1 * 80 * fs_mult))
            && (inst->PacketBuffer_inst.packSizeSamples >= (240 * fs_mult)))
        {
            
            wantedNoOfTimeStamps = inst->timestampsPerCall;
        }
        else if (dspInfo.samplesLeft >= (2 * 80 * fs_mult))
        {
            
            wantedNoOfTimeStamps = inst->timestampsPerCall;
        }
        else
        {
            



            wantedNoOfTimeStamps = 2 * inst->timestampsPerCall;
            uw16_instr = BUFSTATS_DO_NORMAL;
        }
    }
    else if (uw16_instr == BUFSTATS_DO_PREEMPTIVE_EXPAND)
    {
        
        if (dspInfo.samplesLeft >= (3 * 80 * fs_mult))
        {
            
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                    | DSP_INSTR_PREEMPTIVE_EXPAND;
            *blockPtr = 0;
            inst->BufferStat_inst.Automode_inst.sampleMemory
            = (WebRtc_Word32) dspInfo.samplesLeft;
            inst->BufferStat_inst.Automode_inst.prevTimeScale = 1;
            return 0;
        }
        else if ((dspInfo.samplesLeft >= (1 * 80 * fs_mult))
            && (inst->PacketBuffer_inst.packSizeSamples >= (240 * fs_mult)))
        {
            



            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                    | DSP_INSTR_PREEMPTIVE_EXPAND;
            *blockPtr = 0;
            inst->BufferStat_inst.Automode_inst.sampleMemory
            = (WebRtc_Word32) dspInfo.samplesLeft;
            inst->BufferStat_inst.Automode_inst.prevTimeScale = 1;
            return 0;
        }
        else if ((dspInfo.samplesLeft < (1 * 80 * fs_mult))
            && (inst->PacketBuffer_inst.packSizeSamples >= (240 * fs_mult)))
        {
            



            wantedNoOfTimeStamps = inst->timestampsPerCall;
        }
        else if (dspInfo.samplesLeft >= (2 * 80 * fs_mult))
        {
            
            wantedNoOfTimeStamps = inst->timestampsPerCall;
        }
        else
        {
            



            wantedNoOfTimeStamps = 2 * inst->timestampsPerCall;
        }
    }
    else
    {
        wantedNoOfTimeStamps = inst->timestampsPerCall;
    }

    
    totalTS = 0;
    oldTS = uw32_availableTS;
    if ((i_bufferpos > -1) && (uw16_instr != BUFSTATS_DO_ALTERNATIVE_PLC) && (uw16_instr
        != BUFSTATS_DO_ALTERNATIVE_PLC_INC_TS) && (uw16_instr != BUFSTATS_DO_AUDIO_REPETITION)
        && (uw16_instr != BUFSTATS_DO_AUDIO_REPETITION_INC_TS))
    {
        uw32_tmp = (uw32_availableTS - dspInfo.playedOutTS);
        inst->pw16_writeAddress[1] = (WebRtc_UWord16) (uw32_tmp >> 16);
        inst->pw16_writeAddress[2] = (WebRtc_UWord16) (uw32_tmp & 0xFFFF);
        if (inst->BufferStat_inst.w16_cngOn == CNG_OFF)
        {
            





            inst->lostTS += uw32_tmp;
        }

        if (uw16_instr != BUFSTATS_DO_RFC3389CNG_PACKET)
        {
            
            inst->BufferStat_inst.w16_cngOn = CNG_OFF;
        }

        



        inst->BufferStat_inst.uw32_CNGplayedTS = 0;

        prevSeqNo = inst->PacketBuffer_inst.seqNumber[i_bufferpos];
        prevTS = inst->PacketBuffer_inst.timeStamp[i_bufferpos];
        oldPT = inst->PacketBuffer_inst.payloadType[i_bufferpos];

        
        inst->pw16_writeAddress[0] = inst->pw16_writeAddress[0] & 0xFF3F;
        do
        {
            int waitingTime;
            inst->timeStamp = uw32_availableTS;
            
            temp_pkt.payload = blockPtr + 1;
            i_res = WebRtcNetEQ_PacketBufferExtract(&inst->PacketBuffer_inst, &temp_pkt,
                i_bufferpos, &waitingTime);

            if (i_res < 0)
            {
                
                return i_res;
            }
            WebRtcNetEQ_StoreWaitingTime(inst, waitingTime);

#ifdef NETEQ_DELAY_LOGGING
            temp_var = NETEQ_DELAY_LOGGING_SIGNAL_DECODE;
            if ((fwrite(&temp_var, sizeof(int),
                        1, delay_fid2) != 1) ||
                (fwrite(&temp_pkt.timeStamp, sizeof(WebRtc_UWord32),
                        1, delay_fid2) != 1) ||
                (fwrite(&dspInfo.samplesLeft, sizeof(WebRtc_UWord16),
                        1, delay_fid2) != 1)) {
              return -1;
            }
#endif

            *blockPtr = temp_pkt.payloadLen;
            
            if (temp_pkt.rcuPlCntr > 0)
            {
                *blockPtr = (*blockPtr) | (DSP_CODEC_RED_FLAG);
            }
            blockPtr += ((temp_pkt.payloadLen + 1) >> 1) + 1;

            if (i_bufferpos > -1)
            {
                



                totalTS = uw32_availableTS - oldTS + inst->PacketBuffer_inst.packSizeSamples;
            }
            
            WebRtcNetEQ_PacketBufferFindLowestTimestamp(&inst->PacketBuffer_inst,
                inst->timeStamp, &uw32_availableTS, &i_bufferpos, 0, &payloadType);

            nextSeqNoAvail = 0;
            if ((i_bufferpos > -1) && (oldPT
                == inst->PacketBuffer_inst.payloadType[i_bufferpos]))
            {
                w16_tmp = inst->PacketBuffer_inst.seqNumber[i_bufferpos] - prevSeqNo;
                w32_tmp = inst->PacketBuffer_inst.timeStamp[i_bufferpos] - prevTS;
                if ((w16_tmp == 1) || 
                    ((w16_tmp == 0) && (w32_tmp == inst->PacketBuffer_inst.packSizeSamples)))
                { 
                    nextSeqNoAvail = 1;
                }
                prevSeqNo = inst->PacketBuffer_inst.seqNumber[i_bufferpos];
            }
            
            inst->PacketBuffer_inst.packSizeSamples =
                WebRtcNetEQ_UpdatePackSizeSamples(inst, i_bufferpos,
                    payloadType, inst->PacketBuffer_inst.packSizeSamples);
        }
        while ((totalTS < wantedNoOfTimeStamps) && (nextSeqNoAvail == 1));
    }

    if ((uw16_instr == BUFSTATS_DO_ACCELERATE)
        || (uw16_instr == BUFSTATS_DO_PREEMPTIVE_EXPAND))
    {
        
        if ((totalTS + dspInfo.samplesLeft) < WEBRTC_SPL_MUL(3,inst->timestampsPerCall)
            && (uw16_instr == BUFSTATS_DO_ACCELERATE))
        {
            
            uw16_instr = BUFSTATS_DO_NORMAL;
        }
        else
        {
            inst->BufferStat_inst.Automode_inst.sampleMemory
            = (WebRtc_Word32) dspInfo.samplesLeft + totalTS;
            inst->BufferStat_inst.Automode_inst.prevTimeScale = 1;
        }
    }

    
    *blockPtr = 0;

    
    switch (uw16_instr)
    {
        case BUFSTATS_DO_NORMAL:
            
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
            | DSP_INSTR_NORMAL;
            break;
        case BUFSTATS_DO_ACCELERATE:
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
            | DSP_INSTR_ACCELERATE;
            break;
        case BUFSTATS_DO_MERGE:
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
            | DSP_INSTR_MERGE;
            break;
        case BUFSTATS_DO_RFC3389CNG_PACKET:
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
            | DSP_INSTR_DO_RFC3389CNG;
            break;
        case BUFSTATS_DO_ALTERNATIVE_PLC:
            inst->pw16_writeAddress[1] = 0;
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                    | DSP_INSTR_DO_ALTERNATIVE_PLC;
            break;
        case BUFSTATS_DO_ALTERNATIVE_PLC_INC_TS:
            inst->pw16_writeAddress[1] = 0;
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                    | DSP_INSTR_DO_ALTERNATIVE_PLC_INC_TS;
            break;
        case BUFSTATS_DO_AUDIO_REPETITION:
            inst->pw16_writeAddress[1] = 0;
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                    | DSP_INSTR_DO_AUDIO_REPETITION;
            break;
        case BUFSTATS_DO_AUDIO_REPETITION_INC_TS:
            inst->pw16_writeAddress[1] = 0;
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
                    | DSP_INSTR_DO_AUDIO_REPETITION_INC_TS;
            break;
        case BUFSTATS_DO_PREEMPTIVE_EXPAND:
            inst->pw16_writeAddress[0] = (inst->pw16_writeAddress[0] & 0x0fff)
            | DSP_INSTR_PREEMPTIVE_EXPAND;
            break;
        default:
            return UNKNOWN_BUFSTAT_DECISION;
    }

    inst->timeStamp = dspInfo.playedOutTS;
    return 0;

}
