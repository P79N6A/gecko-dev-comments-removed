











































































































#if !defined NETEQ_DEFINES_H
#define NETEQ_DEFINES_H


















#define DSP_INSTR_NORMAL                         0x1000


#define DSP_INSTR_MERGE                          0x2000



#define DSP_INSTR_EXPAND                         0x3000


#define DSP_INSTR_ACCELERATE                     0x4000


#define DSP_INSTR_DO_RFC3389CNG                  0x5000


#define DSP_INSTR_DTMF_GENERATE                  0x6000



#define DSP_INSTR_NORMAL_ONE_DESC                0x7000


#define DSP_INSTR_DO_CODEC_INTERNAL_CNG          0x8000


#define DSP_INSTR_PREEMPTIVE_EXPAND              0x9000


#define DSP_INSTR_DO_ALTERNATIVE_PLC             0xB000


#define DSP_INSTR_DO_ALTERNATIVE_PLC_INC_TS      0xC000


#define DSP_INSTR_DO_AUDIO_REPETITION            0xD000


#define DSP_INSTR_DO_AUDIO_REPETITION_INC_TS     0xE000


#define DSP_INSTR_FADE_TO_BGN                    0xF000






#define DSP_CODEC_NO_CHANGE                      0x0100
#define DSP_CODEC_NEW_CODEC                      0x0200
#define DSP_CODEC_ADD_LATE_PKT                   0x0300
#define DSP_CODEC_RESET                          0x0400
#define DSP_DTMF_PAYLOAD                         0x0010










#define DSP_CODEC_MASK_RED_FLAG                  0x7FFF
#define DSP_CODEC_RED_FLAG                       0x8000













#define MODE_NORMAL                    0x0000
#define MODE_EXPAND                    0x0001
#define MODE_MERGE                     0x0002
#define MODE_SUCCESS_ACCELERATE        0x0003
#define MODE_UNSUCCESS_ACCELERATE      0x0004
#define MODE_RFC3389CNG                0x0005
#define MODE_LOWEN_ACCELERATE          0x0006
#define MODE_DTMF                      0x0007
#define MODE_ONE_DESCRIPTOR            0x0008
#define MODE_CODEC_INTERNAL_CNG        0x0009
#define MODE_SUCCESS_PREEMPTIVE        0x000A
#define MODE_UNSUCCESS_PREEMPTIVE      0x000B
#define MODE_LOWEN_PREEMPTIVE          0x000C
#define MODE_FADE_TO_BGN               0x000D

#define MODE_ERROR                     0x0010

#define MODE_AWAITING_CODEC_PTR        0x0100

#define MODE_BGN_ONLY                  0x0200

#define MODE_MASTER_DTMF_SIGNAL        0x0400

#define MODE_USING_STEREO              0x0800







#if (defined(NETEQ_ALL_SPECIAL_CODECS))
    #define NETEQ_CNG_CODEC
    #define NETEQ_ATEVENT_DECODE
    #define NETEQ_RED_CODEC
    #define NETEQ_VAD
    #define NETEQ_ARBITRARY_CODEC
#endif

#if (defined(NETEQ_ALL_NB_CODECS))        
    #define NETEQ_PCM16B_CODEC
    #define NETEQ_G711_CODEC
    #define NETEQ_ILBC_CODEC
    #define NETEQ_G729_CODEC
    #define NETEQ_G726_CODEC
    #define NETEQ_GSMFR_CODEC
    #define NETEQ_OPUS_CODEC
    #define NETEQ_AMR_CODEC
#endif

#if (defined(NETEQ_ALL_WB_CODECS))        
    #define NETEQ_ISAC_CODEC
    #define NETEQ_G722_CODEC
    #define NETEQ_G722_1_CODEC
    #define NETEQ_G729_1_CODEC
    #define NETEQ_OPUS_CODEC
    #define NETEQ_SPEEX_CODEC
    #define NETEQ_AMRWB_CODEC
    #define NETEQ_WIDEBAND
#endif

#if (defined(NETEQ_ALL_WB32_CODECS))        
    #define NETEQ_ISAC_SWB_CODEC
    #define NETEQ_32KHZ_WIDEBAND
    #define NETEQ_G722_1C_CODEC
    #define NETEQ_CELT_CODEC
    #define NETEQ_OPUS_CODEC
#endif

#if (defined(NETEQ_VOICEENGINE_CODECS))
    
    #define NETEQ_CNG_CODEC
    #define NETEQ_ATEVENT_DECODE
    #define NETEQ_RED_CODEC
    #define NETEQ_VAD
    #define NETEQ_ARBITRARY_CODEC


    #define NETEQ_PCM16B_CODEC
    #define NETEQ_G711_CODEC
    #define NETEQ_ILBC_CODEC
    #define NETEQ_AMR_CODEC
    #define NETEQ_G729_CODEC
    #define NETEQ_GSMFR_CODEC


    #define NETEQ_WIDEBAND
    #define NETEQ_ISAC_CODEC
    #define NETEQ_G722_CODEC
    #define NETEQ_G722_1_CODEC
    #define NETEQ_G729_1_CODEC
    #define NETEQ_AMRWB_CODEC
    #define NETEQ_SPEEX_CODEC

    
    #define NETEQ_ISAC_SWB_CODEC
    #define NETEQ_32KHZ_WIDEBAND
    #define NETEQ_G722_1C_CODEC
    #define NETEQ_CELT_CODEC

    
    #define NETEQ_OPUS_CODEC
    #define NETEQ_ISAC_FB_CODEC
#endif 

#if (defined(NETEQ_ALL_CODECS))
    
    #define NETEQ_CNG_CODEC
    #define NETEQ_ATEVENT_DECODE
    #define NETEQ_RED_CODEC
    #define NETEQ_VAD
    #define NETEQ_ARBITRARY_CODEC


    #define NETEQ_PCM16B_CODEC
    #define NETEQ_G711_CODEC
    #define NETEQ_ILBC_CODEC
    #define NETEQ_G729_CODEC
    #define NETEQ_G726_CODEC
    #define NETEQ_GSMFR_CODEC
    #define NETEQ_AMR_CODEC

    
    #define NETEQ_WIDEBAND
    #define NETEQ_ISAC_CODEC
    #define NETEQ_G722_CODEC
    #define NETEQ_G722_1_CODEC
    #define NETEQ_G729_1_CODEC
    #define NETEQ_SPEEX_CODEC
    #define NETEQ_AMRWB_CODEC


    #define NETEQ_ISAC_SWB_CODEC
    #define NETEQ_32KHZ_WIDEBAND
    #define NETEQ_G722_1C_CODEC
    #define NETEQ_CELT_CODEC


    #define NETEQ_48KHZ_WIDEBAND
    #define NETEQ_OPUS_CODEC
    #define NETEQ_ISAC_FB_CODEC
#endif


#if defined(NETEQ_48KHZ_WIDEBAND)
    #define NETEQ_MAX_FRAME_SIZE 5760  /* 120 ms super wideband */
    #define NETEQ_MAX_OUTPUT_SIZE 6480  /* 120+15 ms super wideband (120 ms
                                         * decoded + 15 ms for merge overlap) */
#elif defined(NETEQ_32KHZ_WIDEBAND)
    #define NETEQ_MAX_FRAME_SIZE 3840  /* 120 ms super wideband */
    #define NETEQ_MAX_OUTPUT_SIZE 4320  /* 120+15 ms super wideband (120 ms
                                         * decoded + 15 ms for merge overlap) */
#elif defined(NETEQ_WIDEBAND)
    #define NETEQ_MAX_FRAME_SIZE 1920  /* 120 ms wideband */
    #define NETEQ_MAX_OUTPUT_SIZE 2160  /* 120+15 ms wideband (120 ms decoded +
                                         * 15 ms for merge overlap) */
#else
    #define NETEQ_MAX_FRAME_SIZE 960  /* 120 ms narrowband */
    #define NETEQ_MAX_OUTPUT_SIZE 1080  /* 120+15 ms narrowband (120 ms decoded
                                         * + 15 ms for merge overlap) */
#endif



#define NETEQ_STEREO

#endif 

