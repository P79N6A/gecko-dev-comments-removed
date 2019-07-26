














#include "mcu.h"

#include <string.h>

#include "signal_processing_library.h"

#include "automode.h"
#include "dtmf_buffer.h"
#include "neteq_defines.h"
#include "neteq_error_codes.h"


int WebRtcNetEQ_RecInInternal(MCUInst_t *MCU_inst, RTPPacket_t *RTPpacketInput,
                              WebRtc_UWord32 uw32_timeRec)
{
    RTPPacket_t RTPpacket[2];
    int i_k;
    int i_ok = 0, i_No_Of_Payloads = 1;
    WebRtc_Word16 flushed = 0;
    WebRtc_Word16 codecPos;
    int curr_Codec;
    WebRtc_Word16 isREDPayload = 0;
    WebRtc_Word32 temp_bufsize;
#ifdef NETEQ_RED_CODEC
    RTPPacket_t* RTPpacketPtr[2]; 
    RTPpacketPtr[0] = &RTPpacket[0];
    RTPpacketPtr[1] = &RTPpacket[1];
#endif

    temp_bufsize = WebRtcNetEQ_PacketBufferGetSize(&MCU_inst->PacketBuffer_inst,
                                                   &MCU_inst->codec_DB_inst);
    




    WEBRTC_SPL_MEMCPY_W8(&RTPpacket[0], RTPpacketInput, sizeof(RTPPacket_t));

    

    if ((RTPpacket[0].ssrc != MCU_inst->ssrc) || (MCU_inst->first_packet == 1))
    {
        WebRtcNetEQ_RTCPInit(&MCU_inst->RTCP_inst, RTPpacket[0].seqNumber);
        MCU_inst->first_packet = 0;

        
        WebRtcNetEQ_PacketBufferFlush(&MCU_inst->PacketBuffer_inst);

        
        MCU_inst->ssrc = RTPpacket[0].ssrc;

        
        MCU_inst->timeStamp = RTPpacket[0].timeStamp;
        MCU_inst->current_Payload = RTPpacket[0].payloadType;

        
        MCU_inst->new_codec = 1;

        
        MCU_inst->TSscalingInitialized = 0;

    }

    
    i_ok |= WebRtcNetEQ_RTCPUpdate(&(MCU_inst->RTCP_inst), RTPpacket[0].seqNumber,
        RTPpacket[0].timeStamp, uw32_timeRec);

    
#ifdef NETEQ_RED_CODEC
    if (RTPpacket[0].payloadType == WebRtcNetEQ_DbGetPayload(&MCU_inst->codec_DB_inst,
        kDecoderRED))
    {

        
        i_ok = WebRtcNetEQ_RedundancySplit(RTPpacketPtr, 2, &i_No_Of_Payloads);
        if (i_ok < 0)
        {
            
            return i_ok;
        }

        



        if ((i_No_Of_Payloads > 1) && (RTPpacket[0].payloadType != RTPpacket[1].payloadType)
            && (RTPpacket[0].payloadType != WebRtcNetEQ_DbGetPayload(&MCU_inst->codec_DB_inst,
                kDecoderAVT)) && (RTPpacket[1].payloadType != WebRtcNetEQ_DbGetPayload(
            &MCU_inst->codec_DB_inst, kDecoderAVT)) && (!WebRtcNetEQ_DbIsCNGPayload(
            &MCU_inst->codec_DB_inst, RTPpacket[0].payloadType))
            && (!WebRtcNetEQ_DbIsCNGPayload(&MCU_inst->codec_DB_inst, RTPpacket[1].payloadType)))
        {
            i_No_Of_Payloads = 1;
        }
        isREDPayload = 1;
    }
#endif

    
    for (i_k = 0; i_k < i_No_Of_Payloads; i_k++)
    {

        if (isREDPayload == 1)
        {
            RTPpacket[i_k].rcuPlCntr = i_k;
        }
        else
        {
            RTPpacket[i_k].rcuPlCntr = 0;
        }

        
        if (RTPpacket[i_k].payloadType == WebRtcNetEQ_DbGetPayload(&MCU_inst->codec_DB_inst,
            kDecoderILBC))
        {
            i_ok = WebRtcNetEQ_DbGetSplitInfo(
                &MCU_inst->PayloadSplit_inst,
                (enum WebRtcNetEQDecoder) WebRtcNetEQ_DbGetCodec(&MCU_inst->codec_DB_inst,
                    RTPpacket[i_k].payloadType), RTPpacket[i_k].payloadLen);
            if (i_ok < 0)
            {
                
                return i_ok;
            }
        }

        
        i_ok = WebRtcNetEQ_GetTimestampScaling(MCU_inst, RTPpacket[i_k].payloadType);
        if (i_ok < 0)
        {
            
            return i_ok;
        }

        if (MCU_inst->TSscalingInitialized == 0 && MCU_inst->scalingFactor != kTSnoScaling)
        {
            
            MCU_inst->externalTS = RTPpacket[i_k].timeStamp;
            MCU_inst->internalTS = RTPpacket[i_k].timeStamp;
            MCU_inst->TSscalingInitialized = 1;
        }

        
        if (MCU_inst->TSscalingInitialized == 1)
        {
            WebRtc_UWord32 newTS = WebRtcNetEQ_ScaleTimestampExternalToInternal(MCU_inst,
                RTPpacket[i_k].timeStamp);

            
            MCU_inst->externalTS = RTPpacket[i_k].timeStamp;

            
            MCU_inst->internalTS = newTS;

            RTPpacket[i_k].timeStamp = newTS;
        }

        
        if (RTPpacket[i_k].payloadType == WebRtcNetEQ_DbGetPayload(&MCU_inst->codec_DB_inst,
            kDecoderAVT))
        {
#ifdef NETEQ_ATEVENT_DECODE
            if (MCU_inst->AVT_PlayoutOn)
            {
                i_ok = WebRtcNetEQ_DtmfInsertEvent(&MCU_inst->DTMF_inst,
                    RTPpacket[i_k].payload, RTPpacket[i_k].payloadLen,
                    RTPpacket[i_k].timeStamp);
                if (i_ok != 0)
                {
                    return i_ok;
                }
            }
#endif
#ifdef NETEQ_STEREO
            if (MCU_inst->usingStereo == 0)
            {
                
                MCU_inst->BufferStat_inst.Automode_inst.lastPackCNGorDTMF = 1;
            }
#else
            MCU_inst->BufferStat_inst.Automode_inst.lastPackCNGorDTMF = 1;
#endif
        }
        else if (WebRtcNetEQ_DbIsCNGPayload(&MCU_inst->codec_DB_inst,
            RTPpacket[i_k].payloadType))
        {
            
#ifdef NETEQ_CNG_CODEC
            
            WebRtc_UWord16 fsCng = WebRtcNetEQ_DbGetSampleRate(&MCU_inst->codec_DB_inst,
                RTPpacket[i_k].payloadType);

            
            

            if (fsCng > 32000) {
                fsCng = 32000;
            }
            if ((fsCng != MCU_inst->fs) && (fsCng > 8000))
            {
                




                WebRtcNetEQ_PacketBufferFlush(&MCU_inst->PacketBuffer_inst);
                MCU_inst->new_codec = 1;
                MCU_inst->current_Codec = -1;
            }
            i_ok = WebRtcNetEQ_PacketBufferInsert(&MCU_inst->PacketBuffer_inst,
                &RTPpacket[i_k], &flushed);
            if (i_ok < 0)
            {
                return RECIN_CNG_ERROR;
            }
            MCU_inst->BufferStat_inst.Automode_inst.lastPackCNGorDTMF = 1;
#else 
            return RECIN_UNKNOWNPAYLOAD;
#endif 
        }
        else
        {
            
            curr_Codec = WebRtcNetEQ_DbGetCodec(&MCU_inst->codec_DB_inst,
                RTPpacket[i_k].payloadType);
            if (curr_Codec != MCU_inst->current_Codec)
            {
                if (curr_Codec < 0)
                {
                    return RECIN_UNKNOWNPAYLOAD;
                }
                MCU_inst->current_Codec = curr_Codec;
                MCU_inst->current_Payload = RTPpacket[i_k].payloadType;
                i_ok = WebRtcNetEQ_DbGetSplitInfo(&MCU_inst->PayloadSplit_inst,
                    (enum WebRtcNetEQDecoder) MCU_inst->current_Codec,
                    RTPpacket[i_k].payloadLen);
                if (i_ok < 0)
                { 
                    return i_ok;
                }
                WebRtcNetEQ_PacketBufferFlush(&MCU_inst->PacketBuffer_inst);
                MCU_inst->new_codec = 1;
            }

            
            i_ok = WebRtcNetEQ_SplitAndInsertPayload(&RTPpacket[i_k],
                &MCU_inst->PacketBuffer_inst, &MCU_inst->PayloadSplit_inst, &flushed);
            if (i_ok < 0)
            {
                return i_ok;
            }
            if (MCU_inst->BufferStat_inst.Automode_inst.lastPackCNGorDTMF != 0)
            {
                
                MCU_inst->BufferStat_inst.Automode_inst.lastPackCNGorDTMF = -1;
            }
        }
        
        if (flushed)
        {
            MCU_inst->new_codec = 1;
        }
    }

    



    if ((curr_Codec = WebRtcNetEQ_DbGetCodec(&MCU_inst->codec_DB_inst,
        RTPpacket[0].payloadType)) >= 0)
    {
        codecPos = MCU_inst->codec_DB_inst.position[curr_Codec];
        if (MCU_inst->codec_DB_inst.funcUpdBWEst[codecPos] != NULL) 
        {
            if (RTPpacket[0].starts_byte1) 
            {
                
                for (i_k = 0; i_k < RTPpacket[0].payloadLen; i_k++)
                {
                    WEBRTC_SPL_SET_BYTE(RTPpacket[0].payload,
                        WEBRTC_SPL_GET_BYTE(RTPpacket[0].payload, i_k+1),
                        i_k);
                }
                RTPpacket[0].starts_byte1 = 0;
            }

            MCU_inst->codec_DB_inst.funcUpdBWEst[codecPos](
                MCU_inst->codec_DB_inst.codec_state[codecPos],
                (G_CONST WebRtc_UWord16 *) RTPpacket[0].payload,
                (WebRtc_Word32) RTPpacket[0].payloadLen, RTPpacket[0].seqNumber,
                (WebRtc_UWord32) RTPpacket[0].timeStamp, (WebRtc_UWord32) uw32_timeRec);
        }
    }

    if (MCU_inst->BufferStat_inst.Automode_inst.lastPackCNGorDTMF == 0)
    {
        
        temp_bufsize = WebRtcNetEQ_PacketBufferGetSize(
            &MCU_inst->PacketBuffer_inst, &MCU_inst->codec_DB_inst)
            - temp_bufsize;

        if ((temp_bufsize > 0) && (MCU_inst->BufferStat_inst.Automode_inst.lastPackCNGorDTMF
            == 0) && (temp_bufsize
            != MCU_inst->BufferStat_inst.Automode_inst.packetSpeechLenSamp))
        {
            
            WebRtcNetEQ_SetPacketSpeechLen(&(MCU_inst->BufferStat_inst.Automode_inst),
                (WebRtc_Word16) temp_bufsize, MCU_inst->fs);
        }

        
        if ((WebRtc_Word32) (RTPpacket[0].timeStamp - MCU_inst->timeStamp) >= 0
            && !MCU_inst->new_codec)
        {
            



            WebRtcNetEQ_UpdateIatStatistics(&MCU_inst->BufferStat_inst.Automode_inst,
                MCU_inst->PacketBuffer_inst.maxInsertPositions, RTPpacket[0].seqNumber,
                RTPpacket[0].timeStamp, MCU_inst->fs,
                WebRtcNetEQ_DbIsMDCodec((enum WebRtcNetEQDecoder) MCU_inst->current_Codec),
                (MCU_inst->NetEqPlayoutMode == kPlayoutStreaming));
        }
    }
    else if (MCU_inst->BufferStat_inst.Automode_inst.lastPackCNGorDTMF == -1)
    {
        




        MCU_inst->BufferStat_inst.Automode_inst.lastPackCNGorDTMF = 0;
        MCU_inst->BufferStat_inst.Automode_inst.packetIatCountSamp = 0;
    }
    return 0;

}

int WebRtcNetEQ_GetTimestampScaling(MCUInst_t *MCU_inst, int rtpPayloadType)
{
    enum WebRtcNetEQDecoder codec;
    int codecNumber;

    codecNumber = WebRtcNetEQ_DbGetCodec(&MCU_inst->codec_DB_inst, rtpPayloadType);
    if (codecNumber < 0)
    {
        
        return codecNumber;
    }

    
    codec = (enum WebRtcNetEQDecoder) codecNumber;

    



    switch (codec)
    {
        case kDecoderG722:
        case kDecoderG722_2ch:
        {
            
            MCU_inst->scalingFactor = kTSscalingTwo;
            break;
        }
        case kDecoderISACfb:
        case kDecoderOpus:
        {
            


            MCU_inst->scalingFactor = kTSscalingTwoThirds;
            break;
        }

        case kDecoderAVT:
        case kDecoderCNG:
        {
            

            WebRtc_UWord16 sample_freq =
                WebRtcNetEQ_DbGetSampleRate(&MCU_inst->codec_DB_inst,
                                            rtpPayloadType);
            if (sample_freq == 48000) {
              MCU_inst->scalingFactor = kTSscalingTwoThirds;
            }

            

            break;
        }
        default:
        {
            
            MCU_inst->scalingFactor = kTSnoScaling;
            break;
        }
    }
    return 0;
}

WebRtc_UWord32 WebRtcNetEQ_ScaleTimestampExternalToInternal(const MCUInst_t *MCU_inst,
                                                            WebRtc_UWord32 externalTS)
{
    WebRtc_Word32 timestampDiff;
    WebRtc_UWord32 internalTS;

    
    timestampDiff = externalTS - MCU_inst->externalTS;

    switch (MCU_inst->scalingFactor)
    {
        case kTSscalingTwo:
        {
            
            timestampDiff = WEBRTC_SPL_LSHIFT_W32(timestampDiff, 1);
            break;
        }
        case kTSscalingTwoThirds:
        {
            
            timestampDiff = WEBRTC_SPL_LSHIFT_W32(timestampDiff, 1);
            timestampDiff = WebRtcSpl_DivW32W16(timestampDiff, 3);
            break;
        }
        case kTSscalingFourThirds:
        {
            
            timestampDiff = WEBRTC_SPL_LSHIFT_W32(timestampDiff, 2);
            timestampDiff = WebRtcSpl_DivW32W16(timestampDiff, 3);
            break;
        }
        default:
        {
            
        }
    }

    
    internalTS = MCU_inst->internalTS + timestampDiff;

    return internalTS;
}

WebRtc_UWord32 WebRtcNetEQ_ScaleTimestampInternalToExternal(const MCUInst_t *MCU_inst,
                                                            WebRtc_UWord32 internalTS)
{
    WebRtc_Word32 timestampDiff;
    WebRtc_UWord32 externalTS;

    
    timestampDiff = (WebRtc_Word32) internalTS - MCU_inst->internalTS;

    switch (MCU_inst->scalingFactor)
    {
        case kTSscalingTwo:
        {
            
            timestampDiff = WEBRTC_SPL_RSHIFT_W32(timestampDiff, 1);
            break;
        }
        case kTSscalingTwoThirds:
        {
            
            timestampDiff = WEBRTC_SPL_MUL_32_16(timestampDiff, 3);
            timestampDiff = WEBRTC_SPL_RSHIFT_W32(timestampDiff, 1);
            break;
        }
        case kTSscalingFourThirds:
        {
            
            timestampDiff = WEBRTC_SPL_MUL_32_16(timestampDiff, 3);
            timestampDiff = WEBRTC_SPL_RSHIFT_W32(timestampDiff, 2);
            break;
        }
        default:
        {
            
        }
    }

    
    externalTS = MCU_inst->externalTS + timestampDiff;

    return externalTS;
}
