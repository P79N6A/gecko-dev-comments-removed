
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_STRUCTS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_STRUCTS_H_


#include "typedefs.h"
#include "settings.h"
#include "isac.h"

typedef struct Bitstreamstruct {

  WebRtc_UWord8   stream[STREAM_SIZE_MAX];
  WebRtc_UWord32  W_upper;
  WebRtc_UWord32  streamval;
  WebRtc_UWord32  stream_index;

} Bitstr;

typedef struct {

  double    DataBufferLo[WINLEN];
  double    DataBufferHi[WINLEN];

  double    CorrBufLo[ORDERLO+1];
  double    CorrBufHi[ORDERHI+1];

  float    PreStateLoF[ORDERLO+1];
  float    PreStateLoG[ORDERLO+1];
  float    PreStateHiF[ORDERHI+1];
  float    PreStateHiG[ORDERHI+1];
  float    PostStateLoF[ORDERLO+1];
  float    PostStateLoG[ORDERLO+1];
  float    PostStateHiF[ORDERHI+1];
  float    PostStateHiG[ORDERHI+1];

  double    OldEnergy;

} MaskFiltstr;


typedef struct {

  
  double    INSTAT1[2*(QORDER-1)];
  double    INSTAT2[2*(QORDER-1)];
  double    INSTATLA1[2*(QORDER-1)];
  double    INSTATLA2[2*(QORDER-1)];
  double    INLABUF1[QLOOKAHEAD];
  double    INLABUF2[QLOOKAHEAD];

  float    INSTAT1_float[2*(QORDER-1)];
  float    INSTAT2_float[2*(QORDER-1)];
  float    INSTATLA1_float[2*(QORDER-1)];
  float    INSTATLA2_float[2*(QORDER-1)];
  float    INLABUF1_float[QLOOKAHEAD];
  float    INLABUF2_float[QLOOKAHEAD];

  
  double    HPstates[HPORDER];
  float    HPstates_float[HPORDER];

} PreFiltBankstr;


typedef struct {

  
  double    STATE_0_LOWER[2*POSTQORDER];
  double    STATE_0_UPPER[2*POSTQORDER];

  
  double    HPstates1[HPORDER];
  double    HPstates2[HPORDER];

  float    STATE_0_LOWER_float[2*POSTQORDER];
  float    STATE_0_UPPER_float[2*POSTQORDER];

  float    HPstates1_float[HPORDER];
  float    HPstates2_float[HPORDER];

} PostFiltBankstr;

typedef struct {

  
  double    ubuf[PITCH_BUFFSIZE];

  
  double    ystate[PITCH_DAMPORDER];

  
  double    oldlagp[1];
  double    oldgainp[1];

} PitchFiltstr;

typedef struct {

  
  double    buffer[PITCH_WLPCBUFLEN];

  
  double    istate[PITCH_WLPCORDER];
  double    weostate[PITCH_WLPCORDER];
  double    whostate[PITCH_WLPCORDER];

  
  double    window[PITCH_WLPCWINLEN];

} WeightFiltstr;

typedef struct {

  
  double         dec_buffer[PITCH_CORR_LEN2 + PITCH_CORR_STEP2 +
                            PITCH_MAX_LAG/2 - PITCH_FRAME_LEN/2+2];
  double        decimator_state[2*ALLPASSSECTIONS+1];
  double        hp_state[2];

  double        whitened_buf[QLOOKAHEAD];

  double        inbuf[QLOOKAHEAD];

  PitchFiltstr  PFstr_wght;
  PitchFiltstr  PFstr;
  WeightFiltstr Wghtstr;

} PitchAnalysisStruct;




typedef struct {

  
  WebRtc_Word32    prev_frame_length;

  

  WebRtc_Word32    prev_rec_rtp_number;

  
  WebRtc_UWord32    prev_rec_send_ts;

  
  WebRtc_UWord32    prev_rec_arr_ts;

  
  float   prev_rec_rtp_rate;

  
  WebRtc_UWord32    last_update_ts;

  
  WebRtc_UWord32    last_reduction_ts;

  
  WebRtc_Word32    count_tot_updates_rec;

  
  WebRtc_Word32  rec_bw;
  float   rec_bw_inv;
  float   rec_bw_avg;
  float   rec_bw_avg_Q;

  

  float   rec_jitter;
  float   rec_jitter_short_term;
  float   rec_jitter_short_term_abs;
  float   rec_max_delay;
  float   rec_max_delay_avg_Q;

  
  float   rec_header_rate;

  
  float    send_bw_avg;

  

  float   send_max_delay_avg;

  
  int num_pkts_rec;

  int num_consec_rec_pkts_over_30k;

  
  
  int hsn_detect_rec;

  int num_consec_snt_pkts_over_30k;

  
  
  int hsn_detect_snd;

  WebRtc_UWord32 start_wait_period;

  int in_wait_period;

  int change_to_WB;

  WebRtc_UWord32                 senderTimestamp;
  WebRtc_UWord32                 receiverTimestamp;
  
  WebRtc_UWord16                 numConsecLatePkts;
  float                        consecLatency;
  WebRtc_Word16                  inWaitLatePkts;
} BwEstimatorstr;


typedef struct {

  
  int    PrevExceed;
  
  int    ExceedAgo;
  
  int    BurstCounter;
  
  int    InitCounter;
  
  double StillBuffered;

} RateModel;


typedef struct {

  unsigned int SpaceAlloced;
  unsigned int MaxPermAlloced;
  double Tmp0[MAXFFTSIZE];
  double Tmp1[MAXFFTSIZE];
  double Tmp2[MAXFFTSIZE];
  double Tmp3[MAXFFTSIZE];
  int Perm[MAXFFTSIZE];
  int factor [NFACTOR];

} FFTstr;







typedef struct {

  
  int         startIdx;

  
  WebRtc_Word16 framelength;

  
  int         pitchGain_index[2];

  
  double      meanGain[2];
  int         pitchIndex[PITCH_SUBFRAMES*2];

  
  int         LPCindex_s[108*2]; 
  int         LPCindex_g[12*2];  
  double      LPCcoeffs_lo[(ORDERLO+1)*SUBFRAMES*2];
  double      LPCcoeffs_hi[(ORDERHI+1)*SUBFRAMES*2];

  
  WebRtc_Word16 fre[FRAMESAMPLES];
  WebRtc_Word16 fim[FRAMESAMPLES];
  WebRtc_Word16 AvgPitchGain[2];

  
  int         minBytes;

} ISAC_SaveEncData_t;


typedef struct {

  int         indexLPCShape[UB_LPC_ORDER * UB16_LPC_VEC_PER_FRAME];
  double      lpcGain[SUBFRAMES<<1];
  int         lpcGainIndex[SUBFRAMES<<1];

  Bitstr      bitStreamObj;

  WebRtc_Word16 realFFT[FRAMESAMPLES_HALF];
  WebRtc_Word16 imagFFT[FRAMESAMPLES_HALF];
} ISACUBSaveEncDataStruct;



typedef struct {

  Bitstr              bitstr_obj;
  MaskFiltstr         maskfiltstr_obj;
  PreFiltBankstr      prefiltbankstr_obj;
  PitchFiltstr        pitchfiltstr_obj;
  PitchAnalysisStruct pitchanalysisstr_obj;
  FFTstr              fftstr_obj;
  ISAC_SaveEncData_t  SaveEnc_obj;

  int                 buffer_index;
  WebRtc_Word16         current_framesamples;

  float               data_buffer_float[FRAMESAMPLES_30ms];

  int                 frame_nb;
  double              bottleneck;
  WebRtc_Word16         new_framelength;
  double              s2nr;

  
  WebRtc_Word16         payloadLimitBytes30;
  
  WebRtc_Word16         payloadLimitBytes60;
  
  WebRtc_Word16         maxPayloadBytes;
  
  WebRtc_Word16         maxRateInBytes;

  



  WebRtc_Word16         enforceFrameSize;

  






  WebRtc_Word16         lastBWIdx;
} ISACLBEncStruct;

typedef struct {

  Bitstr                  bitstr_obj;
  MaskFiltstr             maskfiltstr_obj;
  PreFiltBankstr          prefiltbankstr_obj;
  FFTstr                  fftstr_obj;
  ISACUBSaveEncDataStruct SaveEnc_obj;

  int                     buffer_index;
  float                   data_buffer_float[MAX_FRAMESAMPLES +
                                            LB_TOTAL_DELAY_SAMPLES];
  double                  bottleneck;
  
  
  
  
  WebRtc_Word16             maxPayloadSizeBytes;

  double                  lastLPCVec[UB_LPC_ORDER];
  WebRtc_Word16             numBytesUsed;
  WebRtc_Word16             lastJitterInfo;
} ISACUBEncStruct;



typedef struct {

  Bitstr          bitstr_obj;
  MaskFiltstr     maskfiltstr_obj;
  PostFiltBankstr postfiltbankstr_obj;
  PitchFiltstr    pitchfiltstr_obj;
  FFTstr          fftstr_obj;

} ISACLBDecStruct;

typedef struct {

  Bitstr          bitstr_obj;
  MaskFiltstr     maskfiltstr_obj;
  PostFiltBankstr postfiltbankstr_obj;
  FFTstr          fftstr_obj;

} ISACUBDecStruct;



typedef struct {

  ISACLBEncStruct ISACencLB_obj;
  ISACLBDecStruct ISACdecLB_obj;
} ISACLBStruct;


typedef struct {

  ISACUBEncStruct ISACencUB_obj;
  ISACUBDecStruct ISACdecUB_obj;
} ISACUBStruct;






typedef struct {
  
  double       loFiltGain[SUBFRAMES];
  double       hiFiltGain[SUBFRAMES];
  
  WebRtc_UWord32 W_upper;
  WebRtc_UWord32 streamval;
  
  WebRtc_UWord32 stream_index;
  WebRtc_UWord8  stream[3];
} transcode_obj;


typedef struct {
  
  ISACLBStruct              instLB;
  
  ISACUBStruct              instUB;

  
  BwEstimatorstr            bwestimator_obj;
  RateModel                 rate_data_obj;
  double                    MaxDelay;

  
  WebRtc_Word16               codingMode;

  
  WebRtc_Word32               bottleneck;

  
  WebRtc_Word32               analysisFBState1[FB_STATE_SIZE_WORD32];
  WebRtc_Word32               analysisFBState2[FB_STATE_SIZE_WORD32];
  WebRtc_Word32               synthesisFBState1[FB_STATE_SIZE_WORD32];
  WebRtc_Word32               synthesisFBState2[FB_STATE_SIZE_WORD32];

  
  WebRtc_Word16               errorCode;

  
  enum ISACBandwidth        bandwidthKHz;
  
  enum IsacSamplingRate encoderSamplingRateKHz;
  enum IsacSamplingRate decoderSamplingRateKHz;
  
  
  WebRtc_Word16               initFlag;

  
  WebRtc_Word16               resetFlag_8kHz;

  
  WebRtc_Word16               maxRateBytesPer30Ms;
  
  WebRtc_Word16               maxPayloadSizeBytes;
  


  WebRtc_UWord16 in_sample_rate_hz;
  
  int16_t state_in_resampler[SIZE_RESAMPLER_STATE];
} ISACMainStruct;

#endif 
