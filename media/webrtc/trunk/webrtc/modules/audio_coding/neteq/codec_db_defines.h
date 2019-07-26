













#ifndef CODEC_DB_DEFINES_H
#define CODEC_DB_DEFINES_H

#include "typedefs.h"

#define NUM_CODECS 47 /* probably too large with the limited set of supported codecs*/
#define NUM_TOTAL_CODECS	kDecoderReservedEnd




typedef WebRtc_Word16 (*FuncDecode)(void* state, WebRtc_Word16* encoded, WebRtc_Word16 len,
                                    WebRtc_Word16* decoded, WebRtc_Word16* speechType);




typedef WebRtc_Word16 (*FuncDecodePLC)(void* state, WebRtc_Word16* decodec,
                                       WebRtc_Word16 frames);




typedef WebRtc_Word16 (*FuncDecodeInit)(void* state);




typedef WebRtc_Word16
                (*FuncAddLatePkt)(void* state, WebRtc_Word16* encoded, WebRtc_Word16 len);




typedef WebRtc_Word16 (*FuncGetMDinfo)(void* state);





typedef WebRtc_Word16 (*FuncGetPitchInfo)(void* state, WebRtc_Word16* encoded,
                                          WebRtc_Word16* length);




typedef WebRtc_Word16 (*FuncUpdBWEst)(void* state, const WebRtc_UWord16 *encoded,
                                      WebRtc_Word32 packet_size,
                                      WebRtc_UWord16 rtp_seq_number, WebRtc_UWord32 send_ts,
                                      WebRtc_UWord32 arr_ts);





typedef int (*FuncDurationEst)(void* state, const uint8_t* payload,
                               int payload_length_bytes);




typedef WebRtc_Word16 (*FuncGetErrorCode)(void* state);

typedef struct CodecFuncInst_t_
{

    FuncDecode funcDecode;
    FuncDecode funcDecodeRCU;
    FuncDecodePLC funcDecodePLC;
    FuncDecodeInit funcDecodeInit;
    FuncAddLatePkt funcAddLatePkt;
    FuncGetMDinfo funcGetMDinfo;
    FuncUpdBWEst funcUpdBWEst; 
    FuncDurationEst funcDurationEst;
    FuncGetErrorCode funcGetErrorCode;
    void * codec_state;
    WebRtc_UWord16 codec_fs;
    WebRtc_UWord32 timeStamp;

} CodecFuncInst_t;

#endif

