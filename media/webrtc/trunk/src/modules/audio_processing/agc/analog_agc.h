









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AGC_MAIN_SOURCE_ANALOG_AGC_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AGC_MAIN_SOURCE_ANALOG_AGC_H_

#include "typedefs.h"
#include "gain_control.h"
#include "digital_agc.h"



#ifdef AGC_DEBUG
#include <stdio.h>
#endif













#define RXX_BUFFER_LEN  10

static const WebRtc_Word16 kMsecSpeechInner = 520;
static const WebRtc_Word16 kMsecSpeechOuter = 340;

static const WebRtc_Word16 kNormalVadThreshold = 400;

static const WebRtc_Word16 kAlphaShortTerm = 6; 
static const WebRtc_Word16 kAlphaLongTerm = 10; 

typedef struct
{
    
    WebRtc_UWord32      fs;                 
    WebRtc_Word16       compressionGaindB;  
    WebRtc_Word16       targetLevelDbfs;    
    WebRtc_Word16       agcMode;            
    WebRtc_UWord8       limiterEnable;      
    WebRtcAgc_config_t  defaultConfig;
    WebRtcAgc_config_t  usedConfig;

    
    WebRtc_Word16       initFlag;
    WebRtc_Word16       lastError;

    
    
    WebRtc_Word32       analogTargetLevel;  
    WebRtc_Word32       startUpperLimit;    
    WebRtc_Word32       startLowerLimit;    
    WebRtc_Word32       upperPrimaryLimit;  
    WebRtc_Word32       lowerPrimaryLimit;  
    WebRtc_Word32       upperSecondaryLimit;
    WebRtc_Word32       lowerSecondaryLimit;
    WebRtc_UWord16      targetIdx;          
#ifdef MIC_LEVEL_FEEDBACK
    WebRtc_UWord16      targetIdxOffset;    
#endif
    WebRtc_Word16       analogTarget;       

    
    WebRtc_Word32       filterState[8];     
    WebRtc_Word32       upperLimit;         
    WebRtc_Word32       lowerLimit;         
    WebRtc_Word32       Rxx160w32;          
    WebRtc_Word32       Rxx16_LPw32;        
    WebRtc_Word32       Rxx160_LPw32;       
    WebRtc_Word32       Rxx16_LPw32Max;     
    WebRtc_Word32       Rxx16_vectorw32[RXX_BUFFER_LEN];
    WebRtc_Word32       Rxx16w32_array[2][5];
    WebRtc_Word32       env[2][10];         

    WebRtc_Word16       Rxx16pos;           
    WebRtc_Word16       envSum;             
    WebRtc_Word16       vadThreshold;       
    WebRtc_Word16       inActive;           
    WebRtc_Word16       msTooLow;           
    WebRtc_Word16       msTooHigh;          
    WebRtc_Word16       changeToSlowMode;   
    WebRtc_Word16       firstCall;          
    WebRtc_Word16       msZero;             
    WebRtc_Word16       msecSpeechOuterChange;
    WebRtc_Word16       msecSpeechInnerChange;
    WebRtc_Word16       activeSpeech;       
    WebRtc_Word16       muteGuardMs;        
    WebRtc_Word16       inQueue;            

    
    WebRtc_Word32       micRef;             
    WebRtc_UWord16      gainTableIdx;       
    WebRtc_Word32       micGainIdx;         
    WebRtc_Word32       micVol;             
    WebRtc_Word32       maxLevel;           
    WebRtc_Word32       maxAnalog;          
    WebRtc_Word32       maxInit;            
    WebRtc_Word32       minLevel;           
    WebRtc_Word32       minOutput;          
    WebRtc_Word32       zeroCtrlMax;        

    WebRtc_Word16       scale;              
#ifdef MIC_LEVEL_FEEDBACK
    WebRtc_Word16       numBlocksMicLvlSat;
    WebRtc_UWord8 micLvlSat;
#endif
    
    AgcVad_t            vadMic;
    DigitalAgc_t        digitalAgc;

#ifdef AGC_DEBUG
    FILE*               fpt;
    FILE*               agcLog;
    WebRtc_Word32       fcount;
#endif

    WebRtc_Word16       lowLevelSignal;
} Agc_t;

#endif 
