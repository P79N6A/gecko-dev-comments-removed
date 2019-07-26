
















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






extern const int16_t WebRtcNetEQ_kDownsample8kHzTbl[];
extern const int16_t WebRtcNetEQ_kDownsample16kHzTbl[];
extern const int16_t WebRtcNetEQ_kDownsample32kHzTbl[];
extern const int16_t WebRtcNetEQ_kDownsample48kHzTbl[];
extern const int16_t WebRtcNetEQ_kRandnTbl[];
extern const int16_t WebRtcNetEQ_kMixFractionFuncTbl[];
extern const int16_t WebRtcNetEQ_k1049div[];
extern const int16_t WebRtcNetEQ_k2097div[];
extern const int16_t WebRtcNetEQ_k5243div[];







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

    int32_t w32_energy;
    int32_t w32_energyMax;
    int32_t w32_energyUpdate;
    int32_t w32_energyUpdateLow;
    int16_t pw16_filterState[BGN_LPC_ORDER];
    int16_t pw16_filter[BGN_LPC_ORDER + 1];
    int16_t w16_mutefactor;
    int16_t w16_scale;
    int16_t w16_scaleShift;
    int16_t w16_initialized;
    enum BGNMode bgnMode;

} BGNInst_t;


typedef struct ExpandInst_t_
{

    int16_t w16_overlap; 
    int16_t w16_consecExp; 
    int16_t *pw16_arFilter; 
    int16_t *pw16_arState; 
    int16_t w16_arGain;
    int16_t w16_arGainScale;
    int16_t w16_vFraction; 
    int16_t w16_currentVFraction; 
    int16_t *pw16_expVecs[2];
    int16_t w16_lags[3];
    int16_t w16_maxLag;
    int16_t *pw16_overlapVec; 
    int16_t w16_lagsDirection;
    int16_t w16_lagsPosition;
    int16_t w16_expandMuteFactor; 
    int16_t w16_stopMuting;
    int16_t w16_onset;
    int16_t w16_muteSlope; 

} ExpandInst_t;

#ifdef NETEQ_VAD






typedef int (*VADInitFunction)(void *VAD_inst);
typedef int (*VADSetmodeFunction)(void *VAD_inst, int mode);
typedef int (*VADFunction)(void *VAD_inst, int fs, int16_t *frame,
                           int frameLen);


typedef struct PostDecodeVAD_t_
{

    void *VADState; 

    int16_t VADEnabled; 
    int VADMode; 
    int VADDecision; 
    int16_t SIDintervalCounter; 


    
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

    uint16_t instruction;
    int16_t distLag;
    int16_t corrLag;
    int16_t bestIndex;

    uint32_t endTimestamp;
    uint16_t samplesLeftWithOverlap;

} MasterSlaveInfo;
#endif



typedef struct DSPInst_t_
{

    
    int16_t *pw16_readAddress;
    int16_t *pw16_writeAddress;
    void *main_inst;

    
    int16_t millisecondsPerCall;
    int16_t timestampsPerCall;

    













    int16_t speechBuffer[SPEECH_BUF_SIZE]; 
    int curPosition; 
    int endPosition; 
    uint32_t endTimestamp; 
    uint32_t videoSyncTimestamp; 



    uint16_t fs; 
    int16_t w16_frameLen; 
    int16_t w16_mode; 
    int16_t w16_muteFactor; 
    int16_t *pw16_speechHistory; 
    int16_t w16_speechHistoryLen; 

    
    int16_t w16_seedInc;
    uint32_t uw16_seed;

    
    int16_t w16_concealedTS;

    
    
    

    
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

    
    ActivityStats activity_stats;

#ifdef NETEQ_STEREO
    
    MasterSlaveInfo *msInfo;
#endif

} DSPInst_t;






















int WebRtcNetEQ_DSPInit(DSPInst_t *inst, uint16_t fs);


















int WebRtcNetEQ_AddressInit(DSPInst_t *inst, const void *data2McuAddress,
                            const void *data2DspAddress, const void *mainInst);















int WebRtcNetEQ_ClearInCallStats(DSPInst_t *inst);















int WebRtcNetEQ_ClearPostCallStats(DSPInst_t *inst);














void WebRtcNetEQ_ClearActivityStats(DSPInst_t *inst);
























int WebRtcNetEQ_RecOutInternal(DSPInst_t *inst, int16_t *pw16_outData,
                               int16_t *pw16_len, int16_t BGNonly, int av_sync);























int WebRtcNetEQ_Normal(DSPInst_t *inst,
#ifdef SCRATCH
                       int16_t *pw16_scratchPtr,
#endif
                       int16_t *pw16_decoded, int16_t len,
                       int16_t *pw16_outData, int16_t *pw16_len);
























int WebRtcNetEQ_Expand(DSPInst_t *inst,
#ifdef SCRATCH
                       int16_t *pw16_scratchPtr,
#endif
                       int16_t *pw16_outData, int16_t *pw16_len,
                       int16_t BGNonly);























int WebRtcNetEQ_GenerateBGN(DSPInst_t *inst,
#ifdef SCRATCH
                            int16_t *pw16_scratchPtr,
#endif
                            int16_t *pw16_outData, int16_t len);
































int WebRtcNetEQ_PreEmptiveExpand(DSPInst_t *inst,
#ifdef SCRATCH
                                 int16_t *pw16_scratchPtr,
#endif
                                 const int16_t *pw16_decoded, int len, int oldDataLen,
                                 int16_t *pw16_outData, int16_t *pw16_len,
                                 int16_t BGNonly);




























int WebRtcNetEQ_Accelerate(DSPInst_t *inst,
#ifdef SCRATCH
                           int16_t *pw16_scratchPtr,
#endif
                           const int16_t *pw16_decoded, int len,
                           int16_t *pw16_outData, int16_t *pw16_len,
                           int16_t BGNonly);


























int WebRtcNetEQ_Merge(DSPInst_t *inst,
#ifdef SCRATCH
                      int16_t *pw16_scratchPtr,
#endif
                      int16_t *pw16_decoded, int len, int16_t *pw16_outData,
                      int16_t *pw16_len);

















#ifdef NETEQ_CNG_CODEC


int WebRtcNetEQ_Cng(DSPInst_t *inst, int16_t *pw16_outData, int len);

#endif 
















void WebRtcNetEQ_BGNUpdate(
#ifdef SCRATCH
                           DSPInst_t *inst, int16_t *pw16_scratchPtr
#else
                           DSPInst_t *inst
#endif
                );

#ifdef NETEQ_VAD


















int WebRtcNetEQ_InitVAD(PostDecodeVAD_t *VADInst, uint16_t fs);


















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
























void WebRtcNetEQ_40BitAccCrossCorr(int32_t *crossCorr, int16_t *seq1,
                                   int16_t *seq2, int16_t dimSeq,
                                   int16_t dimCrossCorr, int16_t rShift,
                                   int16_t step_seq2);
















int32_t WebRtcNetEQ_40BitAccDotW16W16(int16_t *vector1, int16_t *vector2,
                                      int len, int scaling);

#endif 

#endif 
