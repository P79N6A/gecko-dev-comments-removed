













#include "webrtc_neteq.h"
#include "webrtc_neteq_internal.h"

#include <assert.h>
#include <string.h>

#include "typedefs.h"
#include "signal_processing_library.h"

#include "neteq_error_codes.h"
#include "mcu_dsp_common.h"
#include "rtcp.h"

#define RETURN_ON_ERROR( macroExpr, macroInstPtr )  { \
    if ((macroExpr) != 0) { \
    if ((macroExpr) == -1) { \
    (macroInstPtr)->ErrorCode = - (NETEQ_OTHER_ERROR); \
    } else { \
    (macroInstPtr)->ErrorCode = -((WebRtc_Word16) (macroExpr)); \
    } \
    return(-1); \
    } }

int WebRtcNetEQ_strncpy(char *strDest, int numberOfElements,
                        const char *strSource, int count)
{
    
    if (count > numberOfElements)
    {
        strDest[0] = '\0';
        return (-1);
    }
    else
    {
        strncpy(strDest, strSource, count);
        return (0);
    }
}









int WebRtcNetEQ_GetErrorCode(void *inst)
{
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    return (NetEqMainInst->ErrorCode);
}

int WebRtcNetEQ_GetErrorName(int errorCode, char *errorName, int maxStrLen)
{
    if ((errorName == NULL) || (maxStrLen <= 0))
    {
        return (-1);
    }

    if (errorCode < 0)
    {
        errorCode = -errorCode; 
    }

    switch (errorCode)
    {
        case 1: 
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "OTHER_ERROR", maxStrLen);
            break;
        }
        case 1001:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "FAULTY_INSTRUCTION", maxStrLen);
            break;
        }
        case 1002:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "FAULTY_NETWORK_TYPE", maxStrLen);
            break;
        }
        case 1003:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "FAULTY_DELAYVALUE", maxStrLen);
            break;
        }
        case 1004:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "FAULTY_PLAYOUTMODE", maxStrLen);
            break;
        }
        case 1005:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "CORRUPT_INSTANCE", maxStrLen);
            break;
        }
        case 1006:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "ILLEGAL_MASTER_SLAVE_SWITCH", maxStrLen);
            break;
        }
        case 1007:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "MASTER_SLAVE_ERROR", maxStrLen);
            break;
        }
        case 2001:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "UNKNOWN_BUFSTAT_DECISION", maxStrLen);
            break;
        }
        case 2002:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "RECOUT_ERROR_DECODING", maxStrLen);
            break;
        }
        case 2003:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "RECOUT_ERROR_SAMPLEUNDERRUN", maxStrLen);
            break;
        }
        case 2004:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "RECOUT_ERROR_DECODED_TOO_MUCH",
                maxStrLen);
            break;
        }
        case 3001:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "RECIN_CNG_ERROR", maxStrLen);
            break;
        }
        case 3002:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "RECIN_UNKNOWNPAYLOAD", maxStrLen);
            break;
        }
        case 3003:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "RECIN_BUFFERINSERT_ERROR", maxStrLen);
            break;
        }
        case 4001:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "PBUFFER_INIT_ERROR", maxStrLen);
            break;
        }
        case 4002:
        case 4003:
        case 4004:
        case 4005:
        case 4006:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "PBUFFER_INSERT_ERROR1", maxStrLen);
            break;
        }
        case 4007:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "UNKNOWN_G723_HEADER", maxStrLen);
            break;
        }
        case 4008:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "PBUFFER_NONEXISTING_PACKET", maxStrLen);
            break;
        }
        case 4009:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "PBUFFER_NOT_INITIALIZED", maxStrLen);
            break;
        }
        case 4010:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "AMBIGUOUS_ILBC_FRAME_SIZE", maxStrLen);
            break;
        }
        case 5001:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "CODEC_DB_FULL", maxStrLen);
            break;
        }
        case 5002:
        case 5003:
        case 5004:
        case 5005:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "CODEC_DB_NOT_EXIST", maxStrLen);
            break;
        }
        case 5006:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "CODEC_DB_UNKNOWN_CODEC", maxStrLen);
            break;
        }
        case 5007:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "CODEC_DB_PAYLOAD_TAKEN", maxStrLen);
            break;
        }
        case 5008:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "CODEC_DB_UNSUPPORTED_CODEC", maxStrLen);
            break;
        }
        case 5009:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "CODEC_DB_UNSUPPORTED_FS", maxStrLen);
            break;
        }
        case 6001:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "DTMF_DEC_PARAMETER_ERROR", maxStrLen);
            break;
        }
        case 6002:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "DTMF_INSERT_ERROR", maxStrLen);
            break;
        }
        case 6003:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "DTMF_GEN_UNKNOWN_SAMP_FREQ", maxStrLen);
            break;
        }
        case 6004:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "DTMF_NOT_SUPPORTED", maxStrLen);
            break;
        }
        case 7001:
        case 7002:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "RED_SPLIT_ERROR", maxStrLen);
            break;
        }
        case 7003:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "RTP_TOO_SHORT_PACKET", maxStrLen);
            break;
        }
        case 7004:
        {
            WebRtcNetEQ_strncpy(errorName, maxStrLen, "RTP_CORRUPT_PACKET", maxStrLen);
            break;
        }
        default:
        {
            
            if (errorCode >= 6010 && errorCode <= 6810)
            {
                
                WebRtcNetEQ_strncpy(errorName, maxStrLen, "iSAC ERROR", maxStrLen);
                break;
            }

            WebRtcNetEQ_strncpy(errorName, maxStrLen, "UNKNOWN_ERROR", maxStrLen);
            return (-1);
        }
    }

    return (0);
}


int WebRtcNetEQ_AssignSize(int *sizeinbytes)
{
    *sizeinbytes = (sizeof(MainInst_t) * 2) / sizeof(WebRtc_Word16);
    return (0);
}

int WebRtcNetEQ_Assign(void **inst, void *NETEQ_inst_Addr)
{
    int ok = 0;
    MainInst_t *NetEqMainInst = (MainInst_t*) NETEQ_inst_Addr;
    *inst = NETEQ_inst_Addr;
    if (*inst == NULL) return (-1);

    WebRtcSpl_Init();

    
    WebRtcSpl_MemSetW16((WebRtc_Word16*) NetEqMainInst, 0,
        (sizeof(MainInst_t) / sizeof(WebRtc_Word16)));
    ok = WebRtcNetEQ_McuReset(&NetEqMainInst->MCUinst);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (0);
}

int WebRtcNetEQ_GetRecommendedBufferSize(void *inst, const enum WebRtcNetEQDecoder *codec,
                                         int noOfCodecs, enum WebRtcNetEQNetworkType nwType,
                                         int *MaxNoOfPackets, int *sizeinbytes)
{
    int ok = 0;
    int multiplier;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    *MaxNoOfPackets = 0;
    *sizeinbytes = 0;
    ok = WebRtcNetEQ_GetDefaultCodecSettings(codec, noOfCodecs, sizeinbytes, MaxNoOfPackets);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    if (nwType == kUDPNormal)
    {
        multiplier = 1;
    }
    else if (nwType == kUDPVideoSync)
    {
        multiplier = 4;
    }
    else if (nwType == kTCPNormal)
    {
        multiplier = 4;
    }
    else if (nwType == kTCPLargeJitter)
    {
        multiplier = 8;
    }
    else if (nwType == kTCPXLargeJitter)
    {
        multiplier = 20;
    }
    else
    {
        NetEqMainInst->ErrorCode = -FAULTY_NETWORK_TYPE;
        return (-1);
    }
    *MaxNoOfPackets = (*MaxNoOfPackets) * multiplier;
    *sizeinbytes = (*sizeinbytes) * multiplier;
    return 0;
}

int WebRtcNetEQ_AssignBuffer(void *inst, int MaxNoOfPackets, void *NETEQ_Buffer_Addr,
                             int sizeinbytes)
{
    int ok;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    ok = WebRtcNetEQ_PacketBufferInit(&NetEqMainInst->MCUinst.PacketBuffer_inst,
        MaxNoOfPackets, (WebRtc_Word16*) NETEQ_Buffer_Addr, (sizeinbytes >> 1));
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (ok);
}





















int WebRtcNetEQ_Init(void *inst, WebRtc_UWord16 fs)
{
    int ok = 0;

    
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;

    if (NetEqMainInst == NULL)
    {
        return (-1);
    }

#ifdef NETEQ_VAD
    
    NetEqMainInst->DSPinst.VADInst.VADState = NULL;
    
    NetEqMainInst->DSPinst.VADInst.initFunction = NULL;
    NetEqMainInst->DSPinst.VADInst.setmodeFunction = NULL;
    NetEqMainInst->DSPinst.VADInst.VADFunction = NULL;
#endif 

    ok = WebRtcNetEQ_DSPinit(NetEqMainInst); 
    RETURN_ON_ERROR(ok, NetEqMainInst);

    ok = WebRtcNetEQ_DSPInit(&NetEqMainInst->DSPinst, fs); 
    RETURN_ON_ERROR(ok, NetEqMainInst);
    
    NetEqMainInst->DSPinst.BGNInst.bgnMode = BGN_ON;

    
    ok = WebRtcNetEQ_ClearInCallStats(&NetEqMainInst->DSPinst);
    RETURN_ON_ERROR(ok, NetEqMainInst);
    ok = WebRtcNetEQ_ClearPostCallStats(&NetEqMainInst->DSPinst);
    RETURN_ON_ERROR(ok, NetEqMainInst);
    ok = WebRtcNetEQ_ResetMcuJitterStat(&NetEqMainInst->MCUinst);
    RETURN_ON_ERROR(ok, NetEqMainInst);

    
    ok = WebRtcNetEQ_PacketBufferFlush(&NetEqMainInst->MCUinst.PacketBuffer_inst);
    RETURN_ON_ERROR(ok, NetEqMainInst);

    
    NetEqMainInst->MCUinst.current_Codec = -1;
    NetEqMainInst->MCUinst.current_Payload = -1;
    NetEqMainInst->MCUinst.first_packet = 1;
    NetEqMainInst->MCUinst.one_desc = 0;
    NetEqMainInst->MCUinst.BufferStat_inst.Automode_inst.extraDelayMs = 0;
    NetEqMainInst->MCUinst.NoOfExpandCalls = 0;
    NetEqMainInst->MCUinst.fs = fs;

#ifdef NETEQ_ATEVENT_DECODE
    
    ok = WebRtcNetEQ_DtmfDecoderInit(&(NetEqMainInst->MCUinst.DTMF_inst),fs,560);
    RETURN_ON_ERROR(ok, NetEqMainInst);
#endif

    
    WebRtcNetEQ_RTCPInit(&(NetEqMainInst->MCUinst.RTCP_inst), 0);

    
    WebRtcSpl_MemSetW16((WebRtc_Word16*) &(NetEqMainInst->MCUinst.BufferStat_inst), 0,
        sizeof(BufstatsInst_t) / sizeof(WebRtc_Word16));

    
    WebRtcNetEQ_ResetAutomode(&(NetEqMainInst->MCUinst.BufferStat_inst.Automode_inst),
        NetEqMainInst->MCUinst.PacketBuffer_inst.maxInsertPositions);

    NetEqMainInst->ErrorCode = 0;

#ifdef NETEQ_STEREO
    
    NetEqMainInst->masterSlave = 0;
#endif

    return (ok);
}

int WebRtcNetEQ_FlushBuffers(void *inst)
{
    int ok = 0;

    
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;

    if (NetEqMainInst == NULL)
    {
        return (-1);
    }

    
    ok = WebRtcNetEQ_PacketBufferFlush(&NetEqMainInst->MCUinst.PacketBuffer_inst);
    RETURN_ON_ERROR(ok, NetEqMainInst);

    
    NetEqMainInst->MCUinst.first_packet = 1;

    
    ok = WebRtcNetEQ_FlushSpeechBuffer(&NetEqMainInst->DSPinst);
    RETURN_ON_ERROR(ok, NetEqMainInst);

    return 0;
}

int WebRtcNetEQ_SetAVTPlayout(void *inst, int PlayoutAVTon)
{
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
#ifdef NETEQ_ATEVENT_DECODE
    NetEqMainInst->MCUinst.AVT_PlayoutOn = PlayoutAVTon;
    return(0);
#else
    if (PlayoutAVTon != 0)
    {
        NetEqMainInst->ErrorCode = -DTMF_NOT_SUPPORTED;
        return (-1);
    }
    else
    {
        return (0);
    }
#endif
}

int WebRtcNetEQ_SetExtraDelay(void *inst, int DelayInMs)
{
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    if ((DelayInMs < 0) || (DelayInMs > 1000))
    {
        NetEqMainInst->ErrorCode = -FAULTY_DELAYVALUE;
        return (-1);
    }
    NetEqMainInst->MCUinst.BufferStat_inst.Automode_inst.extraDelayMs = DelayInMs;
    return (0);
}

int WebRtcNetEQ_SetPlayoutMode(void *inst, enum WebRtcNetEQPlayoutMode playoutMode)
{
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    if ((playoutMode != kPlayoutOn) && (playoutMode != kPlayoutOff) && (playoutMode
        != kPlayoutFax) && (playoutMode != kPlayoutStreaming))
    {
        NetEqMainInst->ErrorCode = -FAULTY_PLAYOUTMODE;
        return (-1);
    }
    else
    {
        NetEqMainInst->MCUinst.NetEqPlayoutMode = playoutMode;
        return (0);
    }
}

int WebRtcNetEQ_SetBGNMode(void *inst, enum WebRtcNetEQBGNMode bgnMode)
{

    MainInst_t *NetEqMainInst = (MainInst_t*) inst;

    
    if (NetEqMainInst == NULL) return (-1);

    
    if (NetEqMainInst->MCUinst.main_inst != NetEqMainInst)
    {
        
        NetEqMainInst->ErrorCode = CORRUPT_INSTANCE;
        return (-1);
    }

    NetEqMainInst->DSPinst.BGNInst.bgnMode = (enum BGNMode) bgnMode;

    return (0);
}

int WebRtcNetEQ_GetBGNMode(const void *inst, enum WebRtcNetEQBGNMode *bgnMode)
{

    const MainInst_t *NetEqMainInst = (const MainInst_t*) inst;

    
    if (NetEqMainInst == NULL) return (-1);

    *bgnMode = (enum WebRtcNetEQBGNMode) NetEqMainInst->DSPinst.BGNInst.bgnMode;

    return (0);
}





int WebRtcNetEQ_CodecDbReset(void *inst)
{
    int ok = 0;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    ok = WebRtcNetEQ_DbReset(&NetEqMainInst->MCUinst.codec_DB_inst);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }

    
    NetEqMainInst->DSPinst.codec_ptr_inst.funcDecode = NULL;
    NetEqMainInst->DSPinst.codec_ptr_inst.funcDecodeRCU = NULL;
    NetEqMainInst->DSPinst.codec_ptr_inst.funcAddLatePkt = NULL;
    NetEqMainInst->DSPinst.codec_ptr_inst.funcDecode = NULL;
    NetEqMainInst->DSPinst.codec_ptr_inst.funcDecodeInit = NULL;
    NetEqMainInst->DSPinst.codec_ptr_inst.funcDecodePLC = NULL;
    NetEqMainInst->DSPinst.codec_ptr_inst.funcGetMDinfo = NULL;
    NetEqMainInst->DSPinst.codec_ptr_inst.funcUpdBWEst = NULL;
    NetEqMainInst->DSPinst.codec_ptr_inst.funcGetErrorCode = NULL;

    return (0);
}

int WebRtcNetEQ_CodecDbGetSizeInfo(void *inst, WebRtc_Word16 *UsedEntries,
                                   WebRtc_Word16 *MaxEntries)
{
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    *MaxEntries = NUM_CODECS;
    *UsedEntries = NetEqMainInst->MCUinst.codec_DB_inst.nrOfCodecs;
    return (0);
}

int WebRtcNetEQ_CodecDbGetCodecInfo(void *inst, WebRtc_Word16 Entry,
                                    enum WebRtcNetEQDecoder *codec)
{
    int i;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    *codec = (enum WebRtcNetEQDecoder) 0;
    if ((Entry >= 0) && (Entry < NetEqMainInst->MCUinst.codec_DB_inst.nrOfCodecs))
    {
        for (i = 0; i < NUM_TOTAL_CODECS; i++)
        {
            if (NetEqMainInst->MCUinst.codec_DB_inst.position[i] == Entry)
            {
                *codec = (enum WebRtcNetEQDecoder) i;
            }
        }
    }
    else
    {
        NetEqMainInst->ErrorCode = -(CODEC_DB_NOT_EXIST1);
        return (-1);
    }
    return (0);
}

int WebRtcNetEQ_CodecDbAdd(void *inst, WebRtcNetEQ_CodecDef *codecInst)
{
    int ok = 0;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    ok = WebRtcNetEQ_DbAdd(&NetEqMainInst->MCUinst.codec_DB_inst, codecInst->codec,
        codecInst->payloadType, codecInst->funcDecode, codecInst->funcDecodeRCU,
        codecInst->funcDecodePLC, codecInst->funcDecodeInit, codecInst->funcAddLatePkt,
        codecInst->funcGetMDinfo, codecInst->funcGetPitch, codecInst->funcUpdBWEst,
        codecInst->funcDurationEst, codecInst->funcGetErrorCode,
        codecInst->codec_state, codecInst->codec_fs);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (ok);
}

int WebRtcNetEQ_CodecDbRemove(void *inst, enum WebRtcNetEQDecoder codec)
{
    int ok = 0;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);

    
    if (NetEqMainInst->MCUinst.current_Codec == (WebRtc_Word16) codec)
    {
        
        NetEqMainInst->DSPinst.codec_ptr_inst.funcDecode = NULL;
        NetEqMainInst->DSPinst.codec_ptr_inst.funcDecodeRCU = NULL;
        NetEqMainInst->DSPinst.codec_ptr_inst.funcAddLatePkt = NULL;
        NetEqMainInst->DSPinst.codec_ptr_inst.funcDecode = NULL;
        NetEqMainInst->DSPinst.codec_ptr_inst.funcDecodeInit = NULL;
        NetEqMainInst->DSPinst.codec_ptr_inst.funcDecodePLC = NULL;
        NetEqMainInst->DSPinst.codec_ptr_inst.funcGetMDinfo = NULL;
        NetEqMainInst->DSPinst.codec_ptr_inst.funcUpdBWEst = NULL;
        NetEqMainInst->DSPinst.codec_ptr_inst.funcGetErrorCode = NULL;
    }

    ok = WebRtcNetEQ_DbRemove(&NetEqMainInst->MCUinst.codec_DB_inst, codec);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (ok);
}





int WebRtcNetEQ_RecIn(void *inst, WebRtc_Word16 *p_w16datagramstart, WebRtc_Word16 w16_RTPlen,
                      WebRtc_UWord32 uw32_timeRec)
{
    int ok = 0;
    RTPPacket_t RTPpacket;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);

    
    if (NetEqMainInst->MCUinst.main_inst != NetEqMainInst)
    {
        
        NetEqMainInst->ErrorCode = CORRUPT_INSTANCE;
        return (-1);
    }

    
    ok = WebRtcNetEQ_RTPPayloadInfo(p_w16datagramstart, w16_RTPlen, &RTPpacket);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }

    ok = WebRtcNetEQ_RecInInternal(&NetEqMainInst->MCUinst, &RTPpacket, uw32_timeRec);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (ok);
}

















int WebRtcNetEQ_RecInRTPStruct(void *inst, WebRtcNetEQ_RTPInfo *rtpInfo,
                               const WebRtc_UWord8 *payloadPtr, WebRtc_Word16 payloadLenBytes,
                               WebRtc_UWord32 uw32_timeRec)
{
    int ok = 0;
    RTPPacket_t RTPpacket;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL)
    {
        return (-1);
    }

    
    if (NetEqMainInst->MCUinst.main_inst != NetEqMainInst)
    {
        
        NetEqMainInst->ErrorCode = CORRUPT_INSTANCE;
        return (-1);
    }

    
    RTPpacket.payloadType = rtpInfo->payloadType;
    RTPpacket.seqNumber = rtpInfo->sequenceNumber;
    RTPpacket.timeStamp = rtpInfo->timeStamp;
    RTPpacket.ssrc = rtpInfo->SSRC;
    RTPpacket.payload = (const WebRtc_Word16*) payloadPtr;
    RTPpacket.payloadLen = payloadLenBytes;
    RTPpacket.starts_byte1 = 0;

    ok = WebRtcNetEQ_RecInInternal(&NetEqMainInst->MCUinst, &RTPpacket, uw32_timeRec);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (ok);
}

int WebRtcNetEQ_RecOut(void *inst, WebRtc_Word16 *pw16_outData, WebRtc_Word16 *pw16_len)
{
    int ok = 0;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
#ifdef NETEQ_STEREO
    MasterSlaveInfo msInfo;
    msInfo.msMode = NETEQ_MONO;
#endif

    if (NetEqMainInst == NULL) return (-1);

    
    if (NetEqMainInst->DSPinst.main_inst != NetEqMainInst)
    {
        
        NetEqMainInst->ErrorCode = CORRUPT_INSTANCE;
        return (-1);
    }

#ifdef NETEQ_STEREO
    NetEqMainInst->DSPinst.msInfo = &msInfo;
#endif

    ok = WebRtcNetEQ_RecOutInternal(&NetEqMainInst->DSPinst, pw16_outData,
        pw16_len, 0 );
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (ok);
}






















int WebRtcNetEQ_RecOutMasterSlave(void *inst, WebRtc_Word16 *pw16_outData,
                                  WebRtc_Word16 *pw16_len, void *msInfo,
                                  WebRtc_Word16 isMaster)
{
#ifndef NETEQ_STEREO
    
    return(-1);
#else
    int ok = 0;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;

    if (NetEqMainInst == NULL) return (-1);

    
    if (NetEqMainInst->DSPinst.main_inst != NetEqMainInst)
    {
        
        NetEqMainInst->ErrorCode = CORRUPT_INSTANCE;
        return (-1);
    }

    if (msInfo == NULL)
    {
        
        NetEqMainInst->ErrorCode = NETEQ_OTHER_ERROR;
        return (-1);
    }

    
    NetEqMainInst->DSPinst.msInfo = (MasterSlaveInfo *) msInfo;

    
    if ((NetEqMainInst->masterSlave == 1 && !isMaster) || 
    (NetEqMainInst->masterSlave == 2 && isMaster)) 
    {
        NetEqMainInst->ErrorCode = ILLEGAL_MASTER_SLAVE_SWITCH;
        return (-1);
    }

    if (!isMaster)
    {
        
        NetEqMainInst->masterSlave = 2;
        NetEqMainInst->DSPinst.msInfo->msMode = NETEQ_SLAVE;
    }
    else
    {
        NetEqMainInst->DSPinst.msInfo->msMode = NETEQ_MASTER;
    }

    ok  = WebRtcNetEQ_RecOutInternal(&NetEqMainInst->DSPinst, pw16_outData,
        pw16_len, 0 );
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }

    if (isMaster)
    {
        
        NetEqMainInst->masterSlave = 1;
    }

    return (ok);
#endif
}

int WebRtcNetEQ_GetMasterSlaveInfoSize()
{
#ifdef NETEQ_STEREO
    return (sizeof(MasterSlaveInfo));
#else
    return(-1);
#endif
}


int WebRtcNetEQ_RecOutNoDecode(void *inst, WebRtc_Word16 *pw16_outData,
                               WebRtc_Word16 *pw16_len)
{
    int ok = 0;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
#ifdef NETEQ_STEREO
    MasterSlaveInfo msInfo;
#endif

    if (NetEqMainInst == NULL) return (-1);

    
    if (NetEqMainInst->DSPinst.main_inst != NetEqMainInst)
    {
        
        NetEqMainInst->ErrorCode = CORRUPT_INSTANCE;
        return (-1);
    }

#ifdef NETEQ_STEREO
    
    switch (NetEqMainInst->masterSlave)
    {
        case 1:
        {
            msInfo.msMode = NETEQ_MASTER;
            break;
        }
        case 2:
        {
            msInfo.msMode = NETEQ_SLAVE;
            break;
        }
        default:
        {
            msInfo.msMode = NETEQ_MONO;
            break;
        }
    }

    NetEqMainInst->DSPinst.msInfo = &msInfo;
#endif

    ok = WebRtcNetEQ_RecOutInternal(&NetEqMainInst->DSPinst, pw16_outData,
        pw16_len, 1 );
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (ok);
}

int WebRtcNetEQ_GetRTCPStats(void *inst, WebRtcNetEQ_RTCPStat *RTCP_inst)
{
    int ok = 0;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    ok = WebRtcNetEQ_RTCPGetStats(&NetEqMainInst->MCUinst.RTCP_inst,
        &RTCP_inst->fraction_lost, &RTCP_inst->cum_lost, &RTCP_inst->ext_max,
        &RTCP_inst->jitter, 0);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (ok);
}

int WebRtcNetEQ_GetRTCPStatsNoReset(void *inst, WebRtcNetEQ_RTCPStat *RTCP_inst)
{
    int ok = 0;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    ok = WebRtcNetEQ_RTCPGetStats(&NetEqMainInst->MCUinst.RTCP_inst,
        &RTCP_inst->fraction_lost, &RTCP_inst->cum_lost, &RTCP_inst->ext_max,
        &RTCP_inst->jitter, 1);
    if (ok != 0)
    {
        NetEqMainInst->ErrorCode = -ok;
        return (-1);
    }
    return (ok);
}

int WebRtcNetEQ_GetSpeechTimeStamp(void *inst, WebRtc_UWord32 *timestamp)
{
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);

    if (NetEqMainInst->MCUinst.TSscalingInitialized)
    {
        *timestamp = WebRtcNetEQ_ScaleTimestampInternalToExternal(&NetEqMainInst->MCUinst,
            NetEqMainInst->DSPinst.videoSyncTimestamp);
    }
    else
    {
        *timestamp = NetEqMainInst->DSPinst.videoSyncTimestamp;
    }

    return (0);
}























int WebRtcNetEQ_GetSpeechOutputType(void *inst, enum WebRtcNetEQOutputType *outputType)
{
    
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;

    if (NetEqMainInst == NULL)
    {
        return (-1);
    }

    if ((NetEqMainInst->DSPinst.w16_mode & MODE_BGN_ONLY) != 0)
    {
        
        *outputType = kOutputPLCtoCNG;

    }
    else if ((NetEqMainInst->DSPinst.w16_mode == MODE_CODEC_INTERNAL_CNG)
        || (NetEqMainInst->DSPinst.w16_mode == MODE_RFC3389CNG))
    {
        
        *outputType = kOutputCNG;

#ifdef NETEQ_VAD
    }
    else if ( NetEqMainInst->DSPinst.VADInst.VADDecision == 0 )
    {
        
        *outputType = kOutputVADPassive;
#endif

    }
    else if ((NetEqMainInst->DSPinst.w16_mode == MODE_EXPAND)
        && (NetEqMainInst->DSPinst.ExpandInst.w16_expandMuteFactor == 0))
    {
        
        *outputType = kOutputPLCtoCNG;

    }
    else if (NetEqMainInst->DSPinst.w16_mode == MODE_EXPAND)
    {
        
        *outputType = kOutputPLC;

    }
    else
    {
        
        *outputType = kOutputNormal;
    }

    return (0);
}





#define WEBRTC_NETEQ_CONCEALMENTFLAG_LOST       0x01
#define WEBRTC_NETEQ_CONCEALMENTFLAG_DISCARDED  0x02
#define WEBRTC_NETEQ_CONCEALMENTFLAG_SUPRESS    0x04
#define WEBRTC_NETEQ_CONCEALMENTFLAG_CNGACTIVE  0x80

int WebRtcNetEQ_VQmonRecOutStatistics(void *inst, WebRtc_UWord16 *validVoiceDurationMs,
                                      WebRtc_UWord16 *concealedVoiceDurationMs,
                                      WebRtc_UWord8 *concealedVoiceFlags)
{
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    WebRtc_Word16 fs_mult;
    WebRtc_Word16 ms_lost;
    if (NetEqMainInst == NULL) return (-1);
    fs_mult = WebRtcSpl_DivW32W16ResW16(NetEqMainInst->MCUinst.fs, 8000);

    ms_lost = WebRtcSpl_DivW32W16ResW16(
        (WebRtc_Word32) NetEqMainInst->DSPinst.w16_concealedTS, (WebRtc_Word16) (8 * fs_mult));
    if (ms_lost > NetEqMainInst->DSPinst.millisecondsPerCall) ms_lost
        = NetEqMainInst->DSPinst.millisecondsPerCall;

    *validVoiceDurationMs = NetEqMainInst->DSPinst.millisecondsPerCall - ms_lost;
    *concealedVoiceDurationMs = ms_lost;
    if (ms_lost > 0)
    {
        *concealedVoiceFlags = WEBRTC_NETEQ_CONCEALMENTFLAG_LOST;
    }
    else
    {
        *concealedVoiceFlags = 0;
    }
    NetEqMainInst->DSPinst.w16_concealedTS -= ms_lost * (8 * fs_mult);

    return (0);
}

int WebRtcNetEQ_VQmonGetConfiguration(void *inst, WebRtc_UWord16 *absMaxDelayMs,
                                      WebRtc_UWord8 *adaptationRate)
{
    
    if (inst == NULL)
    {
        
    }

    
    *absMaxDelayMs = 240;
    *adaptationRate = 1;
    return (0);
}

int WebRtcNetEQ_VQmonGetRxStatistics(void *inst, WebRtc_UWord16 *avgDelayMs,
                                     WebRtc_UWord16 *maxDelayMs)
{
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL) return (-1);
    *avgDelayMs = (WebRtc_UWord16) (NetEqMainInst->MCUinst.BufferStat_inst.avgDelayMsQ8 >> 8);
    *maxDelayMs = (WebRtc_UWord16) NetEqMainInst->MCUinst.BufferStat_inst.maxDelayMs;
    return (0);
}







int WebRtcNetEQ_GetNetworkStatistics(void *inst, WebRtcNetEQ_NetworkStatistics *stats)

{

    WebRtc_UWord16 tempU16;
    WebRtc_UWord32 tempU32, tempU32_2;
    int numShift;
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;

    
    if (NetEqMainInst == NULL) return (-1);

    stats->addedSamples = NetEqMainInst->DSPinst.statInst.addedSamples;

    
    
    

    if (NetEqMainInst->MCUinst.fs != 0)
    {
        WebRtc_Word32 temp32;
        
        temp32 = WebRtcNetEQ_PacketBufferGetSize(
            &NetEqMainInst->MCUinst.PacketBuffer_inst,
            &NetEqMainInst->MCUinst.codec_DB_inst);

        

        stats->currentBufferSize = (WebRtc_UWord16)
            WebRtcSpl_DivU32U16(temp32 * 1000, NetEqMainInst->MCUinst.fs);

        
        temp32 = (WebRtc_Word32) (NetEqMainInst->DSPinst.endPosition -
            NetEqMainInst->DSPinst.curPosition);
        stats->currentBufferSize += (WebRtc_UWord16)
            WebRtcSpl_DivU32U16(temp32 * 1000, NetEqMainInst->MCUinst.fs);
    }
    else
    {
        
        stats->currentBufferSize = 0;
    }

    
    
    

    if (NetEqMainInst->MCUinst.fs != 0 && NetEqMainInst->MCUinst.fs <= WEBRTC_SPL_WORD16_MAX)
    {
        
        stats->preferredBufferSize
            = (WebRtc_UWord16) WEBRTC_SPL_MUL_16_16(
                (WebRtc_Word16) ((NetEqMainInst->MCUinst.BufferStat_inst.Automode_inst.optBufLevel) >> 8), 
                WebRtcSpl_DivW32W16ResW16(
                    (WebRtc_Word32) NetEqMainInst->MCUinst.BufferStat_inst.Automode_inst.packetSpeechLenSamp, 
                    WebRtcSpl_DivW32W16ResW16( (WebRtc_Word32) NetEqMainInst->MCUinst.fs, (WebRtc_Word16) 1000 ) 
                ) );

        
        if (NetEqMainInst->MCUinst.BufferStat_inst.Automode_inst.extraDelayMs > 0)
        {
            stats->preferredBufferSize
                += NetEqMainInst->MCUinst.BufferStat_inst.Automode_inst.extraDelayMs;
        }
    }
    else
    {
        
        stats->preferredBufferSize = 0;
    }

    
    
    

    stats->jitterPeaksFound =
        NetEqMainInst->MCUinst.BufferStat_inst.Automode_inst.peakFound;

    
    
    

    
    tempU32 = NetEqMainInst->MCUinst.lastReportTS;

    if (NetEqMainInst->MCUinst.lostTS == 0)
    {
        
        stats->currentPacketLossRate = 0;
    }
    else if (NetEqMainInst->MCUinst.lostTS < tempU32)
    {
        
        numShift = WebRtcSpl_NormU32(NetEqMainInst->MCUinst.lostTS); 

        if (numShift < 14)
        {
            
            tempU32 = WEBRTC_SPL_RSHIFT_U32(tempU32, 14-numShift); 
        }
        else
        {
            
            numShift = 14;
        }

        if (tempU32 == 0)
        {
            
            stats->currentPacketLossRate = 0;
        }
        else
        {
            
            while (tempU32 > WEBRTC_SPL_WORD16_MAX)
            {
                tempU32 >>= 1; 
                numShift--; 
            }
            tempU16 = (WebRtc_UWord16) tempU32;

            
            tempU32
                = WEBRTC_SPL_SHIFT_W32( (WebRtc_UWord32) NetEqMainInst->MCUinst.lostTS, numShift);

            stats->currentPacketLossRate = (WebRtc_UWord16) WebRtcSpl_DivU32U16(tempU32,
                tempU16);
        }
    }
    else
    {
        
        
        stats->currentPacketLossRate = 1 << 14; 
    }

    
    
    

    
    tempU32 = NetEqMainInst->MCUinst.lastReportTS;

    
    tempU32_2
        = WEBRTC_SPL_MUL_16_U16( (WebRtc_Word16) NetEqMainInst->MCUinst.PacketBuffer_inst.packSizeSamples,
            NetEqMainInst->MCUinst.PacketBuffer_inst.discardedPackets);

    if (tempU32_2 == 0)
    {
        
        stats->currentDiscardRate = 0;
    }
    else if (tempU32_2 < tempU32)
    {
        
        numShift = WebRtcSpl_NormU32(tempU32_2); 

        if (numShift < 14)
        {
            
            tempU32 = WEBRTC_SPL_RSHIFT_U32(tempU32, 14-numShift); 
        }
        else
        {
            
            numShift = 14;
        }

        if (tempU32 == 0)
        {
            
            stats->currentDiscardRate = 0;
        }
        else
        {
            
            while (tempU32 > WEBRTC_SPL_WORD16_MAX)
            {
                tempU32 >>= 1; 
                numShift--; 
            }
            tempU16 = (WebRtc_UWord16) tempU32;

            
            tempU32 = WEBRTC_SPL_SHIFT_W32( tempU32_2, numShift);

            stats->currentDiscardRate = (WebRtc_UWord16) WebRtcSpl_DivU32U16(tempU32, tempU16);
        }
    }
    else
    {
        
        
        stats->currentDiscardRate = 1 << 14; 
    }

    
    
    

    
    tempU32 = NetEqMainInst->MCUinst.lastReportTS;

    if (NetEqMainInst->DSPinst.statInst.accelerateLength == 0)
    {
        
        stats->currentAccelerateRate = 0;
    }
    else if (NetEqMainInst->DSPinst.statInst.accelerateLength < tempU32)
    {
        
        numShift = WebRtcSpl_NormU32(NetEqMainInst->DSPinst.statInst.accelerateLength); 

        if (numShift < 14)
        {
            
            tempU32 = WEBRTC_SPL_RSHIFT_U32(tempU32, 14-numShift); 
        }
        else
        {
            
            numShift = 14;
        }

        if (tempU32 == 0)
        {
            
            stats->currentAccelerateRate = 0;
        }
        else
        {
            
            while (tempU32 > WEBRTC_SPL_WORD16_MAX)
            {
                tempU32 >>= 1; 
                numShift--; 
            }
            tempU16 = (WebRtc_UWord16) tempU32;

            
            tempU32
                = WEBRTC_SPL_SHIFT_W32( NetEqMainInst->DSPinst.statInst.accelerateLength, numShift);

            stats->currentAccelerateRate = (WebRtc_UWord16) WebRtcSpl_DivU32U16(tempU32,
                tempU16);
        }
    }
    else
    {
        
        
        stats->currentAccelerateRate = 1 << 14; 
    }

    
    tempU32 = NetEqMainInst->MCUinst.lastReportTS;

    if (NetEqMainInst->DSPinst.statInst.expandLength == 0)
    {
        
        stats->currentExpandRate = 0;
    }
    else if (NetEqMainInst->DSPinst.statInst.expandLength < tempU32)
    {
        
        numShift = WebRtcSpl_NormU32(NetEqMainInst->DSPinst.statInst.expandLength); 

        if (numShift < 14)
        {
            
            tempU32 = WEBRTC_SPL_RSHIFT_U32(tempU32, 14-numShift); 
        }
        else
        {
            
            numShift = 14;
        }

        if (tempU32 == 0)
        {
            
            stats->currentExpandRate = 0;
        }
        else
        {
            
            while (tempU32 > WEBRTC_SPL_WORD16_MAX)
            {
                tempU32 >>= 1; 
                numShift--; 
            }
            tempU16 = (WebRtc_UWord16) tempU32;

            
            tempU32
                = WEBRTC_SPL_SHIFT_W32( NetEqMainInst->DSPinst.statInst.expandLength, numShift);

            stats->currentExpandRate = (WebRtc_UWord16) WebRtcSpl_DivU32U16(tempU32, tempU16);
        }
    }
    else
    {
        
        
        stats->currentExpandRate = 1 << 14; 
    }

    
    tempU32 = NetEqMainInst->MCUinst.lastReportTS;

    if (NetEqMainInst->DSPinst.statInst.preemptiveLength == 0)
    {
        
        stats->currentPreemptiveRate = 0;
    }
    else if (NetEqMainInst->DSPinst.statInst.preemptiveLength < tempU32)
    {
        
        numShift = WebRtcSpl_NormU32(NetEqMainInst->DSPinst.statInst.preemptiveLength); 

        if (numShift < 14)
        {
            
            tempU32 = WEBRTC_SPL_RSHIFT_U32(tempU32, 14-numShift); 
        }
        else
        {
            
            numShift = 14;
        }

        if (tempU32 == 0)
        {
            
            stats->currentPreemptiveRate = 0;
        }
        else
        {
            
            while (tempU32 > WEBRTC_SPL_WORD16_MAX)
            {
                tempU32 >>= 1; 
                numShift--; 
            }
            tempU16 = (WebRtc_UWord16) tempU32;

            
            tempU32
                = WEBRTC_SPL_SHIFT_W32( NetEqMainInst->DSPinst.statInst.preemptiveLength, numShift);

            stats->currentPreemptiveRate = (WebRtc_UWord16) WebRtcSpl_DivU32U16(tempU32,
                tempU16);
        }
    }
    else
    {
        
        
        stats->currentPreemptiveRate = 1 << 14; 
    }

    stats->clockDriftPPM = WebRtcNetEQ_AverageIAT(
        &NetEqMainInst->MCUinst.BufferStat_inst.Automode_inst);

    
    WebRtcNetEQ_ResetMcuInCallStats(&(NetEqMainInst->MCUinst));
    WebRtcNetEQ_ClearInCallStats(&(NetEqMainInst->DSPinst));

    return (0);
}

int WebRtcNetEQ_GetRawFrameWaitingTimes(void *inst,
                                        int max_length,
                                        int* waiting_times_ms) {
  int i = 0;
  MainInst_t *main_inst = (MainInst_t*) inst;
  if (main_inst == NULL) return -1;

  while ((i < max_length) && (i < main_inst->MCUinst.len_waiting_times)) {
    waiting_times_ms[i] = main_inst->MCUinst.waiting_times[i] *
        main_inst->DSPinst.millisecondsPerCall;
    ++i;
  }
  assert(i <= kLenWaitingTimes);
  WebRtcNetEQ_ResetWaitingTimeStats(&main_inst->MCUinst);
  return i;
}

























int WebRtcNetEQ_SetVADInstance(void *NetEQ_inst, void *VAD_inst,
                               WebRtcNetEQ_VADInitFunction initFunction,
                               WebRtcNetEQ_VADSetmodeFunction setmodeFunction,
                               WebRtcNetEQ_VADFunction VADFunction)
{

    
    MainInst_t *NetEqMainInst = (MainInst_t*) NetEQ_inst;
    if (NetEqMainInst == NULL)
    {
        return (-1);
    }

#ifdef NETEQ_VAD

    
    NetEqMainInst->DSPinst.VADInst.VADState = VAD_inst;

    
    NetEqMainInst->DSPinst.VADInst.initFunction = initFunction;
    NetEqMainInst->DSPinst.VADInst.setmodeFunction = setmodeFunction;
    NetEqMainInst->DSPinst.VADInst.VADFunction = VADFunction;

    
    return(WebRtcNetEQ_InitVAD(&NetEqMainInst->DSPinst.VADInst, NetEqMainInst->DSPinst.fs));

#else 
    return (-1);
#endif 

}


















int WebRtcNetEQ_SetVADMode(void *inst, int mode)
{

    
    MainInst_t *NetEqMainInst = (MainInst_t*) inst;
    if (NetEqMainInst == NULL)
    {
        return (-1);
    }

#ifdef NETEQ_VAD

    
    return(WebRtcNetEQ_SetVADModeInternal(&NetEqMainInst->DSPinst.VADInst, mode));

#else 
    return (-1);
#endif 

}
