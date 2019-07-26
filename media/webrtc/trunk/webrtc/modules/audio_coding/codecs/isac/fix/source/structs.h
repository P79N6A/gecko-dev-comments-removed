
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_STRUCTS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_STRUCTS_H_


#include "common_audio/signal_processing/include/signal_processing_library.h"
#include "modules/audio_coding/codecs/isac/fix/source/settings.h"
#include "typedefs.h"


typedef struct Bitstreamstruct_dec {

  WebRtc_UWord16  *stream;          
  WebRtc_UWord32  W_upper;          
  WebRtc_UWord32  streamval;
  WebRtc_UWord16  stream_index;     
  WebRtc_Word16   full;             
  

} Bitstr_dec;


typedef struct Bitstreamstruct_enc {

  WebRtc_UWord16  stream[STREAM_MAXW16_60MS];   
  WebRtc_UWord32  W_upper;          
  WebRtc_UWord32  streamval;
  WebRtc_UWord16  stream_index;     
  WebRtc_Word16   full;             
  

} Bitstr_enc;


typedef struct {

  WebRtc_Word16 DataBufferLoQ0[WINLEN];
  WebRtc_Word16 DataBufferHiQ0[WINLEN];

  WebRtc_Word32 CorrBufLoQQ[ORDERLO+1];
  WebRtc_Word32 CorrBufHiQQ[ORDERHI+1];

  WebRtc_Word16 CorrBufLoQdom[ORDERLO+1];
  WebRtc_Word16 CorrBufHiQdom[ORDERHI+1];

  WebRtc_Word32 PreStateLoGQ15[ORDERLO+1];
  WebRtc_Word32 PreStateHiGQ15[ORDERHI+1];

  WebRtc_UWord32 OldEnergy;

} MaskFiltstr_enc;



typedef struct {

  WebRtc_Word16 PostStateLoGQ0[ORDERLO+1];
  WebRtc_Word16 PostStateHiGQ0[ORDERHI+1];

  WebRtc_UWord32 OldEnergy;

} MaskFiltstr_dec;








typedef struct {

  

  WebRtc_Word32 INSTAT1_fix[2*(QORDER-1)];
  WebRtc_Word32 INSTAT2_fix[2*(QORDER-1)];
  WebRtc_Word16 INLABUF1_fix[QLOOKAHEAD];
  WebRtc_Word16 INLABUF2_fix[QLOOKAHEAD];

  
  WebRtc_Word32 HPstates_fix[HPORDER];

} PreFiltBankstr;


typedef struct {

  
  WebRtc_Word32 STATE_0_LOWER_fix[2*POSTQORDER];
  WebRtc_Word32 STATE_0_UPPER_fix[2*POSTQORDER];

  

  WebRtc_Word32 HPstates1_fix[HPORDER];
  WebRtc_Word32 HPstates2_fix[HPORDER];

} PostFiltBankstr;

typedef struct {


  
  WebRtc_Word16 ubufQQ[PITCH_BUFFSIZE];

  
  WebRtc_Word16 ystateQQ[PITCH_DAMPORDER];

  
  WebRtc_Word16 oldlagQ7;
  WebRtc_Word16 oldgainQ12;

} PitchFiltstr;



typedef struct {

  
  WebRtc_Word16   dec_buffer16[PITCH_CORR_LEN2+PITCH_CORR_STEP2+PITCH_MAX_LAG/2-PITCH_FRAME_LEN/2+2];
  WebRtc_Word32   decimator_state32[2*ALLPASSSECTIONS+1];
  WebRtc_Word16   inbuf[QLOOKAHEAD];

  PitchFiltstr  PFstr_wght;
  PitchFiltstr  PFstr;


} PitchAnalysisStruct;


typedef struct {
  

  
  WebRtc_Word16 prevPitchInvIn[FRAMESAMPLES/2];
  WebRtc_Word16 prevPitchInvOut[PITCH_MAX_LAG + 10];            
  WebRtc_Word32 prevHP[PITCH_MAX_LAG + 10];                     


  WebRtc_Word16 decayCoeffPriodic; 
  WebRtc_Word16 decayCoeffNoise;
  WebRtc_Word16 used;       


  WebRtc_Word16 *lastPitchLP;                                  


  
  WebRtc_Word16 lofilt_coefQ15[ ORDERLO ];
  WebRtc_Word16 hifilt_coefQ15[ ORDERHI ];
  WebRtc_Word32 gain_lo_hiQ17[2];

  
  WebRtc_Word16 AvgPitchGain_Q12;
  WebRtc_Word16 lastPitchGain_Q12;
  WebRtc_Word16 lastPitchLag_Q7;

  
  WebRtc_Word16 overlapLP[ RECOVERY_OVERLAP ];                 

  WebRtc_Word16 pitchCycles;
  WebRtc_Word16 A;
  WebRtc_Word16 B;
  WebRtc_Word16 pitchIndex;
  WebRtc_Word16 stretchLag;
  WebRtc_Word16 *prevPitchLP;                                  
  WebRtc_Word16 seed;

  WebRtc_Word16 std;
} PLCstr;




typedef struct {

  WebRtc_Word16   prevFrameSizeMs;      
  WebRtc_UWord16  prevRtpNumber;      
  
  WebRtc_UWord32  prevSendTime;   
  WebRtc_UWord32  prevArrivalTime;      
  WebRtc_UWord16  prevRtpRate;          
  WebRtc_UWord32  lastUpdate;           
  WebRtc_UWord32  lastReduction;        
  WebRtc_Word32   countUpdates;         

  
  WebRtc_UWord32  recBw;
  WebRtc_UWord32  recBwInv;
  WebRtc_UWord32  recBwAvg;
  WebRtc_UWord32  recBwAvgQ;

  WebRtc_UWord32  minBwInv;
  WebRtc_UWord32  maxBwInv;

  
  WebRtc_Word32   recJitter;
  WebRtc_Word32   recJitterShortTerm;
  WebRtc_Word32   recJitterShortTermAbs;
  WebRtc_Word32   recMaxDelay;
  WebRtc_Word32   recMaxDelayAvgQ;


  WebRtc_Word16   recHeaderRate;         

  WebRtc_UWord32  sendBwAvg;           
  WebRtc_Word32   sendMaxDelayAvg;    


  WebRtc_Word16   countRecPkts;          
  WebRtc_Word16   highSpeedRec;        

  

  WebRtc_Word16   countHighSpeedRec;

  
  WebRtc_Word16   inWaitPeriod;

  

  WebRtc_UWord32  startWaitPeriod;

  

  WebRtc_Word16   countHighSpeedSent;

  

  WebRtc_Word16   highSpeedSend;




} BwEstimatorstr;


typedef struct {

  
  WebRtc_Word16    PrevExceed;
  
  WebRtc_Word16    ExceedAgo;
  
  WebRtc_Word16    BurstCounter;
  
  WebRtc_Word16    InitCounter;
  
  WebRtc_Word16    StillBuffered;

} RateModel;






typedef struct {

  
  int     startIdx;

  
  WebRtc_Word16         framelength;

  
  WebRtc_Word16   pitchGain_index[2];

  
  WebRtc_Word32   meanGain[2];
  WebRtc_Word16   pitchIndex[PITCH_SUBFRAMES*2];

  
  WebRtc_Word32         LPCcoeffs_g[12*2]; 
  WebRtc_Word16   LPCindex_s[108*2]; 
  WebRtc_Word16   LPCindex_g[12*2];  

  
  WebRtc_Word16   fre[FRAMESAMPLES];
  WebRtc_Word16   fim[FRAMESAMPLES];
  WebRtc_Word16   AvgPitchGain[2];

  
  int     minBytes;

} ISAC_SaveEncData_t;

typedef struct {

  Bitstr_enc          bitstr_obj;
  MaskFiltstr_enc     maskfiltstr_obj;
  PreFiltBankstr      prefiltbankstr_obj;
  PitchFiltstr        pitchfiltstr_obj;
  PitchAnalysisStruct pitchanalysisstr_obj;
  RateModel           rate_data_obj;

  WebRtc_Word16         buffer_index;
  WebRtc_Word16         current_framesamples;

  WebRtc_Word16      data_buffer_fix[FRAMESAMPLES]; 

  WebRtc_Word16         frame_nb;
  WebRtc_Word16         BottleNeck;
  WebRtc_Word16         MaxDelay;
  WebRtc_Word16         new_framelength;
  WebRtc_Word16         s2nr;
  WebRtc_UWord16        MaxBits;

  WebRtc_Word16         bitstr_seed;
#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  PostFiltBankstr     interpolatorstr_obj;
#endif

  ISAC_SaveEncData_t *SaveEnc_ptr;
  WebRtc_Word16         payloadLimitBytes30; 
  WebRtc_Word16         payloadLimitBytes60; 
  WebRtc_Word16         maxPayloadBytes;     
  WebRtc_Word16         maxRateInBytes;      
  WebRtc_Word16         enforceFrameSize;    

} ISACFIX_EncInst_t;


typedef struct {

  Bitstr_dec          bitstr_obj;
  MaskFiltstr_dec     maskfiltstr_obj;
  PostFiltBankstr     postfiltbankstr_obj;
  PitchFiltstr        pitchfiltstr_obj;
  PLCstr              plcstr_obj;               

#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED
  PreFiltBankstr      decimatorstr_obj;
#endif

} ISACFIX_DecInst_t;



typedef struct {

  ISACFIX_EncInst_t ISACenc_obj;
  ISACFIX_DecInst_t ISACdec_obj;
  BwEstimatorstr     bwestimator_obj;
  WebRtc_Word16         CodingMode;       
  WebRtc_Word16   errorcode;
  WebRtc_Word16   initflag;  
  
} ISACFIX_SubStruct;


typedef struct {
  WebRtc_Word32   lpcGains[12];     
  
  WebRtc_UWord32  W_upper;          
  WebRtc_UWord32  streamval;
  WebRtc_UWord16  stream_index;     
  WebRtc_Word16   full;             
  
  WebRtc_UWord16  beforeLastWord;
  WebRtc_UWord16  lastWord;
} transcode_obj;




#endif  
