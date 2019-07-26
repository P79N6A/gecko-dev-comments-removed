













#ifndef CODEC_DB_DEFINES_H
#define CODEC_DB_DEFINES_H

#include "typedefs.h"

#define NUM_CODECS 47 /* probably too large with the limited set of supported codecs*/
#define NUM_TOTAL_CODECS	kDecoderReservedEnd




typedef int16_t (*FuncDecode)(void* state, int16_t* encoded, int16_t len,
                                    int16_t* decoded, int16_t* speechType);




typedef int16_t (*FuncDecodePLC)(void* state, int16_t* decodec,
                                       int16_t frames);




typedef int16_t (*FuncDecodeInit)(void* state);




typedef int16_t
                (*FuncAddLatePkt)(void* state, int16_t* encoded, int16_t len);




typedef int16_t (*FuncGetMDinfo)(void* state);





typedef int16_t (*FuncGetPitchInfo)(void* state, int16_t* encoded,
                                          int16_t* length);




typedef int16_t (*FuncUpdBWEst)(void* state, const uint16_t *encoded,
                                      int32_t packet_size,
                                      uint16_t rtp_seq_number, uint32_t send_ts,
                                      uint32_t arr_ts);





typedef int (*FuncDurationEst)(void* state, const uint8_t* payload,
                               int payload_length_bytes);




typedef int16_t (*FuncGetErrorCode)(void* state);

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
    uint16_t codec_fs;
    uint32_t timeStamp;

} CodecFuncInst_t;

#endif

