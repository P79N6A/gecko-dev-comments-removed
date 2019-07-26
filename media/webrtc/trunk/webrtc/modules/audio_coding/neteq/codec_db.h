













#ifndef CODEC_DB_H
#define CODEC_DB_H

#include "typedefs.h"

#include "webrtc_neteq.h"
#include "codec_db_defines.h"
#include "neteq_defines.h"

#if defined(NETEQ_48KHZ_WIDEBAND)
    #define NUM_CNG_CODECS 4
#elif defined(NETEQ_32KHZ_WIDEBAND)
    #define NUM_CNG_CODECS 3
#elif defined(NETEQ_WIDEBAND)
    #define NUM_CNG_CODECS 2
#else
    #define NUM_CNG_CODECS 1
#endif

typedef struct
{

    WebRtc_Word16 position[NUM_TOTAL_CODECS];
    WebRtc_Word16 nrOfCodecs;

    WebRtc_Word16 payloadType[NUM_CODECS];
    FuncDecode funcDecode[NUM_CODECS];
    FuncDecode funcDecodeRCU[NUM_CODECS];
    FuncDecodePLC funcDecodePLC[NUM_CODECS];
    FuncDecodeInit funcDecodeInit[NUM_CODECS];
    FuncAddLatePkt funcAddLatePkt[NUM_CODECS];
    FuncGetMDinfo funcGetMDinfo[NUM_CODECS];
    FuncGetPitchInfo funcGetPitch[NUM_CODECS];
    FuncUpdBWEst funcUpdBWEst[NUM_CODECS];
    FuncDurationEst funcDurationEst[NUM_CODECS];
    FuncGetErrorCode funcGetErrorCode[NUM_CODECS];
    void * codec_state[NUM_CODECS];
    WebRtc_UWord16 codec_fs[NUM_CODECS];
    WebRtc_Word16 CNGpayloadType[NUM_CNG_CODECS];

} CodecDbInst_t;

#define NO_SPLIT -1 /* codec payload cannot be split */

typedef struct
{
    WebRtc_Word16 deltaBytes;
    WebRtc_Word16 deltaTime;
} SplitInfo_t;




int WebRtcNetEQ_DbReset(CodecDbInst_t *inst);




int WebRtcNetEQ_DbAdd(CodecDbInst_t *inst, enum WebRtcNetEQDecoder codec,
                      WebRtc_Word16 payloadType, FuncDecode funcDecode,
                      FuncDecode funcDecodeRCU, FuncDecodePLC funcDecodePLC,
                      FuncDecodeInit funcDecodeInit, FuncAddLatePkt funcAddLatePkt,
                      FuncGetMDinfo funcGetMDinfo, FuncGetPitchInfo funcGetPitch,
                      FuncUpdBWEst funcUpdBWEst, FuncDurationEst funcDurationEst,
                      FuncGetErrorCode funcGetErrorCode, void* codec_state,
                      WebRtc_UWord16 codec_fs);




int WebRtcNetEQ_DbRemove(CodecDbInst_t *inst, enum WebRtcNetEQDecoder codec);




int WebRtcNetEQ_DbGetPtrs(CodecDbInst_t *inst, enum WebRtcNetEQDecoder,
                          CodecFuncInst_t *ptr_inst);





int WebRtcNetEQ_DbGetPayload(CodecDbInst_t *inst, enum WebRtcNetEQDecoder codecID);





int WebRtcNetEQ_DbGetCodec(const CodecDbInst_t *inst, int payloadType);





int WebRtcNetEQ_DbGetSplitInfo(SplitInfo_t *inst, enum WebRtcNetEQDecoder codecID,
                               int codedsize);




int WebRtcNetEQ_DbIsMDCodec(enum WebRtcNetEQDecoder codecID);




int WebRtcNetEQ_DbIsCNGPayload(const CodecDbInst_t *inst, int payloadType);




WebRtc_UWord16 WebRtcNetEQ_DbGetSampleRate(CodecDbInst_t *inst, int payloadType);

#endif

