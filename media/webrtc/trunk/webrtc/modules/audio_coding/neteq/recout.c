














#include "dsp.h"

#include <assert.h>
#include <string.h> 

#include "signal_processing_library.h"

#include "dsp_helpfunctions.h"
#include "neteq_error_codes.h"
#include "neteq_defines.h"
#include "mcu_dsp_common.h"


#define TYPE_SPEECH 1
#define TYPE_CNG 2

#ifdef NETEQ_DELAY_LOGGING
#include "delay_logging.h"
#include <stdio.h>
#pragma message("*******************************************************************")
#pragma message("You have specified to use NETEQ_DELAY_LOGGING in the NetEQ library.")
#pragma message("Make sure that your test application supports this.")
#pragma message("*******************************************************************")
#endif
















#define SCRATCH_ALGORITHM_BUFFER            0
#define SCRATCH_NETEQ_NORMAL                0
#define SCRATCH_NETEQ_MERGE                 0

#if (defined(NETEQ_48KHZ_WIDEBAND)) 
#define SCRATCH_DSP_INFO                     6480
#define SCRATCH_NETEQ_ACCELERATE            1440
#define SCRATCH_NETEQ_BGN_UPDATE            2880
#define SCRATCH_NETEQ_EXPAND                756
#elif (defined(NETEQ_32KHZ_WIDEBAND)) 
#define SCRATCH_DSP_INFO                     4320
#define SCRATCH_NETEQ_ACCELERATE            960
#define SCRATCH_NETEQ_BGN_UPDATE            1920
#define SCRATCH_NETEQ_EXPAND                504
#elif (defined(NETEQ_WIDEBAND)) 
#define SCRATCH_DSP_INFO                     2160
#define SCRATCH_NETEQ_ACCELERATE            480
#define SCRATCH_NETEQ_BGN_UPDATE            960
#define SCRATCH_NETEQ_EXPAND                252
#else    
#define SCRATCH_DSP_INFO                     1080
#define SCRATCH_NETEQ_ACCELERATE            240
#define SCRATCH_NETEQ_BGN_UPDATE            480
#define SCRATCH_NETEQ_EXPAND                126
#endif

#if (defined(NETEQ_48KHZ_WIDEBAND)) 
#define SIZE_SCRATCH_BUFFER                 6516
#elif (defined(NETEQ_32KHZ_WIDEBAND)) 
#define SIZE_SCRATCH_BUFFER                 4344
#elif (defined(NETEQ_WIDEBAND)) 
#define SIZE_SCRATCH_BUFFER                 2172
#else    
#define SIZE_SCRATCH_BUFFER                 1086
#endif

#ifdef NETEQ_DELAY_LOGGING
extern FILE *delay_fid2; 
extern WebRtc_UWord32 tot_received_packets;
#endif


int WebRtcNetEQ_RecOutInternal(DSPInst_t *inst, WebRtc_Word16 *pw16_outData,
                               WebRtc_Word16 *pw16_len, WebRtc_Word16 BGNonly)
{

    WebRtc_Word16 blockLen, payloadLen, len = 0, pos;
    WebRtc_Word16 w16_tmp1, w16_tmp2, w16_tmp3, DataEnough;
    WebRtc_Word16 *blockPtr;
    WebRtc_Word16 MD = 0;

    WebRtc_Word16 speechType = TYPE_SPEECH;
    WebRtc_UWord16 instr;
    WebRtc_UWord16 uw16_tmp;
#ifdef SCRATCH
    char pw8_ScratchBuffer[((SIZE_SCRATCH_BUFFER + 1) * 2)];
    WebRtc_Word16 *pw16_scratchPtr = (WebRtc_Word16*) pw8_ScratchBuffer;
    
    WebRtc_Word16 pw16_decoded_buffer[NETEQ_MAX_FRAME_SIZE+240*6];
    WebRtc_Word16 *pw16_NetEqAlgorithm_buffer = pw16_scratchPtr
        + SCRATCH_ALGORITHM_BUFFER;
    DSP2MCU_info_t *dspInfo = (DSP2MCU_info_t*) (pw16_scratchPtr + SCRATCH_DSP_INFO);
#else
    
    WebRtc_Word16 pw16_decoded_buffer[NETEQ_MAX_FRAME_SIZE+240*6];
    WebRtc_Word16 pw16_NetEqAlgorithm_buffer[NETEQ_MAX_OUTPUT_SIZE+240*6];
    DSP2MCU_info_t dspInfoStruct;
    DSP2MCU_info_t *dspInfo = &dspInfoStruct;
#endif
    WebRtc_Word16 fs_mult;
    int borrowedSamples;
    int oldBorrowedSamples;
    int return_value = 0;
    WebRtc_Word16 lastModeBGNonly = (inst->w16_mode & MODE_BGN_ONLY) != 0; 
    void *mainInstBackup = inst->main_inst;

#ifdef NETEQ_DELAY_LOGGING
    int temp_var;
#endif
    WebRtc_Word16 dtmfValue = -1;
    WebRtc_Word16 dtmfVolume = -1;
    int playDtmf = 0;
#ifdef NETEQ_ATEVENT_DECODE
    int dtmfSwitch = 0;
#endif
#ifdef NETEQ_STEREO
    MasterSlaveInfo *msInfo = inst->msInfo;
#endif
    WebRtc_Word16 *sharedMem = pw16_NetEqAlgorithm_buffer; 
    inst->pw16_readAddress = sharedMem;
    inst->pw16_writeAddress = sharedMem;

    
    if (inst->codec_ptr_inst.funcGetMDinfo != NULL)
    {
        MD = inst->codec_ptr_inst.funcGetMDinfo(inst->codec_ptr_inst.codec_state);
        if (MD > 0)
            MD = 1;
        else
            MD = 0;
    }

#ifdef NETEQ_STEREO
    if ((msInfo->msMode == NETEQ_SLAVE) && (inst->codec_ptr_inst.funcDecode != NULL))
    {
        




        
        WebRtc_UWord32 currentMasterTimestamp;
        WebRtc_UWord32 currentSlaveTimestamp;

        currentMasterTimestamp = msInfo->endTimestamp - msInfo->samplesLeftWithOverlap;
        currentSlaveTimestamp = inst->endTimestamp - (inst->endPosition - inst->curPosition);

        



        if (currentSlaveTimestamp < 0x40000000 &&
            currentMasterTimestamp > 0xc0000000) {
          
          currentSlaveTimestamp += (0xffffffff - currentMasterTimestamp) + 1;
          currentMasterTimestamp = 0;
        } else if (currentMasterTimestamp < 0x40000000 &&
            currentSlaveTimestamp > 0xc0000000) {
          
          currentMasterTimestamp += (0xffffffff - currentSlaveTimestamp) + 1;
          currentSlaveTimestamp = 0;
        }

        if (currentSlaveTimestamp < currentMasterTimestamp)
        {
            
            inst->curPosition += currentMasterTimestamp - currentSlaveTimestamp;

        }
        else if (currentSlaveTimestamp > currentMasterTimestamp)
        {
            
            inst->curPosition -= currentSlaveTimestamp - currentMasterTimestamp;
        }

        
        inst->curPosition = WEBRTC_SPL_MIN(inst->curPosition,
            inst->endPosition - inst->ExpandInst.w16_overlap);

        
        inst->curPosition = WEBRTC_SPL_MAX(inst->curPosition, 0);
    }
#endif

    
    dspInfo->playedOutTS = inst->endTimestamp;
    dspInfo->samplesLeft = inst->endPosition - inst->curPosition
        - inst->ExpandInst.w16_overlap;
    dspInfo->MD = MD;
    dspInfo->lastMode = inst->w16_mode;
    dspInfo->frameLen = inst->w16_frameLen;

    
    if (inst->codec_ptr_inst.funcDecode == NULL)
    {
        dspInfo->lastMode |= MODE_AWAITING_CODEC_PTR;
    }

#ifdef NETEQ_STEREO
    if (msInfo->msMode == NETEQ_SLAVE && (msInfo->extraInfo == DTMF_OVERDUB
        || msInfo->extraInfo == DTMF_ONLY))
    {
        
        dspInfo->lastMode |= MODE_MASTER_DTMF_SIGNAL;
    }

    if (msInfo->msMode != NETEQ_MONO)
    {
        
        dspInfo->lastMode |= MODE_USING_STEREO;
    }
#endif

    WEBRTC_SPL_MEMCPY_W8(inst->pw16_writeAddress,dspInfo,sizeof(DSP2MCU_info_t));

    
#ifdef NETEQ_STEREO
    assert(msInfo != NULL);
    if (msInfo->msMode == NETEQ_MASTER)
    {
        
        WebRtcSpl_MemSetW16((WebRtc_Word16 *) msInfo, 0,
            sizeof(MasterSlaveInfo) / sizeof(WebRtc_Word16));
        
        msInfo->msMode = NETEQ_MASTER;

        
        msInfo->endTimestamp = inst->endTimestamp;
        msInfo->samplesLeftWithOverlap = inst->endPosition - inst->curPosition;
    }
#endif

    




    return_value = WebRtcNetEQ_DSP2MCUinterrupt((MainInst_t *) inst->main_inst, sharedMem);

    
    instr = (WebRtc_UWord16) (inst->pw16_readAddress[0] & 0xf000);

#ifdef NETEQ_STEREO
    if (msInfo->msMode == NETEQ_MASTER)
    {
        msInfo->instruction = instr;
    }
    else if (msInfo->msMode == NETEQ_SLAVE)
    {
        
    }
#endif

    
    if (return_value < 0)
    {
        inst->w16_mode = MODE_ERROR;
        dspInfo->lastMode = MODE_ERROR;
        return return_value;
    }

    blockPtr = &((inst->pw16_readAddress)[3]);

    
    if ((inst->pw16_readAddress[0] & DSP_DTMF_PAYLOAD) != 0)
    {
        playDtmf = 1;
        dtmfValue = blockPtr[1];
        dtmfVolume = blockPtr[2];
        blockPtr += 3;

#ifdef NETEQ_STEREO
        if (msInfo->msMode == NETEQ_MASTER)
        {
            
            msInfo->extraInfo = DTMF_OVERDUB;
        }
#endif
    }

    blockLen = (((*blockPtr) & DSP_CODEC_MASK_RED_FLAG) + 1) >> 1; 
    payloadLen = ((*blockPtr) & DSP_CODEC_MASK_RED_FLAG);
    blockPtr++;

    
    if ((inst->pw16_readAddress[0] & 0x0f00) == DSP_CODEC_NEW_CODEC)
    {
        WEBRTC_SPL_MEMCPY_W16(&inst->codec_ptr_inst,blockPtr,(payloadLen+1)>>1);
        if (inst->codec_ptr_inst.codec_fs != 0)
        {
            return_value = WebRtcNetEQ_DSPInit(inst, inst->codec_ptr_inst.codec_fs);
            if (return_value != 0)
            { 
                instr = DSP_INSTR_FADE_TO_BGN; 
            }
#ifdef NETEQ_DELAY_LOGGING
            temp_var = NETEQ_DELAY_LOGGING_SIGNAL_CHANGE_FS;
            if ((fwrite(&temp_var, sizeof(int),
                        1, delay_fid2) != 1) ||
                (fwrite(&inst->fs, sizeof(WebRtc_UWord16),
                        1, delay_fid2) != 1)) {
              return -1;
            }
#endif
        }

        

        WEBRTC_SPL_MEMCPY_W16(&inst->codec_ptr_inst,blockPtr,(payloadLen+1)>>1);
        inst->endTimestamp = inst->codec_ptr_inst.timeStamp;
        inst->videoSyncTimestamp = inst->codec_ptr_inst.timeStamp;
        blockPtr += blockLen;
        blockLen = (((*blockPtr) & DSP_CODEC_MASK_RED_FLAG) + 1) >> 1;
        payloadLen = ((*blockPtr) & DSP_CODEC_MASK_RED_FLAG);
        blockPtr++;
        if (inst->codec_ptr_inst.funcDecodeInit != NULL)
        {
            inst->codec_ptr_inst.funcDecodeInit(inst->codec_ptr_inst.codec_state);
        }

#ifdef NETEQ_CNG_CODEC

        

        WEBRTC_SPL_MEMCPY_W16(&inst->CNG_Codec_inst,blockPtr,(payloadLen+1)>>1);
        blockPtr += blockLen;
        blockLen = (((*blockPtr) & DSP_CODEC_MASK_RED_FLAG) + 1) >> 1;
        payloadLen = ((*blockPtr) & DSP_CODEC_MASK_RED_FLAG);
        blockPtr++;
        if (inst->CNG_Codec_inst != NULL)
        {
            WebRtcCng_InitDec(inst->CNG_Codec_inst);
        }
#endif
    }
    else if ((inst->pw16_readAddress[0] & 0x0f00) == DSP_CODEC_RESET)
    {
        
        if (inst->codec_ptr_inst.funcDecodeInit != NULL)
        {
            inst->codec_ptr_inst.funcDecodeInit(inst->codec_ptr_inst.codec_state);
        }

#ifdef NETEQ_CNG_CODEC
        
        if (inst->CNG_Codec_inst != NULL)
        {
            WebRtcCng_InitDec(inst->CNG_Codec_inst);
        }
#endif 
    }

    fs_mult = WebRtcNetEQ_CalcFsMult(inst->fs);

    
    if ((inst->pw16_readAddress[0] & 0x0f00) == DSP_CODEC_ADD_LATE_PKT)
    {
        if (inst->codec_ptr_inst.funcAddLatePkt != NULL)
        {
            
            inst->codec_ptr_inst.funcAddLatePkt(inst->codec_ptr_inst.codec_state, blockPtr,
                payloadLen);
        }
        blockPtr += blockLen;
        blockLen = (((*blockPtr) & DSP_CODEC_MASK_RED_FLAG) + 1) >> 1; 
        payloadLen = ((*blockPtr) & DSP_CODEC_MASK_RED_FLAG);
        blockPtr++;
    }

    
    if ((instr == DSP_INSTR_NORMAL) || (instr == DSP_INSTR_ACCELERATE) || (instr
        == DSP_INSTR_MERGE) || (instr == DSP_INSTR_PREEMPTIVE_EXPAND))
    {
        
        if ((instr == DSP_INSTR_MERGE) && (inst->codec_ptr_inst.funcDecodePLC != NULL))
        {
            len = 0;
            len = inst->codec_ptr_inst.funcDecodePLC(inst->codec_ptr_inst.codec_state,
                &pw16_decoded_buffer[len], 1);
        }
        len = 0;

        
        while ((blockLen > 0) && (len < (240 * fs_mult))) 
        {
            if (inst->codec_ptr_inst.funcDecode != NULL)
            {
                WebRtc_Word16 dec_Len;
                if (!BGNonly)
                {
                    





                    if (((*(blockPtr - 1) & DSP_CODEC_RED_FLAG) != 0)
                        && (inst->codec_ptr_inst.funcDecodeRCU != NULL))
                    {
                        dec_Len = inst->codec_ptr_inst.funcDecodeRCU(
                            inst->codec_ptr_inst.codec_state, blockPtr, payloadLen,
                            &pw16_decoded_buffer[len], &speechType);
                    }
                    else
                    {
                        dec_Len = inst->codec_ptr_inst.funcDecode(
                            inst->codec_ptr_inst.codec_state, blockPtr, payloadLen,
                            &pw16_decoded_buffer[len], &speechType);
                    }
                }
                else
                {
                    




                    dec_Len = inst->w16_frameLen;
                }

                if (dec_Len > 0)
                {
                    len += dec_Len;
                    
                    inst->w16_frameLen = dec_Len;
                }
                else if (dec_Len < 0)
                {
                    
                    len = -1;
                    break;
                }
                



                if (len > NETEQ_MAX_FRAME_SIZE)
                {
                    WebRtcSpl_MemSetW16(pw16_outData, 0, inst->timestampsPerCall);
                    *pw16_len = inst->timestampsPerCall;
                    inst->w16_mode = MODE_ERROR;
                    dspInfo->lastMode = MODE_ERROR;
                    return RECOUT_ERROR_DECODED_TOO_MUCH;
                }

                
                if (mainInstBackup != inst->main_inst)
                {
                    
                    return CORRUPT_INSTANCE;
                }

            }
            blockPtr += blockLen;
            blockLen = (((*blockPtr) & DSP_CODEC_MASK_RED_FLAG) + 1) >> 1; 
            payloadLen = ((*blockPtr) & DSP_CODEC_MASK_RED_FLAG);
            blockPtr++;
        }

        if (len < 0)
        {
            len = 0;
            inst->endTimestamp += inst->w16_frameLen; 
            if (inst->codec_ptr_inst.funcGetErrorCode != NULL)
            {
                return_value = -inst->codec_ptr_inst.funcGetErrorCode(
                    inst->codec_ptr_inst.codec_state);
            }
            else
            {
                return_value = RECOUT_ERROR_DECODING;
            }
            instr = DSP_INSTR_FADE_TO_BGN;
        }
        if (speechType != TYPE_CNG)
        {
            



            inst->endTimestamp += len;
        }
    }
    else if (instr == DSP_INSTR_NORMAL_ONE_DESC)
    {
        if (inst->codec_ptr_inst.funcDecode != NULL)
        {
            len = inst->codec_ptr_inst.funcDecode(inst->codec_ptr_inst.codec_state, NULL, 0,
                pw16_decoded_buffer, &speechType);
#ifdef NETEQ_DELAY_LOGGING
            temp_var = NETEQ_DELAY_LOGGING_SIGNAL_DECODE_ONE_DESC;
            if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
              return -1;
            }
            if (fwrite(&inst->endTimestamp, sizeof(WebRtc_UWord32),
                       1, delay_fid2) != 1) {
              return -1;
            }
            if (fwrite(&dspInfo->samplesLeft, sizeof(WebRtc_UWord16),
                       1, delay_fid2) != 1) {
              return -1;
            }
            tot_received_packets++;
#endif
        }
        if (speechType != TYPE_CNG)
        {
            



            inst->endTimestamp += len;
        }

        
        if (mainInstBackup != inst->main_inst)
        {
            
            return CORRUPT_INSTANCE;
        }

        if (len <= 0)
        {
            len = 0;
            if (inst->codec_ptr_inst.funcGetErrorCode != NULL)
            {
                return_value = -inst->codec_ptr_inst.funcGetErrorCode(
                    inst->codec_ptr_inst.codec_state);
            }
            else
            {
                return_value = RECOUT_ERROR_DECODING;
            }
            if ((inst->codec_ptr_inst.funcDecodeInit != NULL)
                && (inst->codec_ptr_inst.codec_state != NULL))
            {
                
                inst->codec_ptr_inst.funcDecodeInit(inst->codec_ptr_inst.codec_state);
            }
            inst->endTimestamp += inst->w16_frameLen; 
            instr = DSP_INSTR_FADE_TO_BGN;
        }
    }

    if (len == 0 && lastModeBGNonly) 
    {
        BGNonly = 1; 
    }

#ifdef NETEQ_VAD
    if ((speechType == TYPE_CNG) 
    || ((instr == DSP_INSTR_DO_RFC3389CNG) && (blockLen > 0)) 
    || (inst->fs > 16000)) 
    {
        
        inst->VADInst.VADEnabled = 0;
        inst->VADInst.VADDecision = 1; 
        inst->VADInst.SIDintervalCounter = 0; 
    }
    else if (!inst->VADInst.VADEnabled) 
    {
        inst->VADInst.SIDintervalCounter++; 
    }

    
    if (inst->VADInst.SIDintervalCounter >= POST_DECODE_VAD_AUTO_ENABLE)
    {
        




        WebRtcNetEQ_InitVAD(&inst->VADInst, inst->fs);
    }

    if (len > 0 
    && inst->VADInst.VADEnabled 
    && inst->fs <= 16000) 
    {
        int VADframeSize; 
        int VADSamplePtr = 0;

        inst->VADInst.VADDecision = 0;

        if (inst->VADInst.VADFunction != NULL) 
        {
            
            for (VADframeSize = 30; VADframeSize >= 10; VADframeSize -= 10)
            {
                

                while (inst->VADInst.VADDecision == 0
                    && len - VADSamplePtr >= VADframeSize * fs_mult * 8)
                {
                    




                    
                    inst->VADInst.VADDecision |= inst->VADInst.VADFunction(
                        inst->VADInst.VADState, (int) inst->fs,
                        (WebRtc_Word16 *) &pw16_decoded_buffer[VADSamplePtr],
                        (VADframeSize * fs_mult * 8));

                    VADSamplePtr += VADframeSize * fs_mult * 8; 
                }
            }
        }
        else
        { 
            inst->VADInst.VADDecision = 1; 
            inst->VADInst.VADEnabled = 0; 
        }

    }
#endif 

    
    uw16_tmp = (WebRtc_UWord16) inst->pw16_readAddress[1];
    inst->endTimestamp += (((WebRtc_UWord32) uw16_tmp) << 16);
    uw16_tmp = (WebRtc_UWord16) inst->pw16_readAddress[2];
    inst->endTimestamp += uw16_tmp;

    if (BGNonly && len > 0)
    {
        




        WebRtcNetEQ_GenerateBGN(inst,
#ifdef SCRATCH
            pw16_scratchPtr + SCRATCH_NETEQ_EXPAND,
#endif
            pw16_decoded_buffer, len);
    }

    
    switch (instr)
    {
        case DSP_INSTR_NORMAL:

            
            WebRtcNetEQ_Normal(inst,
#ifdef SCRATCH
                pw16_scratchPtr + SCRATCH_NETEQ_NORMAL,
#endif
                pw16_decoded_buffer, len, pw16_NetEqAlgorithm_buffer, &len);

            
            if ((speechType == TYPE_CNG) || ((inst->w16_mode == MODE_CODEC_INTERNAL_CNG)
                && (len == 0)))
            {
                inst->w16_mode = MODE_CODEC_INTERNAL_CNG;
            }

#ifdef NETEQ_ATEVENT_DECODE
            if (playDtmf == 0)
            {
                inst->DTMFInst.reinit = 1;
            }
#endif
            break;
        case DSP_INSTR_NORMAL_ONE_DESC:

            
            WebRtcNetEQ_Normal(inst,
#ifdef SCRATCH
                pw16_scratchPtr + SCRATCH_NETEQ_NORMAL,
#endif
                pw16_decoded_buffer, len, pw16_NetEqAlgorithm_buffer, &len);
#ifdef NETEQ_ATEVENT_DECODE
            if (playDtmf == 0)
            {
                inst->DTMFInst.reinit = 1;
            }
#endif
            inst->w16_mode = MODE_ONE_DESCRIPTOR;
            break;
        case DSP_INSTR_MERGE:
#ifdef NETEQ_DELAY_LOGGING
            temp_var = NETEQ_DELAY_LOGGING_SIGNAL_MERGE_INFO;
            if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
              return -1;
            }
            temp_var = -len;
#endif
            
            return_value = WebRtcNetEQ_Merge(inst,
#ifdef SCRATCH
                pw16_scratchPtr + SCRATCH_NETEQ_MERGE,
#endif
                pw16_decoded_buffer, len, pw16_NetEqAlgorithm_buffer, &len);

            if (return_value < 0)
            {
                
                return return_value;
            }

#ifdef NETEQ_DELAY_LOGGING
            temp_var += len;
            if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
              return -1;
            }
#endif
            
            if (speechType == TYPE_CNG) inst->w16_mode = MODE_CODEC_INTERNAL_CNG;
#ifdef NETEQ_ATEVENT_DECODE
            if (playDtmf == 0)
            {
                inst->DTMFInst.reinit = 1;
            }
#endif
            break;

        case DSP_INSTR_EXPAND:
            len = 0;
            pos = 0;
            while ((inst->endPosition - inst->curPosition - inst->ExpandInst.w16_overlap + pos)
                < (inst->timestampsPerCall))
            {
                return_value = WebRtcNetEQ_Expand(inst,
#ifdef SCRATCH
                    pw16_scratchPtr + SCRATCH_NETEQ_EXPAND,
#endif
                    pw16_NetEqAlgorithm_buffer, &len, BGNonly);
                if (return_value < 0)
                {
                    
                    return return_value;
                }

                




                WEBRTC_SPL_MEMMOVE_W16(inst->pw16_speechHistory,
                                       inst->pw16_speechHistory + len,
                                       (inst->w16_speechHistoryLen-len));
                WEBRTC_SPL_MEMCPY_W16(&inst->pw16_speechHistory[inst->w16_speechHistoryLen-len],
                                      pw16_NetEqAlgorithm_buffer, len);

                inst->curPosition -= len;

                
                inst->w16_concealedTS += len;
#ifdef NETEQ_DELAY_LOGGING
                temp_var = NETEQ_DELAY_LOGGING_SIGNAL_EXPAND_INFO;
                if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
                  return -1;
                }
                temp_var = len;
                if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
                  return -1;
                }
#endif
                len = 0; 
            }
#ifdef NETEQ_ATEVENT_DECODE
            if (playDtmf == 0)
            {
                inst->DTMFInst.reinit = 1;
            }
#endif
            break;

        case DSP_INSTR_ACCELERATE:
            if (len < 3 * 80 * fs_mult)
            {
                
                borrowedSamples = 3 * 80 * fs_mult - len;

                WEBRTC_SPL_MEMMOVE_W16(&pw16_decoded_buffer[borrowedSamples],
                                       pw16_decoded_buffer, len);
                WEBRTC_SPL_MEMCPY_W16(pw16_decoded_buffer,
                                      &(inst->speechBuffer[inst->endPosition-borrowedSamples]),
                                      borrowedSamples);

                return_value = WebRtcNetEQ_Accelerate(inst,
#ifdef SCRATCH
                    pw16_scratchPtr + SCRATCH_NETEQ_ACCELERATE,
#endif
                    pw16_decoded_buffer, 3 * inst->timestampsPerCall,
                    pw16_NetEqAlgorithm_buffer, &len, BGNonly);

                if (return_value < 0)
                {
                    
                    return return_value;
                }

                
                if (len < borrowedSamples)
                {
                    




                    WEBRTC_SPL_MEMCPY_W16(&inst->speechBuffer[inst->endPosition-borrowedSamples],
                        pw16_NetEqAlgorithm_buffer, len);
                    WEBRTC_SPL_MEMMOVE_W16(&inst->speechBuffer[borrowedSamples-len],
                                           inst->speechBuffer,
                                           (inst->endPosition-(borrowedSamples-len)));

                    inst->curPosition += (borrowedSamples - len);
#ifdef NETEQ_DELAY_LOGGING
                    temp_var = NETEQ_DELAY_LOGGING_SIGNAL_ACCELERATE_INFO;
                    if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
                      return -1;
                    }
                    temp_var = 3 * inst->timestampsPerCall - len;
                    if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
                      return -1;
                    }
#endif
                    len = 0;
                }
                else
                {
                    WEBRTC_SPL_MEMCPY_W16(&inst->speechBuffer[inst->endPosition-borrowedSamples],
                        pw16_NetEqAlgorithm_buffer, borrowedSamples);
                    WEBRTC_SPL_MEMMOVE_W16(pw16_NetEqAlgorithm_buffer,
                                           &pw16_NetEqAlgorithm_buffer[borrowedSamples],
                                           (len-borrowedSamples));
#ifdef NETEQ_DELAY_LOGGING
                    temp_var = NETEQ_DELAY_LOGGING_SIGNAL_ACCELERATE_INFO;
                    if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
                      return -1;
                    }
                    temp_var = 3 * inst->timestampsPerCall - len;
                    if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
                      return -1;
                    }
#endif
                    len = len - borrowedSamples;
                }

            }
            else
            {
#ifdef NETEQ_DELAY_LOGGING
                temp_var = NETEQ_DELAY_LOGGING_SIGNAL_ACCELERATE_INFO;
                if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
                  return -1;
                }
                temp_var = len;
#endif
                return_value = WebRtcNetEQ_Accelerate(inst,
#ifdef SCRATCH
                    pw16_scratchPtr + SCRATCH_NETEQ_ACCELERATE,
#endif
                    pw16_decoded_buffer, len, pw16_NetEqAlgorithm_buffer, &len, BGNonly);

                if (return_value < 0)
                {
                    
                    return return_value;
                }

#ifdef NETEQ_DELAY_LOGGING
                temp_var -= len;
                if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
                  return -1;
                }
#endif
            }
            
            if (speechType == TYPE_CNG) inst->w16_mode = MODE_CODEC_INTERNAL_CNG;
#ifdef NETEQ_ATEVENT_DECODE
            if (playDtmf == 0)
            {
                inst->DTMFInst.reinit = 1;
            }
#endif
            break;

        case DSP_INSTR_DO_RFC3389CNG:
#ifdef NETEQ_CNG_CODEC
            if (blockLen > 0)
            {
                if (WebRtcCng_UpdateSid(inst->CNG_Codec_inst, (WebRtc_UWord8*) blockPtr,
                    payloadLen) < 0)
                {
                    
                    return_value = -WebRtcCng_GetErrorCodeDec(inst->CNG_Codec_inst);
                    len = inst->timestampsPerCall;
                    WebRtcSpl_MemSetW16(pw16_NetEqAlgorithm_buffer, 0, len);
                    break;
                }
            }

            if (BGNonly)
            {
                
                len = WebRtcNetEQ_GenerateBGN(inst,
#ifdef SCRATCH
                    pw16_scratchPtr + SCRATCH_NETEQ_EXPAND,
#endif
                    pw16_NetEqAlgorithm_buffer, inst->timestampsPerCall);
                if (len != inst->timestampsPerCall)
                {
                    
                    return_value = -1;
                }
            }
            else
            {
                return_value = WebRtcNetEQ_Cng(inst, pw16_NetEqAlgorithm_buffer,
                    inst->timestampsPerCall);
            }
            len = inst->timestampsPerCall;
            inst->ExpandInst.w16_consecExp = 0;
            inst->w16_mode = MODE_RFC3389CNG;
#ifdef NETEQ_ATEVENT_DECODE
            if (playDtmf == 0)
            {
                inst->DTMFInst.reinit = 1;
            }
#endif

            if (return_value < 0)
            {
                
                WebRtcSpl_MemSetW16(pw16_NetEqAlgorithm_buffer, 0, len);
            }

            break;
#else
            return FAULTY_INSTRUCTION;
#endif
        case DSP_INSTR_DO_CODEC_INTERNAL_CNG:
            



            len = 0;
            if (inst->codec_ptr_inst.funcDecode != NULL && !BGNonly)
            {
                len = inst->codec_ptr_inst.funcDecode(inst->codec_ptr_inst.codec_state,
                    blockPtr, 0, pw16_decoded_buffer, &speechType);
            }
            else
            {
                
                len = WebRtcNetEQ_GenerateBGN(inst,
#ifdef SCRATCH
                    pw16_scratchPtr + SCRATCH_NETEQ_EXPAND,
#endif
                    pw16_decoded_buffer, inst->timestampsPerCall);
            }
            WebRtcNetEQ_Normal(inst,
#ifdef SCRATCH
                pw16_scratchPtr + SCRATCH_NETEQ_NORMAL,
#endif
                pw16_decoded_buffer, len, pw16_NetEqAlgorithm_buffer, &len);
            inst->w16_mode = MODE_CODEC_INTERNAL_CNG;
            inst->ExpandInst.w16_consecExp = 0;
            break;

        case DSP_INSTR_DTMF_GENERATE:
#ifdef NETEQ_ATEVENT_DECODE
            dtmfSwitch = 0;
            if ((inst->w16_mode != MODE_DTMF) && (inst->DTMFInst.reinit == 0))
            {
                



                dtmfSwitch = 1;
            }

            len = WebRtcNetEQ_DTMFGenerate(&inst->DTMFInst, dtmfValue, dtmfVolume,
                pw16_NetEqAlgorithm_buffer, inst->fs, -1);
            if (len < 0)
            {
                
                return_value = len;
                len = inst->timestampsPerCall;
                WebRtcSpl_MemSetW16(pw16_NetEqAlgorithm_buffer, 0, len);
            }

            if (dtmfSwitch == 1)
            {
                







                



                WebRtc_Word16 tempLen;

                tempLen = WebRtcNetEQ_DTMFGenerate(&inst->DTMFInst, dtmfValue, dtmfVolume,
                    &pw16_NetEqAlgorithm_buffer[len], inst->fs,
                    inst->endPosition - inst->curPosition);
                if (tempLen < 0)
                {
                    
                    return_value = tempLen;
                    len = inst->endPosition - inst->curPosition;
                    WebRtcSpl_MemSetW16(pw16_NetEqAlgorithm_buffer, 0,
                        inst->endPosition - inst->curPosition);
                }

                
                len += tempLen;

                

                WEBRTC_SPL_MEMCPY_W16(&inst->speechBuffer[inst->curPosition],
                                      pw16_NetEqAlgorithm_buffer,
                                      inst->endPosition - inst->curPosition);

                
                len -= (inst->endPosition - inst->curPosition);
                WEBRTC_SPL_MEMMOVE_W16(pw16_NetEqAlgorithm_buffer,
                    &pw16_NetEqAlgorithm_buffer[inst->endPosition - inst->curPosition],
                    len);
            }

            inst->endTimestamp += inst->timestampsPerCall;
            inst->DTMFInst.reinit = 0;
            inst->ExpandInst.w16_consecExp = 0;
            inst->w16_mode = MODE_DTMF;
            BGNonly = 0; 

            playDtmf = 0; 
            




#ifdef NETEQ_STEREO
            if (msInfo->msMode == NETEQ_MASTER)
            {
                
                msInfo->extraInfo = DTMF_ONLY;
            }
#endif

            break;
#else
            inst->w16_mode = MODE_ERROR;
            dspInfo->lastMode = MODE_ERROR;
            return FAULTY_INSTRUCTION;
#endif

        case DSP_INSTR_DO_ALTERNATIVE_PLC:
            if (inst->codec_ptr_inst.funcDecodePLC != 0)
            {
                len = inst->codec_ptr_inst.funcDecodePLC(inst->codec_ptr_inst.codec_state,
                    pw16_NetEqAlgorithm_buffer, 1);
            }
            else
            {
                len = inst->timestampsPerCall;
                
                WebRtcSpl_MemSetW16(pw16_NetEqAlgorithm_buffer, 0, len);
                
                inst->statInst.addedSamples += len;
            }
            inst->ExpandInst.w16_consecExp = 0;
            break;
        case DSP_INSTR_DO_ALTERNATIVE_PLC_INC_TS:
            if (inst->codec_ptr_inst.funcDecodePLC != 0)
            {
                len = inst->codec_ptr_inst.funcDecodePLC(inst->codec_ptr_inst.codec_state,
                    pw16_NetEqAlgorithm_buffer, 1);
            }
            else
            {
                len = inst->timestampsPerCall;
                
                WebRtcSpl_MemSetW16(pw16_NetEqAlgorithm_buffer, 0, len);
            }
            inst->ExpandInst.w16_consecExp = 0;
            inst->endTimestamp += len;
            break;
        case DSP_INSTR_DO_AUDIO_REPETITION:
            len = inst->timestampsPerCall;
            
            WEBRTC_SPL_MEMCPY_W16(pw16_NetEqAlgorithm_buffer,
                                  &inst->speechBuffer[inst->endPosition-len], len);
            inst->ExpandInst.w16_consecExp = 0;
            break;
        case DSP_INSTR_DO_AUDIO_REPETITION_INC_TS:
            len = inst->timestampsPerCall;
            
            WEBRTC_SPL_MEMCPY_W16(pw16_NetEqAlgorithm_buffer,
                                  &inst->speechBuffer[inst->endPosition-len], len);
            inst->ExpandInst.w16_consecExp = 0;
            inst->endTimestamp += len;
            break;

        case DSP_INSTR_PREEMPTIVE_EXPAND:
            if (len < 3 * inst->timestampsPerCall)
            {
                
                borrowedSamples = 3 * inst->timestampsPerCall - len; 
                
                oldBorrowedSamples = WEBRTC_SPL_MAX(0,
                    borrowedSamples - (inst->endPosition - inst->curPosition));
                WEBRTC_SPL_MEMMOVE_W16(&pw16_decoded_buffer[borrowedSamples],
                                       pw16_decoded_buffer, len);
                WEBRTC_SPL_MEMCPY_W16(pw16_decoded_buffer,
                                      &(inst->speechBuffer[inst->endPosition-borrowedSamples]),
                                      borrowedSamples);
            }
            else
            {
                borrowedSamples = 0;
                oldBorrowedSamples = 0;
            }

#ifdef NETEQ_DELAY_LOGGING
            w16_tmp1 = len;
#endif
            
            return_value = WebRtcNetEQ_PreEmptiveExpand(inst,
#ifdef SCRATCH
                
                pw16_scratchPtr + SCRATCH_NETEQ_ACCELERATE,
#endif
                pw16_decoded_buffer, len + borrowedSamples, oldBorrowedSamples,
                pw16_NetEqAlgorithm_buffer, &len, BGNonly);

            if (return_value < 0)
            {
                
                return return_value;
            }

            if (borrowedSamples > 0)
            {
                

                
                WEBRTC_SPL_MEMCPY_W16( &(inst->speechBuffer[inst->endPosition-borrowedSamples]),
                    pw16_NetEqAlgorithm_buffer,
                    borrowedSamples);

                len -= borrowedSamples; 

                
                WEBRTC_SPL_MEMMOVE_W16( pw16_NetEqAlgorithm_buffer,
                    &pw16_NetEqAlgorithm_buffer[borrowedSamples],
                    len);
            }

#ifdef NETEQ_DELAY_LOGGING
            temp_var = NETEQ_DELAY_LOGGING_SIGNAL_PREEMPTIVE_INFO;
            if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
              return -1;
            }
            temp_var = len - w16_tmp1; 
            if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
              return -1;
            }
#endif
            
            if (speechType == TYPE_CNG) inst->w16_mode = MODE_CODEC_INTERNAL_CNG;
#ifdef NETEQ_ATEVENT_DECODE
            if (playDtmf == 0)
            {
                inst->DTMFInst.reinit = 1;
            }
#endif
            break;

        case DSP_INSTR_FADE_TO_BGN:
        {
            int tempReturnValue;
            

            
            w16_tmp3 = WEBRTC_SPL_MIN(inst->endPosition - inst->curPosition,
                    inst->timestampsPerCall);
            
            if (w16_tmp3 + inst->w16_frameLen > NETEQ_MAX_OUTPUT_SIZE)
            {
                w16_tmp3 = NETEQ_MAX_OUTPUT_SIZE - inst->w16_frameLen;
            }

            
            len = inst->timestampsPerCall + inst->ExpandInst.w16_overlap;
            pos = 0;

            tempReturnValue = WebRtcNetEQ_Expand(inst,
#ifdef SCRATCH
                pw16_scratchPtr + SCRATCH_NETEQ_EXPAND,
#endif
                pw16_NetEqAlgorithm_buffer, &len, 1);

            if (tempReturnValue < 0)
            {
                
                
                return tempReturnValue;
            }

            pos += len; 

            
            while (pos + len <= inst->w16_frameLen + w16_tmp3)
            {
                WEBRTC_SPL_MEMCPY_W16(&pw16_NetEqAlgorithm_buffer[pos],
                    pw16_NetEqAlgorithm_buffer, len);
                pos += len;
            }

            
            if (pos < inst->w16_frameLen + w16_tmp3)
            {
                WEBRTC_SPL_MEMCPY_W16(&pw16_NetEqAlgorithm_buffer[pos], pw16_NetEqAlgorithm_buffer,
                    inst->w16_frameLen + w16_tmp3 - pos);
            }

            len = inst->w16_frameLen + w16_tmp3; 

            



            w16_tmp1 = 2;
            w16_tmp2 = 16384;
            while (w16_tmp1 <= w16_tmp3)
            {
                w16_tmp2 >>= 1; 
                w16_tmp1 <<= 1; 
            }

            w16_tmp1 = 0;
            pos = 0;
            while (w16_tmp1 < 16384)
            {
                inst->speechBuffer[inst->curPosition + pos]
                    =
                    (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(
                        WEBRTC_SPL_MUL_16_16( inst->speechBuffer[inst->endPosition - w16_tmp3 + pos],
                            16384-w16_tmp1 ) +
                        WEBRTC_SPL_MUL_16_16( pw16_NetEqAlgorithm_buffer[pos], w16_tmp1 ),
                        14 );
                w16_tmp1 += w16_tmp2;
                pos++;
            }

            

            WEBRTC_SPL_MEMCPY_W16( &inst->speechBuffer[inst->endPosition - w16_tmp3 + pos],
                &pw16_NetEqAlgorithm_buffer[pos], w16_tmp3 - pos);

            len -= w16_tmp3;
            

            WEBRTC_SPL_MEMMOVE_W16( pw16_NetEqAlgorithm_buffer,
                &pw16_NetEqAlgorithm_buffer[w16_tmp3],
                len );

            
            inst->w16_concealedTS += len;

            inst->w16_mode = MODE_FADE_TO_BGN;
#ifdef NETEQ_DELAY_LOGGING
            temp_var = NETEQ_DELAY_LOGGING_SIGNAL_EXPAND_INFO;
            if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
              return -1;
            }
            temp_var = len;
            if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
              return -1;
            }
#endif

            break;
        }

        default:
            inst->w16_mode = MODE_ERROR;
            dspInfo->lastMode = MODE_ERROR;
            return FAULTY_INSTRUCTION;
    } 

    

    w16_tmp2 = 0;
    if ((inst->endPosition + len - inst->curPosition - inst->ExpandInst.w16_overlap)
        >= inst->timestampsPerCall)
    {
        w16_tmp2 = inst->endPosition - inst->curPosition;
        w16_tmp2 = WEBRTC_SPL_MAX(w16_tmp2, 0); 
        w16_tmp1 = WEBRTC_SPL_MIN(w16_tmp2, inst->timestampsPerCall);
        w16_tmp2 = inst->timestampsPerCall - w16_tmp1;
        WEBRTC_SPL_MEMCPY_W16(pw16_outData, &inst->speechBuffer[inst->curPosition], w16_tmp1);
        WEBRTC_SPL_MEMCPY_W16(&pw16_outData[w16_tmp1], pw16_NetEqAlgorithm_buffer, w16_tmp2);
        DataEnough = 1;
    }
    else
    {
        DataEnough = 0;
    }

    if (playDtmf != 0)
    {
#ifdef NETEQ_ATEVENT_DECODE
        WebRtc_Word16 outDataIndex = 0;
        WebRtc_Word16 overdubLen = -1; 
        WebRtc_Word16 dtmfLen;

        



        if (inst->DTMFInst.lastDtmfSample - inst->curPosition > 0)
        {
            
            outDataIndex
                = WEBRTC_SPL_MIN(inst->DTMFInst.lastDtmfSample - inst->curPosition,
                    inst->timestampsPerCall);
            overdubLen = inst->timestampsPerCall - outDataIndex;
        }

        dtmfLen = WebRtcNetEQ_DTMFGenerate(&inst->DTMFInst, dtmfValue, dtmfVolume,
            &pw16_outData[outDataIndex], inst->fs, overdubLen);
        if (dtmfLen < 0)
        {
            
            return_value = dtmfLen;
        }
        inst->DTMFInst.reinit = 0;
#else
        inst->w16_mode = MODE_ERROR;
        dspInfo->lastMode = MODE_ERROR;
        return FAULTY_INSTRUCTION;
#endif
    }

    



    if (instr != DSP_INSTR_EXPAND)
    {
        w16_tmp1 = WEBRTC_SPL_MIN(inst->endPosition, len);
        WEBRTC_SPL_MEMMOVE_W16(inst->speechBuffer, inst->speechBuffer + w16_tmp1,
                               (inst->endPosition-w16_tmp1));
        WEBRTC_SPL_MEMCPY_W16(&inst->speechBuffer[inst->endPosition-w16_tmp1],
                              &pw16_NetEqAlgorithm_buffer[len-w16_tmp1], w16_tmp1);
#ifdef NETEQ_ATEVENT_DECODE
        
        if (instr == DSP_INSTR_DTMF_GENERATE)
        {
            
            inst->DTMFInst.lastDtmfSample = inst->endPosition;
        }
        else if (inst->DTMFInst.lastDtmfSample > 0)
        {
            
            inst->DTMFInst.lastDtmfSample -= w16_tmp1;
        }
#endif
        



        if ((inst->w16_mode != MODE_EXPAND) && (inst->w16_mode != MODE_MERGE)
            && (inst->w16_mode != MODE_SUCCESS_ACCELERATE) && (inst->w16_mode
            != MODE_LOWEN_ACCELERATE) && (inst->w16_mode != MODE_SUCCESS_PREEMPTIVE)
            && (inst->w16_mode != MODE_LOWEN_PREEMPTIVE) && (inst->w16_mode
            != MODE_FADE_TO_BGN) && (inst->w16_mode != MODE_DTMF) && (!BGNonly))
        {
            WebRtcNetEQ_BGNUpdate(inst
#ifdef SCRATCH
                , pw16_scratchPtr + SCRATCH_NETEQ_BGN_UPDATE
#endif
            );
        }
    }
    else 
    {
        
    }

    inst->curPosition -= len;

    



    if (inst->curPosition < -inst->timestampsPerCall)
    {
        inst->curPosition = -inst->timestampsPerCall;
    }

    if ((instr != DSP_INSTR_EXPAND) && (instr != DSP_INSTR_MERGE) && (instr
        != DSP_INSTR_FADE_TO_BGN))
    {
        
        if (inst->w16_concealedTS > inst->timestampsPerCall)
        {
            inst->w16_concealedTS = 0;
        }
    }

    



    if (!DataEnough)
    {
        
        WebRtcSpl_MemSetW16(pw16_outData, 0, inst->timestampsPerCall);
        *pw16_len = inst->timestampsPerCall;
        inst->w16_mode = MODE_ERROR;
        dspInfo->lastMode = MODE_ERROR;
        return RECOUT_ERROR_SAMPLEUNDERRUN;
    }

    



    if ((inst->w16_mode != MODE_EXPAND) && (inst->w16_mode != MODE_RFC3389CNG))
    {
        WebRtc_UWord32 uw32_tmpTS;
        uw32_tmpTS = inst->endTimestamp - (inst->endPosition - inst->curPosition);
        if ((WebRtc_Word32) (uw32_tmpTS - inst->videoSyncTimestamp) > 0)
        {
            inst->videoSyncTimestamp = uw32_tmpTS;
        }
    }
    else
    {
        inst->videoSyncTimestamp += inst->timestampsPerCall;
    }

    
    inst->curPosition += inst->timestampsPerCall;
    *pw16_len = inst->timestampsPerCall;

    
    if (BGNonly)
    {
        inst->w16_mode |= MODE_BGN_ONLY;
    }

    return return_value;
}

#undef    SCRATCH_ALGORITHM_BUFFER
#undef    SCRATCH_NETEQ_NORMAL
#undef    SCRATCH_NETEQ_MERGE
#undef    SCRATCH_NETEQ_BGN_UPDATE
#undef    SCRATCH_NETEQ_EXPAND
#undef    SCRATCH_DSP_INFO
#undef    SCRATCH_NETEQ_ACCELERATE
#undef    SIZE_SCRATCH_BUFFER
