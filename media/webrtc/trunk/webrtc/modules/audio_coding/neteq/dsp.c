














#include "dsp.h"

#include "signal_processing_library.h"

#include "neteq_error_codes.h"






const WebRtc_Word16 WebRtcNetEQ_kDownsample8kHzTbl[] = { 1229, 1638, 1229 };

#ifdef NETEQ_WIDEBAND

const WebRtc_Word16 WebRtcNetEQ_kDownsample16kHzTbl[] =
{   614, 819, 1229, 819, 614};
#endif

#ifdef NETEQ_32KHZ_WIDEBAND

const WebRtc_Word16 WebRtcNetEQ_kDownsample32kHzTbl[] =
{   584, 512, 625, 667, 625, 512, 584};
#endif

#ifdef NETEQ_48KHZ_WIDEBAND

const WebRtc_Word16 WebRtcNetEQ_kDownsample48kHzTbl[] =
{   1019, 390, 427, 440, 427, 390, 1019};
#endif




const WebRtc_Word16 WebRtcNetEQ_kMixFractionFuncTbl[4] = { -5179, 19931, -16422, 5776 };



const WebRtc_Word16 WebRtcNetEQ_k1049div[7] = { 0, 1049, 524, 349, 262, 209, 174 };


const WebRtc_Word16 WebRtcNetEQ_k2097div[7] = { 0, 2097, 1048, 699, 524, 419, 349 };


const WebRtc_Word16 WebRtcNetEQ_k5243div[7] = { 0, 5243, 2621, 1747, 1310, 1048, 873 };

#ifdef WEBRTC_NETEQ_40BITACC_TEST





























void WebRtcNetEQ_40BitAccCrossCorr(WebRtc_Word32 *crossCorr,
    WebRtc_Word16 *seq1,
    WebRtc_Word16 *seq2,
    WebRtc_Word16 dimSeq,
    WebRtc_Word16 dimCrossCorr,
    WebRtc_Word16 rShift,
    WebRtc_Word16 step_seq2)
{
    int i, j;
    WebRtc_Word16 *seq1Ptr, *seq2Ptr;
    WebRtc_Word64 acc;

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

        (*crossCorr) = (WebRtc_Word32) (acc >> rShift);
        crossCorr++;
    }
}
















WebRtc_Word32 WebRtcNetEQ_40BitAccDotW16W16(WebRtc_Word16 *vector1,
    WebRtc_Word16 *vector2,
    int len,
    int scaling)
{
    WebRtc_Word32 sum;
    int i;
    WebRtc_Word64 acc;

    acc = 0;
    for (i = 0; i < len; i++)
    {
        acc += WEBRTC_SPL_MUL_16_16(*vector1++, *vector2++);
    }

    sum = (WebRtc_Word32) (acc >> scaling);

    return(sum);
}

#endif 

















int WebRtcNetEQ_DSPInit(DSPInst_t *inst, WebRtc_UWord16 fs)
{

    int res = 0;
    WebRtc_Word16 fs_mult;

    
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
    WebRtc_Word16 savedVADEnabled = inst->VADInst.VADEnabled;
    int savedVADMode = inst->VADInst.VADMode;
#endif 
    DSPStats_t saveStats;
    WebRtc_Word16 saveMsPerCall = inst->millisecondsPerCall;
    enum BGNMode saveBgnMode = inst->BGNInst.bgnMode;
#ifdef NETEQ_STEREO
    MasterSlaveInfo saveMSinfo;
#endif

    WEBRTC_SPL_MEMCPY_W16(&saveStats, &(inst->statInst),
        sizeof(DSPStats_t)/sizeof(WebRtc_Word16));

#ifdef NETEQ_STEREO
    WEBRTC_SPL_MEMCPY_W16(&saveMSinfo, &(inst->msInfo),
        sizeof(MasterSlaveInfo)/sizeof(WebRtc_Word16));
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

    
    WebRtcSpl_MemSetW16((WebRtc_Word16 *) inst, 0, sizeof(DSPInst_t) / sizeof(WebRtc_Word16));

    
#ifdef NETEQ_CNG_CODEC
    inst->CNG_Codec_inst = (CNG_dec_inst *)savedPtr1;
#endif
    inst->pw16_readAddress = (WebRtc_Word16 *) savedPtr2;
    inst->pw16_writeAddress = (WebRtc_Word16 *) savedPtr3;
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
        sizeof(DSPStats_t)/sizeof(WebRtc_Word16));

#ifdef NETEQ_STEREO
    WEBRTC_SPL_MEMCPY_W16(&(inst->msInfo), &saveMSinfo,
        sizeof(MasterSlaveInfo)/sizeof(WebRtc_Word16));
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

    
    inst->pw16_readAddress = (WebRtc_Word16 *) data2DspAddress;
    inst->pw16_writeAddress = (WebRtc_Word16 *) data2McuAddress;

    
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

#ifdef NETEQ_VAD

















int WebRtcNetEQ_InitVAD(PostDecodeVAD_t *VADInst, WebRtc_UWord16 fs)
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
    WebRtc_Word16 fs_mult;

    
    fs_mult = WebRtcSpl_DivW32W16ResW16(inst->fs, 8000);

    
    WebRtcSpl_MemSetW16(inst->speechBuffer, 0, SPEECH_BUF_SIZE);
    inst->endPosition = 565 * fs_mult;
    inst->curPosition = inst->endPosition - inst->ExpandInst.w16_overlap;

    return 0;
}

