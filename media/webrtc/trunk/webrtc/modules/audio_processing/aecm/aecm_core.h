











#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AECM_AECM_CORE_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AECM_AECM_CORE_H_

#include "common_audio/signal_processing/include/signal_processing_library.h"
#include "modules/audio_processing/aecm/aecm_defines.h"
#include "typedefs.h"

#ifdef _MSC_VER 
#define ALIGN8_BEG __declspec(align(8))
#define ALIGN8_END
#else 
#define ALIGN8_BEG
#define ALIGN8_END __attribute__((aligned(8)))
#endif

extern const WebRtc_Word16 WebRtcAecm_kSqrtHanning[] ALIGN8_END;

typedef struct {
    WebRtc_Word16 real;
    WebRtc_Word16 imag;
} complex16_t;

typedef struct
{
    int farBufWritePos;
    int farBufReadPos;
    int knownDelay;
    int lastKnownDelay;
    int firstVAD; 

    void *farFrameBuf;
    void *nearNoisyFrameBuf;
    void *nearCleanFrameBuf;
    void *outFrameBuf;

    WebRtc_Word16 farBuf[FAR_BUF_LEN];

    WebRtc_Word16 mult;
    WebRtc_UWord32 seed;

    
    void* delay_estimator;
    WebRtc_UWord16 currentDelay;
    
    
    uint16_t far_history[PART_LEN1 * MAX_DELAY];
    int far_history_pos;
    int far_q_domains[MAX_DELAY];

    WebRtc_Word16 nlpFlag;
    WebRtc_Word16 fixedDelay;

    WebRtc_UWord32 totCount;

    WebRtc_Word16 dfaCleanQDomain;
    WebRtc_Word16 dfaCleanQDomainOld;
    WebRtc_Word16 dfaNoisyQDomain;
    WebRtc_Word16 dfaNoisyQDomainOld;

    WebRtc_Word16 nearLogEnergy[MAX_BUF_LEN];
    WebRtc_Word16 farLogEnergy;
    WebRtc_Word16 echoAdaptLogEnergy[MAX_BUF_LEN];
    WebRtc_Word16 echoStoredLogEnergy[MAX_BUF_LEN];

    
    
    
    WebRtc_Word16 channelStored_buf[PART_LEN1 + 8];
    WebRtc_Word16 channelAdapt16_buf[PART_LEN1 + 8];
    WebRtc_Word32 channelAdapt32_buf[PART_LEN1 + 8];
    WebRtc_Word16 xBuf_buf[PART_LEN2 + 16]; 
    WebRtc_Word16 dBufClean_buf[PART_LEN2 + 16]; 
    WebRtc_Word16 dBufNoisy_buf[PART_LEN2 + 16]; 
    WebRtc_Word16 outBuf_buf[PART_LEN + 8];

    
    WebRtc_Word16 *channelStored;
    WebRtc_Word16 *channelAdapt16;
    WebRtc_Word32 *channelAdapt32;
    WebRtc_Word16 *xBuf;
    WebRtc_Word16 *dBufClean;
    WebRtc_Word16 *dBufNoisy;
    WebRtc_Word16 *outBuf;

    WebRtc_Word32 echoFilt[PART_LEN1];
    WebRtc_Word16 nearFilt[PART_LEN1];
    WebRtc_Word32 noiseEst[PART_LEN1];
    int           noiseEstTooLowCtr[PART_LEN1];
    int           noiseEstTooHighCtr[PART_LEN1];
    WebRtc_Word16 noiseEstCtr;
    WebRtc_Word16 cngMode;

    WebRtc_Word32 mseAdaptOld;
    WebRtc_Word32 mseStoredOld;
    WebRtc_Word32 mseThreshold;

    WebRtc_Word16 farEnergyMin;
    WebRtc_Word16 farEnergyMax;
    WebRtc_Word16 farEnergyMaxMin;
    WebRtc_Word16 farEnergyVAD;
    WebRtc_Word16 farEnergyMSE;
    int currentVADValue;
    WebRtc_Word16 vadUpdateCount;

    WebRtc_Word16 startupState;
    WebRtc_Word16 mseChannelCount;
    WebRtc_Word16 supGain;
    WebRtc_Word16 supGainOld;

    WebRtc_Word16 supGainErrParamA;
    WebRtc_Word16 supGainErrParamD;
    WebRtc_Word16 supGainErrParamDiffAB;
    WebRtc_Word16 supGainErrParamDiffBD;

    struct RealFFT* real_fft;

#ifdef AEC_DEBUG
    FILE *farFile;
    FILE *nearFile;
    FILE *outFile;
#endif
} AecmCore_t;
















int WebRtcAecm_CreateCore(AecmCore_t **aecm);















int WebRtcAecm_InitCore(AecmCore_t * const aecm, int samplingFreq);












int WebRtcAecm_FreeCore(AecmCore_t *aecm);

int WebRtcAecm_Control(AecmCore_t *aecm, int delay, int nlpFlag);












void WebRtcAecm_InitEchoPathCore(AecmCore_t* aecm, const WebRtc_Word16* echo_path);
















int WebRtcAecm_ProcessFrame(AecmCore_t * aecm, const WebRtc_Word16 * farend,
                            const WebRtc_Word16 * nearendNoisy,
                            const WebRtc_Word16 * nearendClean,
                            WebRtc_Word16 * out);

















int WebRtcAecm_ProcessBlock(AecmCore_t * aecm, const WebRtc_Word16 * farend,
                            const WebRtc_Word16 * nearendNoisy,
                            const WebRtc_Word16 * noisyClean,
                            WebRtc_Word16 * out);











void WebRtcAecm_BufferFarFrame(AecmCore_t * const aecm, const WebRtc_Word16 * const farend,
                               const int farLen);












void WebRtcAecm_FetchFarFrame(AecmCore_t * const aecm, WebRtc_Word16 * const farend,
                              const int farLen, const int knownDelay);





typedef void (*CalcLinearEnergies)(
    AecmCore_t* aecm,
    const WebRtc_UWord16* far_spectrum,
    WebRtc_Word32* echoEst,
    WebRtc_UWord32* far_energy,
    WebRtc_UWord32* echo_energy_adapt,
    WebRtc_UWord32* echo_energy_stored);
extern CalcLinearEnergies WebRtcAecm_CalcLinearEnergies;

typedef void (*StoreAdaptiveChannel)(
    AecmCore_t* aecm,
    const WebRtc_UWord16* far_spectrum,
    WebRtc_Word32* echo_est);
extern StoreAdaptiveChannel WebRtcAecm_StoreAdaptiveChannel;

typedef void (*ResetAdaptiveChannel)(AecmCore_t* aecm);
extern ResetAdaptiveChannel WebRtcAecm_ResetAdaptiveChannel;

typedef void (*WindowAndFFT)(
    AecmCore_t* aecm,
    WebRtc_Word16* fft,
    const WebRtc_Word16* time_signal,
    complex16_t* freq_signal,
    int time_signal_scaling);
extern WindowAndFFT WebRtcAecm_WindowAndFFT;

typedef void (*InverseFFTAndWindow)(
    AecmCore_t* aecm,
    WebRtc_Word16* fft, complex16_t* efw,
    WebRtc_Word16* output,
    const WebRtc_Word16* nearendClean);
extern InverseFFTAndWindow WebRtcAecm_InverseFFTAndWindow;




#if (defined WEBRTC_DETECT_ARM_NEON) || defined (WEBRTC_ARCH_ARM_NEON)
void WebRtcAecm_WindowAndFFTNeon(AecmCore_t* aecm,
                                 WebRtc_Word16* fft,
                                 const WebRtc_Word16* time_signal,
                                 complex16_t* freq_signal,
                                 int time_signal_scaling);

void WebRtcAecm_InverseFFTAndWindowNeon(AecmCore_t* aecm,
                                        WebRtc_Word16* fft,
                                        complex16_t* efw,
                                        WebRtc_Word16* output,
                                        const WebRtc_Word16* nearendClean);

void WebRtcAecm_CalcLinearEnergiesNeon(AecmCore_t* aecm,
                                       const WebRtc_UWord16* far_spectrum,
                                       WebRtc_Word32* echo_est,
                                       WebRtc_UWord32* far_energy,
                                       WebRtc_UWord32* echo_energy_adapt,
                                       WebRtc_UWord32* echo_energy_stored);

void WebRtcAecm_StoreAdaptiveChannelNeon(AecmCore_t* aecm,
                                         const WebRtc_UWord16* far_spectrum,
                                         WebRtc_Word32* echo_est);

void WebRtcAecm_ResetAdaptiveChannelNeon(AecmCore_t* aecm);
#endif

#endif
