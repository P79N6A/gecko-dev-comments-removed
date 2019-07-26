














#include "dsp.h"

#include "signal_processing_library.h"

#include "neteq_error_codes.h"






const int16_t WebRtcNetEQ_kDownsample8kHzTbl[] = { 1229, 1638, 1229 };

#ifdef NETEQ_WIDEBAND

const int16_t WebRtcNetEQ_kDownsample16kHzTbl[] =
{   614, 819, 1229, 819, 614};
#endif

#ifdef NETEQ_32KHZ_WIDEBAND

const int16_t WebRtcNetEQ_kDownsample32kHzTbl[] =
{   584, 512, 625, 667, 625, 512, 584};
#endif

#ifdef NETEQ_48KHZ_WIDEBAND

const int16_t WebRtcNetEQ_kDownsample48kHzTbl[] =
{   1019, 390, 427, 440, 427, 390, 1019};
#endif




const int16_t WebRtcNetEQ_kMixFractionFuncTbl[4] = { -5179, 19931, -16422, 5776 };



const int16_t WebRtcNetEQ_k1049div[7] = { 0, 1049, 524, 349, 262, 209, 174 };


const int16_t WebRtcNetEQ_k2097div[7] = { 0, 2097, 1048, 699, 524, 419, 349 };


const int16_t WebRtcNetEQ_k5243div[7] = { 0, 5243, 2621, 1747, 1310, 1048, 873 };

#ifdef WEBRTC_NETEQ_40BITACC_TEST





























void WebRtcNetEQ_40BitAccCrossCorr(int32_t *crossCorr,
    int16_t *seq1,
    int16_t *seq2,
    int16_t dimSeq,
    int16_t dimCrossCorr,
    int16_t rShift,
    int16_t step_seq2)
{
    int i, j;
    int16_t *seq1Ptr, *seq2Ptr;
    int64_t acc;

    for (i = 0; i < dimCrossCorr; i++)
    {
        

        seq1Ptr = seq1;
        seq2Ptr = seq2 + (step_seq2 * i);
        acc = 0;

        
        for (j = 0; j < dimSeq; j++)
        {
            acc += WEBRTC_SPL_MUL_16_16((*seq1Ptr), (*seq2Ptr));
            seq1Ptr++;
            seq2Ptr++;
        }

        (*crossCorr) = (int32_t) (acc >> rShift);
        crossCorr++;
    }
}
















int32_t WebRtcNetEQ_40BitAccDotW16W16(int16_t *vector1,
    int16_t *vector2,
    int len,
    int scaling)
{
    int32_t sum;
    int i;
    int64_t acc;

    acc = 0;
    for (i = 0; i < len; i++)
    {
        acc += WEBRTC_SPL_MUL_16_16(*vector1++, *vector2++);
    }

    sum = (int32_t) (acc >> scaling);

    return(sum);
}

#endif 

















int WebRtcNetEQ_DSPInit(DSPInst_t *inst, uint16_t fs)
{

    int res = 0;
    int16_t fs_mult;

    
#ifdef NETEQ_CNG_CODEC
    void *savedPtr1 = inst->CNG_Codec_inst;
#endif
    void *savedPtr2 = inst->pw16_readAddress;
    void *savedPtr3 = inst->pw16_writeAddress;
    void *savedPtr4 = inst->main_inst;
#ifdef NETEQ_VAD
    void *savedVADptr = inst->VADInst.VADState;
    VADInitFunction savedVADinit = inst->VADInst.initFunction;
    VADSetmodeFunction savedVADsetmode = inst->VADInst.setmodeFunction;
    VADFunction savedVADfunc = inst->VADInst.VADFunction;
    int16_t savedVADEnabled = inst->VADInst.VADEnabled;
    int savedVADMode = inst->VADInst.VADMode;
#endif 
    DSPStats_t saveStats;
    int16_t saveMsPerCall = inst->millisecondsPerCall;
    enum BGNMode saveBgnMode = inst->BGNInst.bgnMode;
#ifdef NETEQ_STEREO
    MasterSlaveInfo saveMSinfo;
#endif

    WEBRTC_SPL_MEMCPY_W16(&saveStats, &(inst->statInst),
        sizeof(DSPStats_t)/sizeof(int16_t));

#ifdef NETEQ_STEREO
    WEBRTC_SPL_MEMCPY_W16(&saveMSinfo, &(inst->msInfo),
        sizeof(MasterSlaveInfo)/sizeof(int16_t));
#endif

    
    if ((fs != 8000)
#ifdef NETEQ_WIDEBAND
    &&(fs!=16000)
#endif
#ifdef NETEQ_32KHZ_WIDEBAND
    &&(fs!=32000)
#endif
#ifdef NETEQ_48KHZ_WIDEBAND
    &&(fs!=48000)
#endif
    )
    {
        
        return (CODEC_DB_UNSUPPORTED_FS);
    }

    
    fs_mult = WebRtcSpl_DivW32W16ResW16(fs, 8000);

    
    WebRtcSpl_MemSetW16((int16_t *) inst, 0, sizeof(DSPInst_t) / sizeof(int16_t));

    
#ifdef NETEQ_CNG_CODEC
    inst->CNG_Codec_inst = (CNG_dec_inst *)savedPtr1;
#endif
    inst->pw16_readAddress = (int16_t *) savedPtr2;
    inst->pw16_writeAddress = (int16_t *) savedPtr3;
    inst->main_inst = savedPtr4;
#ifdef NETEQ_VAD
    inst->VADInst.VADState = savedVADptr;
    inst->VADInst.initFunction = savedVADinit;
    inst->VADInst.setmodeFunction = savedVADsetmode;
    inst->VADInst.VADFunction = savedVADfunc;
    inst->VADInst.VADEnabled = savedVADEnabled;
    inst->VADInst.VADMode = savedVADMode;
#endif 

    
    inst->fs = fs;
    inst->millisecondsPerCall = saveMsPerCall;
    inst->timestampsPerCall = inst->millisecondsPerCall * 8 * fs_mult;
    inst->ExpandInst.w16_overlap = 5 * fs_mult;
    inst->endPosition = 565 * fs_mult;
    inst->curPosition = inst->endPosition - inst->ExpandInst.w16_overlap;
    inst->w16_seedInc = 1;
    inst->uw16_seed = 777;
    inst->w16_muteFactor = 16384; 
    inst->w16_frameLen = 3 * inst->timestampsPerCall; 

    inst->w16_speechHistoryLen = 256 * fs_mult;
    inst->pw16_speechHistory = &inst->speechBuffer[inst->endPosition
        - inst->w16_speechHistoryLen];
    inst->ExpandInst.pw16_overlapVec = &(inst->pw16_speechHistory[inst->w16_speechHistoryLen
        - inst->ExpandInst.w16_overlap]);

    
    inst->ExpandInst.pw16_expVecs[0] = &inst->speechBuffer[0];
    inst->ExpandInst.pw16_expVecs[1] = &inst->speechBuffer[126 * fs_mult];
    inst->ExpandInst.pw16_arState = &inst->speechBuffer[2 * 126 * fs_mult];
    inst->ExpandInst.pw16_arFilter = &inst->speechBuffer[2 * 126 * fs_mult
        + UNVOICED_LPC_ORDER];
    

    inst->ExpandInst.w16_expandMuteFactor = 16384; 

    
    inst->BGNInst.pw16_filter[0] = 4096;
    inst->BGNInst.w16_scale = 20000;
    inst->BGNInst.w16_scaleShift = 24;
    inst->BGNInst.w32_energyUpdate = 500000;
    inst->BGNInst.w32_energyUpdateLow = 0;
    inst->BGNInst.w32_energy = 2500;
    inst->BGNInst.w16_initialized = 0;
    inst->BGNInst.bgnMode = saveBgnMode;

    WEBRTC_SPL_MEMCPY_W16(&(inst->statInst), &saveStats,
        sizeof(DSPStats_t)/sizeof(int16_t));

#ifdef NETEQ_STEREO
    WEBRTC_SPL_MEMCPY_W16(&(inst->msInfo), &saveMSinfo,
        sizeof(MasterSlaveInfo)/sizeof(int16_t));
#endif

#ifdef NETEQ_CNG_CODEC
    if (inst->CNG_Codec_inst!=NULL)
    {
        
        res |= WebRtcCng_InitDec(inst->CNG_Codec_inst);
    }
#endif

#ifdef NETEQ_VAD
    

    res |= WebRtcNetEQ_InitVAD(&inst->VADInst, fs);
#endif 

    return (res);
}


















int WebRtcNetEQ_AddressInit(DSPInst_t *inst, const void *data2McuAddress,
                            const void *data2DspAddress, const void *mainInst)
{

    
    inst->pw16_readAddress = (int16_t *) data2DspAddress;
    inst->pw16_writeAddress = (int16_t *) data2McuAddress;

    
    inst->main_inst = (void *) mainInst;

    
    inst->millisecondsPerCall = 10;
    inst->timestampsPerCall = 80;

    return (0);

}















int WebRtcNetEQ_ClearInCallStats(DSPInst_t *inst)
{
    
    inst->statInst.accelerateLength = 0;
    inst->statInst.expandLength = 0;
    inst->statInst.preemptiveLength = 0;
    inst->statInst.addedSamples = 0;
    return (0);
}















int WebRtcNetEQ_ClearPostCallStats(DSPInst_t *inst)
{

    
    inst->statInst.expandedVoiceSamples = 0;
    inst->statInst.expandedNoiseSamples = 0;
    return (0);
}














void WebRtcNetEQ_ClearActivityStats(DSPInst_t *inst) {
  memset(&inst->activity_stats, 0, sizeof(ActivityStats));
}

#ifdef NETEQ_VAD

















int WebRtcNetEQ_InitVAD(PostDecodeVAD_t *VADInst, uint16_t fs)
{

    int res = 0;

    
    VADInst->VADEnabled = 0;

    if (VADInst->VADState != NULL 
        && VADInst->initFunction != NULL 
        && VADInst->setmodeFunction != NULL 
        && VADInst->VADFunction != NULL) 
    {
        res = VADInst->initFunction( VADInst->VADState ); 
        res |= WebRtcNetEQ_SetVADModeInternal( VADInst, VADInst->VADMode );

        if (res!=0)
        {
            
            VADInst->VADState = NULL;
        }
        else if (fs<=16000)
        {
            
            VADInst->VADEnabled = 1;
        }
    }

    
    VADInst->SIDintervalCounter = 0;

    
    VADInst->VADDecision = 1;

    return(res);

}


















int WebRtcNetEQ_SetVADModeInternal(PostDecodeVAD_t *VADInst, int mode)
{

    int res = 0;

    VADInst->VADMode = mode;

    if (VADInst->VADState != NULL)
    {
        
        res = VADInst->setmodeFunction(VADInst->VADState, mode);
    }

    return(res);

}

#endif 
















int WebRtcNetEQ_FlushSpeechBuffer(DSPInst_t *inst)
{
    int16_t fs_mult;

    
    fs_mult = WebRtcSpl_DivW32W16ResW16(inst->fs, 8000);

    
    WebRtcSpl_MemSetW16(inst->speechBuffer, 0, SPEECH_BUF_SIZE);
    inst->endPosition = 565 * fs_mult;
    inst->curPosition = inst->endPosition - inst->ExpandInst.w16_overlap;

    return 0;
}

