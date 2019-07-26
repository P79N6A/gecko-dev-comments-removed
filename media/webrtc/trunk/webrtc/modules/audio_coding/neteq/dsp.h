
















#ifndef DSP_H
#define DSP_H

#include "typedefs.h"

#include "webrtc_cng.h"

#include "codec_db_defines.h"
#include "neteq_defines.h"
#include "neteq_statistics.h"

#ifdef NETEQ_ATEVENT_DECODE
#include "dtmf_tonegen.h"
#endif








#if defined(NETEQ_48KHZ_WIDEBAND)
	#define FSMULT	6
#elif defined(NETEQ_32KHZ_WIDEBAND)
	#define FSMULT	4
#elif defined(NETEQ_WIDEBAND)
	#define FSMULT 2
#else
	#define FSMULT 1
#endif



#define SPEECH_BUF_SIZE (565 * FSMULT)


#define BGN_LPC_ORDER				(4 + FSMULT)  /* 5, 6, 8, or 10 */
#define UNVOICED_LPC_ORDER			6
#define RANDVEC_NO_OF_SAMPLES		256



#define DEFAULT_TIME_ADJUST 8


#define POST_DECODE_VAD_AUTO_ENABLE 3000  


#define NETEQ_OVERLAP_WINMUTE_8KHZ_START	27307
#define NETEQ_OVERLAP_WINMUTE_8KHZ_INC		-5461
#define NETEQ_OVERLAP_WINUNMUTE_8KHZ_START	 5461
#define NETEQ_OVERLAP_WINUNMUTE_8KHZ_INC	 5461

#define NETEQ_OVERLAP_WINMUTE_16KHZ_START	29789
#define NETEQ_OVERLAP_WINMUTE_16KHZ_INC		-2979
#define NETEQ_OVERLAP_WINUNMUTE_16KHZ_START	 2979
#define NETEQ_OVERLAP_WINUNMUTE_16KHZ_INC	 2979

#define NETEQ_OVERLAP_WINMUTE_32KHZ_START	31208
#define NETEQ_OVERLAP_WINMUTE_32KHZ_INC		-1560
#define NETEQ_OVERLAP_WINUNMUTE_32KHZ_START	 1560
#define NETEQ_OVERLAP_WINUNMUTE_32KHZ_INC	 1560

#define NETEQ_OVERLAP_WINMUTE_48KHZ_START	31711
#define NETEQ_OVERLAP_WINMUTE_48KHZ_INC		-1057
#define NETEQ_OVERLAP_WINUNMUTE_48KHZ_START	 1057
#define NETEQ_OVERLAP_WINUNMUTE_48KHZ_INC	 1057


#define FADE_BGN_TIME 200






extern const WebRtc_Word16 WebRtcNetEQ_kDownsample8kHzTbl[];
extern const WebRtc_Word16 WebRtcNetEQ_kDownsample16kHzTbl[];
extern const WebRtc_Word16 WebRtcNetEQ_kDownsample32kHzTbl[];
extern const WebRtc_Word16 WebRtcNetEQ_kDownsample48kHzTbl[];
extern const WebRtc_Word16 WebRtcNetEQ_kRandnTbl[];
extern const WebRtc_Word16 WebRtcNetEQ_kMixFractionFuncTbl[];
extern const WebRtc_Word16 WebRtcNetEQ_k1049div[];
extern const WebRtc_Word16 WebRtcNetEQ_k2097div[];
extern const WebRtc_Word16 WebRtcNetEQ_k5243div[];







enum BGNMode
{
    BGN_ON,     
    BGN_FADE,   
    BGN_OFF     
};

#ifdef NETEQ_STEREO
enum MasterSlaveMode
{
    NETEQ_MONO,     
    NETEQ_MASTER,   
    NETEQ_SLAVE     
};

enum MasterSlaveExtraInfo
{
    NO_INFO,        
    ACC_FAIL,       
    PE_EXP_FAIL,    
    DTMF_OVERDUB,   
    DTMF_ONLY       
};
#endif







typedef struct BGNInst_t_
{

    WebRtc_Word32 w32_energy;
    WebRtc_Word32 w32_energyMax;
    WebRtc_Word32 w32_energyUpdate;
    WebRtc_Word32 w32_energyUpdateLow;
    WebRtc_Word16 pw16_filterState[BGN_LPC_ORDER];
    WebRtc_Word16 pw16_filter[BGN_LPC_ORDER + 1];
    WebRtc_Word16 w16_mutefactor;
    WebRtc_Word16 w16_scale;
    WebRtc_Word16 w16_scaleShift;
    WebRtc_Word16 w16_initialized;
    enum BGNMode bgnMode;

} BGNInst_t;


typedef struct ExpandInst_t_
{

    WebRtc_Word16 w16_overlap; 
    WebRtc_Word16 w16_consecExp; 
    WebRtc_Word16 *pw16_arFilter; 
    WebRtc_Word16 *pw16_arState; 
    WebRtc_Word16 w16_arGain;
    WebRtc_Word16 w16_arGainScale;
    WebRtc_Word16 w16_vFraction; 
    WebRtc_Word16 w16_currentVFraction; 
    WebRtc_Word16 *pw16_expVecs[2];
    WebRtc_Word16 w16_lags[3];
    WebRtc_Word16 w16_maxLag;
    WebRtc_Word16 *pw16_overlapVec; 
    WebRtc_Word16 w16_lagsDirection;
    WebRtc_Word16 w16_lagsPosition;
    WebRtc_Word16 w16_expandMuteFactor; 
    WebRtc_Word16 w16_stopMuting;
    WebRtc_Word16 w16_onset;
    WebRtc_Word16 w16_muteSlope; 

} ExpandInst_t;

#ifdef NETEQ_VAD






typedef int (*VADInitFunction)(void *VAD_inst);
typedef int (*VADSetmodeFunction)(void *VAD_inst, int mode);
typedef int (*VADFunction)(void *VAD_inst, int fs, WebRtc_Word16 *frame,
                           int frameLen);


typedef struct PostDecodeVAD_t_
{

    void *VADState; 

    WebRtc_Word16 VADEnabled; 
    int VADMode; 
    int VADDecision; 
    WebRtc_Word16 SIDintervalCounter; 


    
    VADInitFunction initFunction; 
    VADSetmodeFunction setmodeFunction; 
    VADFunction VADFunction; 

} PostDecodeVAD_t;

#endif 

#ifdef NETEQ_STEREO
#define MAX_MS_DECODES 10

typedef struct 
{
    
    enum MasterSlaveMode    msMode;

    enum MasterSlaveExtraInfo  extraInfo;

    WebRtc_UWord16 instruction;
    WebRtc_Word16 distLag;
    WebRtc_Word16 corrLag;
    WebRtc_Word16 bestIndex;

    WebRtc_UWord32 endTimestamp;
    WebRtc_UWord16 samplesLeftWithOverlap;

} MasterSlaveInfo;
#endif



typedef struct DSPInst_t_
{

    
    WebRtc_Word16 *pw16_readAddress;
    WebRtc_Word16 *pw16_writeAddress;
    void *main_inst;

    
    WebRtc_Word16 millisecondsPerCall;
    WebRtc_Word16 timestampsPerCall;

    













    WebRtc_Word16 speechBuffer[SPEECH_BUF_SIZE]; 
    int curPosition; 
    int endPosition; 
    WebRtc_UWord32 endTimestamp; 
    WebRtc_UWord32 videoSyncTimestamp; 



    WebRtc_UWord16 fs; 
    WebRtc_Word16 w16_frameLen; 
    WebRtc_Word16 w16_mode; 
    WebRtc_Word16 w16_muteFactor; 
    WebRtc_Word16 *pw16_speechHistory; 
    WebRtc_Word16 w16_speechHistoryLen; 

    
    WebRtc_Word16 w16_seedInc;
    WebRtc_UWord32 uw16_seed;

    
    WebRtc_Word16 w16_concealedTS;

    
    
    

    
    CodecFuncInst_t codec_ptr_inst;

#ifdef NETEQ_CNG_CODEC
    
    CNG_dec_inst *CNG_Codec_inst;
#endif 

#ifdef NETEQ_ATEVENT_DECODE
    
    dtmf_tone_inst_t DTMFInst;
#endif 

#ifdef NETEQ_VAD
    
    PostDecodeVAD_t VADInst;
#endif 

    
    ExpandInst_t ExpandInst;

    
    BGNInst_t BGNInst;

    
    DSPStats_t statInst;

#ifdef NETEQ_STEREO
    
    MasterSlaveInfo *msInfo;
#endif

} DSPInst_t;






















int WebRtcNetEQ_DSPInit(DSPInst_t *inst, WebRtc_UWord16 fs);


















int WebRtcNetEQ_AddressInit(DSPInst_t *inst, const void *data2McuAddress,
                            const void *data2DspAddress, const void *mainInst);















int WebRtcNetEQ_ClearInCallStats(DSPInst_t *inst);















int WebRtcNetEQ_ClearPostCallStats(DSPInst_t *inst);























int WebRtcNetEQ_RecOutInternal(DSPInst_t *inst, WebRtc_Word16 *pw16_outData, WebRtc_Word16 *pw16_len,
                       WebRtc_Word16 BGNonly);























int WebRtcNetEQ_Normal(DSPInst_t *inst,
#ifdef SCRATCH
                       WebRtc_Word16 *pw16_scratchPtr,
#endif
                       WebRtc_Word16 *pw16_decoded, WebRtc_Word16 len,
                       WebRtc_Word16 *pw16_outData, WebRtc_Word16 *pw16_len);
























int WebRtcNetEQ_Expand(DSPInst_t *inst,
#ifdef SCRATCH
                       WebRtc_Word16 *pw16_scratchPtr,
#endif
                       WebRtc_Word16 *pw16_outData, WebRtc_Word16 *pw16_len,
                       WebRtc_Word16 BGNonly);























int WebRtcNetEQ_GenerateBGN(DSPInst_t *inst,
#ifdef SCRATCH
                            WebRtc_Word16 *pw16_scratchPtr,
#endif
                            WebRtc_Word16 *pw16_outData, WebRtc_Word16 len);
































int WebRtcNetEQ_PreEmptiveExpand(DSPInst_t *inst,
#ifdef SCRATCH
                                 WebRtc_Word16 *pw16_scratchPtr,
#endif
                                 const WebRtc_Word16 *pw16_decoded, int len, int oldDataLen,
                                 WebRtc_Word16 *pw16_outData, WebRtc_Word16 *pw16_len,
                                 WebRtc_Word16 BGNonly);




























int WebRtcNetEQ_Accelerate(DSPInst_t *inst,
#ifdef SCRATCH
                           WebRtc_Word16 *pw16_scratchPtr,
#endif
                           const WebRtc_Word16 *pw16_decoded, int len,
                           WebRtc_Word16 *pw16_outData, WebRtc_Word16 *pw16_len,
                           WebRtc_Word16 BGNonly);


























int WebRtcNetEQ_Merge(DSPInst_t *inst,
#ifdef SCRATCH
                      WebRtc_Word16 *pw16_scratchPtr,
#endif
                      WebRtc_Word16 *pw16_decoded, int len, WebRtc_Word16 *pw16_outData,
                      WebRtc_Word16 *pw16_len);

















#ifdef NETEQ_CNG_CODEC


int WebRtcNetEQ_Cng(DSPInst_t *inst, WebRtc_Word16 *pw16_outData, int len);

#endif 
















void WebRtcNetEQ_BGNUpdate(
#ifdef SCRATCH
                           DSPInst_t *inst, WebRtc_Word16 *pw16_scratchPtr
#else
                           DSPInst_t *inst
#endif
                );

#ifdef NETEQ_VAD


















int WebRtcNetEQ_InitVAD(PostDecodeVAD_t *VADInst, WebRtc_UWord16 fs);


















int WebRtcNetEQ_SetVADModeInternal(PostDecodeVAD_t *VADInst, int mode);

#endif 
















int WebRtcNetEQ_FlushSpeechBuffer(DSPInst_t *inst);

#ifndef WEBRTC_NETEQ_40BITACC_TEST

#include "signal_processing_library.h"

#define WebRtcNetEQ_CrossCorr   WebRtcSpl_CrossCorrelation
#define WebRtcNetEQ_DotW16W16   WebRtcSpl_DotProductWithScale

#else 





#define WebRtcNetEQ_CrossCorr		WebRtcNetEQ_40BitAccCrossCorr
#define WebRtcNetEQ_DotW16W16	    WebRtcNetEQ_40BitAccDotW16W16
























void WebRtcNetEQ_40BitAccCrossCorr(WebRtc_Word32 *crossCorr, WebRtc_Word16 *seq1,
                                   WebRtc_Word16 *seq2, WebRtc_Word16 dimSeq,
                                   WebRtc_Word16 dimCrossCorr, WebRtc_Word16 rShift,
                                   WebRtc_Word16 step_seq2);
















WebRtc_Word32 WebRtcNetEQ_40BitAccDotW16W16(WebRtc_Word16 *vector1, WebRtc_Word16 *vector2,
                                            int len, int scaling);

#endif 

#endif 
