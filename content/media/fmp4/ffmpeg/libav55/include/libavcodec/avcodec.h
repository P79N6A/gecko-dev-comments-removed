



















#ifndef AVCODEC_AVCODEC_H
#define AVCODEC_AVCODEC_H







#include <errno.h>
#include "libavutil/samplefmt.h"
#include "libavutil/attributes.h"
#include "libavutil/avutil.h"
#include "libavutil/buffer.h"
#include "libavutil/cpu.h"
#include "libavutil/dict.h"
#include "libavutil/frame.h"
#include "libavutil/log.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"

#include "version.h"

#if FF_API_FAST_MALLOC

#include "libavutil/mem.h"
#endif

























































enum AVCodecID {
    AV_CODEC_ID_NONE,

    
    AV_CODEC_ID_MPEG1VIDEO,
    AV_CODEC_ID_MPEG2VIDEO, 
#if FF_API_XVMC
    AV_CODEC_ID_MPEG2VIDEO_XVMC,
#endif 
    AV_CODEC_ID_H261,
    AV_CODEC_ID_H263,
    AV_CODEC_ID_RV10,
    AV_CODEC_ID_RV20,
    AV_CODEC_ID_MJPEG,
    AV_CODEC_ID_MJPEGB,
    AV_CODEC_ID_LJPEG,
    AV_CODEC_ID_SP5X,
    AV_CODEC_ID_JPEGLS,
    AV_CODEC_ID_MPEG4,
    AV_CODEC_ID_RAWVIDEO,
    AV_CODEC_ID_MSMPEG4V1,
    AV_CODEC_ID_MSMPEG4V2,
    AV_CODEC_ID_MSMPEG4V3,
    AV_CODEC_ID_WMV1,
    AV_CODEC_ID_WMV2,
    AV_CODEC_ID_H263P,
    AV_CODEC_ID_H263I,
    AV_CODEC_ID_FLV1,
    AV_CODEC_ID_SVQ1,
    AV_CODEC_ID_SVQ3,
    AV_CODEC_ID_DVVIDEO,
    AV_CODEC_ID_HUFFYUV,
    AV_CODEC_ID_CYUV,
    AV_CODEC_ID_H264,
    AV_CODEC_ID_INDEO3,
    AV_CODEC_ID_VP3,
    AV_CODEC_ID_THEORA,
    AV_CODEC_ID_ASV1,
    AV_CODEC_ID_ASV2,
    AV_CODEC_ID_FFV1,
    AV_CODEC_ID_4XM,
    AV_CODEC_ID_VCR1,
    AV_CODEC_ID_CLJR,
    AV_CODEC_ID_MDEC,
    AV_CODEC_ID_ROQ,
    AV_CODEC_ID_INTERPLAY_VIDEO,
    AV_CODEC_ID_XAN_WC3,
    AV_CODEC_ID_XAN_WC4,
    AV_CODEC_ID_RPZA,
    AV_CODEC_ID_CINEPAK,
    AV_CODEC_ID_WS_VQA,
    AV_CODEC_ID_MSRLE,
    AV_CODEC_ID_MSVIDEO1,
    AV_CODEC_ID_IDCIN,
    AV_CODEC_ID_8BPS,
    AV_CODEC_ID_SMC,
    AV_CODEC_ID_FLIC,
    AV_CODEC_ID_TRUEMOTION1,
    AV_CODEC_ID_VMDVIDEO,
    AV_CODEC_ID_MSZH,
    AV_CODEC_ID_ZLIB,
    AV_CODEC_ID_QTRLE,
    AV_CODEC_ID_TSCC,
    AV_CODEC_ID_ULTI,
    AV_CODEC_ID_QDRAW,
    AV_CODEC_ID_VIXL,
    AV_CODEC_ID_QPEG,
    AV_CODEC_ID_PNG,
    AV_CODEC_ID_PPM,
    AV_CODEC_ID_PBM,
    AV_CODEC_ID_PGM,
    AV_CODEC_ID_PGMYUV,
    AV_CODEC_ID_PAM,
    AV_CODEC_ID_FFVHUFF,
    AV_CODEC_ID_RV30,
    AV_CODEC_ID_RV40,
    AV_CODEC_ID_VC1,
    AV_CODEC_ID_WMV3,
    AV_CODEC_ID_LOCO,
    AV_CODEC_ID_WNV1,
    AV_CODEC_ID_AASC,
    AV_CODEC_ID_INDEO2,
    AV_CODEC_ID_FRAPS,
    AV_CODEC_ID_TRUEMOTION2,
    AV_CODEC_ID_BMP,
    AV_CODEC_ID_CSCD,
    AV_CODEC_ID_MMVIDEO,
    AV_CODEC_ID_ZMBV,
    AV_CODEC_ID_AVS,
    AV_CODEC_ID_SMACKVIDEO,
    AV_CODEC_ID_NUV,
    AV_CODEC_ID_KMVC,
    AV_CODEC_ID_FLASHSV,
    AV_CODEC_ID_CAVS,
    AV_CODEC_ID_JPEG2000,
    AV_CODEC_ID_VMNC,
    AV_CODEC_ID_VP5,
    AV_CODEC_ID_VP6,
    AV_CODEC_ID_VP6F,
    AV_CODEC_ID_TARGA,
    AV_CODEC_ID_DSICINVIDEO,
    AV_CODEC_ID_TIERTEXSEQVIDEO,
    AV_CODEC_ID_TIFF,
    AV_CODEC_ID_GIF,
    AV_CODEC_ID_DXA,
    AV_CODEC_ID_DNXHD,
    AV_CODEC_ID_THP,
    AV_CODEC_ID_SGI,
    AV_CODEC_ID_C93,
    AV_CODEC_ID_BETHSOFTVID,
    AV_CODEC_ID_PTX,
    AV_CODEC_ID_TXD,
    AV_CODEC_ID_VP6A,
    AV_CODEC_ID_AMV,
    AV_CODEC_ID_VB,
    AV_CODEC_ID_PCX,
    AV_CODEC_ID_SUNRAST,
    AV_CODEC_ID_INDEO4,
    AV_CODEC_ID_INDEO5,
    AV_CODEC_ID_MIMIC,
    AV_CODEC_ID_RL2,
    AV_CODEC_ID_ESCAPE124,
    AV_CODEC_ID_DIRAC,
    AV_CODEC_ID_BFI,
    AV_CODEC_ID_CMV,
    AV_CODEC_ID_MOTIONPIXELS,
    AV_CODEC_ID_TGV,
    AV_CODEC_ID_TGQ,
    AV_CODEC_ID_TQI,
    AV_CODEC_ID_AURA,
    AV_CODEC_ID_AURA2,
    AV_CODEC_ID_V210X,
    AV_CODEC_ID_TMV,
    AV_CODEC_ID_V210,
    AV_CODEC_ID_DPX,
    AV_CODEC_ID_MAD,
    AV_CODEC_ID_FRWU,
    AV_CODEC_ID_FLASHSV2,
    AV_CODEC_ID_CDGRAPHICS,
    AV_CODEC_ID_R210,
    AV_CODEC_ID_ANM,
    AV_CODEC_ID_BINKVIDEO,
    AV_CODEC_ID_IFF_ILBM,
    AV_CODEC_ID_IFF_BYTERUN1,
    AV_CODEC_ID_KGV1,
    AV_CODEC_ID_YOP,
    AV_CODEC_ID_VP8,
    AV_CODEC_ID_PICTOR,
    AV_CODEC_ID_ANSI,
    AV_CODEC_ID_A64_MULTI,
    AV_CODEC_ID_A64_MULTI5,
    AV_CODEC_ID_R10K,
    AV_CODEC_ID_MXPEG,
    AV_CODEC_ID_LAGARITH,
    AV_CODEC_ID_PRORES,
    AV_CODEC_ID_JV,
    AV_CODEC_ID_DFA,
    AV_CODEC_ID_WMV3IMAGE,
    AV_CODEC_ID_VC1IMAGE,
    AV_CODEC_ID_UTVIDEO,
    AV_CODEC_ID_BMV_VIDEO,
    AV_CODEC_ID_VBLE,
    AV_CODEC_ID_DXTORY,
    AV_CODEC_ID_V410,
    AV_CODEC_ID_XWD,
    AV_CODEC_ID_CDXL,
    AV_CODEC_ID_XBM,
    AV_CODEC_ID_ZEROCODEC,
    AV_CODEC_ID_MSS1,
    AV_CODEC_ID_MSA1,
    AV_CODEC_ID_TSCC2,
    AV_CODEC_ID_MTS2,
    AV_CODEC_ID_CLLC,
    AV_CODEC_ID_MSS2,
    AV_CODEC_ID_VP9,
    AV_CODEC_ID_AIC,
    AV_CODEC_ID_ESCAPE130,
    AV_CODEC_ID_G2M,
    AV_CODEC_ID_WEBP,
    AV_CODEC_ID_HNM4_VIDEO,
    AV_CODEC_ID_HEVC,
    AV_CODEC_ID_FIC,

    
    AV_CODEC_ID_FIRST_AUDIO = 0x10000,     
    AV_CODEC_ID_PCM_S16LE = 0x10000,
    AV_CODEC_ID_PCM_S16BE,
    AV_CODEC_ID_PCM_U16LE,
    AV_CODEC_ID_PCM_U16BE,
    AV_CODEC_ID_PCM_S8,
    AV_CODEC_ID_PCM_U8,
    AV_CODEC_ID_PCM_MULAW,
    AV_CODEC_ID_PCM_ALAW,
    AV_CODEC_ID_PCM_S32LE,
    AV_CODEC_ID_PCM_S32BE,
    AV_CODEC_ID_PCM_U32LE,
    AV_CODEC_ID_PCM_U32BE,
    AV_CODEC_ID_PCM_S24LE,
    AV_CODEC_ID_PCM_S24BE,
    AV_CODEC_ID_PCM_U24LE,
    AV_CODEC_ID_PCM_U24BE,
    AV_CODEC_ID_PCM_S24DAUD,
    AV_CODEC_ID_PCM_ZORK,
    AV_CODEC_ID_PCM_S16LE_PLANAR,
    AV_CODEC_ID_PCM_DVD,
    AV_CODEC_ID_PCM_F32BE,
    AV_CODEC_ID_PCM_F32LE,
    AV_CODEC_ID_PCM_F64BE,
    AV_CODEC_ID_PCM_F64LE,
    AV_CODEC_ID_PCM_BLURAY,
    AV_CODEC_ID_PCM_LXF,
    AV_CODEC_ID_S302M,
    AV_CODEC_ID_PCM_S8_PLANAR,
    AV_CODEC_ID_PCM_S24LE_PLANAR,
    AV_CODEC_ID_PCM_S32LE_PLANAR,

    
    AV_CODEC_ID_ADPCM_IMA_QT = 0x11000,
    AV_CODEC_ID_ADPCM_IMA_WAV,
    AV_CODEC_ID_ADPCM_IMA_DK3,
    AV_CODEC_ID_ADPCM_IMA_DK4,
    AV_CODEC_ID_ADPCM_IMA_WS,
    AV_CODEC_ID_ADPCM_IMA_SMJPEG,
    AV_CODEC_ID_ADPCM_MS,
    AV_CODEC_ID_ADPCM_4XM,
    AV_CODEC_ID_ADPCM_XA,
    AV_CODEC_ID_ADPCM_ADX,
    AV_CODEC_ID_ADPCM_EA,
    AV_CODEC_ID_ADPCM_G726,
    AV_CODEC_ID_ADPCM_CT,
    AV_CODEC_ID_ADPCM_SWF,
    AV_CODEC_ID_ADPCM_YAMAHA,
    AV_CODEC_ID_ADPCM_SBPRO_4,
    AV_CODEC_ID_ADPCM_SBPRO_3,
    AV_CODEC_ID_ADPCM_SBPRO_2,
    AV_CODEC_ID_ADPCM_THP,
    AV_CODEC_ID_ADPCM_IMA_AMV,
    AV_CODEC_ID_ADPCM_EA_R1,
    AV_CODEC_ID_ADPCM_EA_R3,
    AV_CODEC_ID_ADPCM_EA_R2,
    AV_CODEC_ID_ADPCM_IMA_EA_SEAD,
    AV_CODEC_ID_ADPCM_IMA_EA_EACS,
    AV_CODEC_ID_ADPCM_EA_XAS,
    AV_CODEC_ID_ADPCM_EA_MAXIS_XA,
    AV_CODEC_ID_ADPCM_IMA_ISS,
    AV_CODEC_ID_ADPCM_G722,
    AV_CODEC_ID_ADPCM_IMA_APC,

    
    AV_CODEC_ID_AMR_NB = 0x12000,
    AV_CODEC_ID_AMR_WB,

    
    AV_CODEC_ID_RA_144 = 0x13000,
    AV_CODEC_ID_RA_288,

    
    AV_CODEC_ID_ROQ_DPCM = 0x14000,
    AV_CODEC_ID_INTERPLAY_DPCM,
    AV_CODEC_ID_XAN_DPCM,
    AV_CODEC_ID_SOL_DPCM,

    
    AV_CODEC_ID_MP2 = 0x15000,
    AV_CODEC_ID_MP3, 
    AV_CODEC_ID_AAC,
    AV_CODEC_ID_AC3,
    AV_CODEC_ID_DTS,
    AV_CODEC_ID_VORBIS,
    AV_CODEC_ID_DVAUDIO,
    AV_CODEC_ID_WMAV1,
    AV_CODEC_ID_WMAV2,
    AV_CODEC_ID_MACE3,
    AV_CODEC_ID_MACE6,
    AV_CODEC_ID_VMDAUDIO,
    AV_CODEC_ID_FLAC,
    AV_CODEC_ID_MP3ADU,
    AV_CODEC_ID_MP3ON4,
    AV_CODEC_ID_SHORTEN,
    AV_CODEC_ID_ALAC,
    AV_CODEC_ID_WESTWOOD_SND1,
    AV_CODEC_ID_GSM, 
    AV_CODEC_ID_QDM2,
    AV_CODEC_ID_COOK,
    AV_CODEC_ID_TRUESPEECH,
    AV_CODEC_ID_TTA,
    AV_CODEC_ID_SMACKAUDIO,
    AV_CODEC_ID_QCELP,
    AV_CODEC_ID_WAVPACK,
    AV_CODEC_ID_DSICINAUDIO,
    AV_CODEC_ID_IMC,
    AV_CODEC_ID_MUSEPACK7,
    AV_CODEC_ID_MLP,
    AV_CODEC_ID_GSM_MS, 
    AV_CODEC_ID_ATRAC3,
#if FF_API_VOXWARE
    AV_CODEC_ID_VOXWARE,
#endif
    AV_CODEC_ID_APE,
    AV_CODEC_ID_NELLYMOSER,
    AV_CODEC_ID_MUSEPACK8,
    AV_CODEC_ID_SPEEX,
    AV_CODEC_ID_WMAVOICE,
    AV_CODEC_ID_WMAPRO,
    AV_CODEC_ID_WMALOSSLESS,
    AV_CODEC_ID_ATRAC3P,
    AV_CODEC_ID_EAC3,
    AV_CODEC_ID_SIPR,
    AV_CODEC_ID_MP1,
    AV_CODEC_ID_TWINVQ,
    AV_CODEC_ID_TRUEHD,
    AV_CODEC_ID_MP4ALS,
    AV_CODEC_ID_ATRAC1,
    AV_CODEC_ID_BINKAUDIO_RDFT,
    AV_CODEC_ID_BINKAUDIO_DCT,
    AV_CODEC_ID_AAC_LATM,
    AV_CODEC_ID_QDMC,
    AV_CODEC_ID_CELT,
    AV_CODEC_ID_G723_1,
    AV_CODEC_ID_G729,
    AV_CODEC_ID_8SVX_EXP,
    AV_CODEC_ID_8SVX_FIB,
    AV_CODEC_ID_BMV_AUDIO,
    AV_CODEC_ID_RALF,
    AV_CODEC_ID_IAC,
    AV_CODEC_ID_ILBC,
    AV_CODEC_ID_OPUS,
    AV_CODEC_ID_COMFORT_NOISE,
    AV_CODEC_ID_TAK,
    AV_CODEC_ID_METASOUND,

    
    AV_CODEC_ID_FIRST_SUBTITLE = 0x17000,          
    AV_CODEC_ID_DVD_SUBTITLE = 0x17000,
    AV_CODEC_ID_DVB_SUBTITLE,
    AV_CODEC_ID_TEXT,  
    AV_CODEC_ID_XSUB,
    AV_CODEC_ID_SSA,
    AV_CODEC_ID_MOV_TEXT,
    AV_CODEC_ID_HDMV_PGS_SUBTITLE,
    AV_CODEC_ID_DVB_TELETEXT,
    AV_CODEC_ID_SRT,

    
    AV_CODEC_ID_FIRST_UNKNOWN = 0x18000,           
    AV_CODEC_ID_TTF = 0x18000,

    AV_CODEC_ID_PROBE = 0x19000, 

    AV_CODEC_ID_MPEG2TS = 0x20000, 

    AV_CODEC_ID_MPEG4SYSTEMS = 0x20001, 

    AV_CODEC_ID_FFMETADATA = 0x21000,   
};






typedef struct AVCodecDescriptor {
    enum AVCodecID     id;
    enum AVMediaType type;
    




    const char      *name;
    


    const char *long_name;
    


    int             props;
} AVCodecDescriptor;





#define AV_CODEC_PROP_INTRA_ONLY    (1 << 0)





#define AV_CODEC_PROP_LOSSY         (1 << 1)



#define AV_CODEC_PROP_LOSSLESS      (1 << 2)









#define FF_INPUT_BUFFER_PADDING_SIZE 8






#define FF_MIN_BUFFER_SIZE 16384






enum Motion_Est_ID {
    ME_ZERO = 1,    
    ME_FULL,
    ME_LOG,
    ME_PHODS,
    ME_EPZS,        
    ME_X1,          
    ME_HEX,         
    ME_UMH,         
    ME_TESA,        
};




enum AVDiscard{
    

    AVDISCARD_NONE    =-16, 
    AVDISCARD_DEFAULT =  0, 
    AVDISCARD_NONREF  =  8, 
    AVDISCARD_BIDIR   = 16, 
    AVDISCARD_NONKEY  = 32, 
    AVDISCARD_ALL     = 48, 
};

enum AVColorPrimaries{
    AVCOL_PRI_BT709       = 1, 
    AVCOL_PRI_UNSPECIFIED = 2,
    AVCOL_PRI_BT470M      = 4,
    AVCOL_PRI_BT470BG     = 5, 
    AVCOL_PRI_SMPTE170M   = 6, 
    AVCOL_PRI_SMPTE240M   = 7, 
    AVCOL_PRI_FILM        = 8,
    AVCOL_PRI_BT2020      = 9, 
    AVCOL_PRI_NB             , 
};

enum AVColorTransferCharacteristic{
    AVCOL_TRC_BT709        =  1, 
    AVCOL_TRC_UNSPECIFIED  =  2,
    AVCOL_TRC_GAMMA22      =  4, 
    AVCOL_TRC_GAMMA28      =  5, 
    AVCOL_TRC_SMPTE170M    =  6, 
    AVCOL_TRC_SMPTE240M    =  7,
    AVCOL_TRC_LINEAR       =  8, 
    AVCOL_TRC_LOG          =  9, 
    AVCOL_TRC_LOG_SQRT     = 10, 
    AVCOL_TRC_IEC61966_2_4 = 11, 
    AVCOL_TRC_BT1361_ECG   = 12, 
    AVCOL_TRC_IEC61966_2_1 = 13, 
    AVCOL_TRC_BT2020_10    = 14, 
    AVCOL_TRC_BT2020_12    = 15, 
    AVCOL_TRC_NB               , 
};

enum AVColorSpace{
    AVCOL_SPC_RGB         =  0,
    AVCOL_SPC_BT709       =  1, 
    AVCOL_SPC_UNSPECIFIED =  2,
    AVCOL_SPC_FCC         =  4,
    AVCOL_SPC_BT470BG     =  5, 
    AVCOL_SPC_SMPTE170M   =  6, 
    AVCOL_SPC_SMPTE240M   =  7,
    AVCOL_SPC_YCOCG       =  8, 
    AVCOL_SPC_BT2020_NCL  =  9, 
    AVCOL_SPC_BT2020_CL   = 10, 
    AVCOL_SPC_NB              , 
};

enum AVColorRange{
    AVCOL_RANGE_UNSPECIFIED = 0,
    AVCOL_RANGE_MPEG        = 1, 
    AVCOL_RANGE_JPEG        = 2, 
    AVCOL_RANGE_NB             , 
};






enum AVChromaLocation{
    AVCHROMA_LOC_UNSPECIFIED = 0,
    AVCHROMA_LOC_LEFT        = 1, 
    AVCHROMA_LOC_CENTER      = 2, 
    AVCHROMA_LOC_TOPLEFT     = 3, 
    AVCHROMA_LOC_TOP         = 4,
    AVCHROMA_LOC_BOTTOMLEFT  = 5,
    AVCHROMA_LOC_BOTTOM      = 6,
    AVCHROMA_LOC_NB             , 
};

enum AVAudioServiceType {
    AV_AUDIO_SERVICE_TYPE_MAIN              = 0,
    AV_AUDIO_SERVICE_TYPE_EFFECTS           = 1,
    AV_AUDIO_SERVICE_TYPE_VISUALLY_IMPAIRED = 2,
    AV_AUDIO_SERVICE_TYPE_HEARING_IMPAIRED  = 3,
    AV_AUDIO_SERVICE_TYPE_DIALOGUE          = 4,
    AV_AUDIO_SERVICE_TYPE_COMMENTARY        = 5,
    AV_AUDIO_SERVICE_TYPE_EMERGENCY         = 6,
    AV_AUDIO_SERVICE_TYPE_VOICE_OVER        = 7,
    AV_AUDIO_SERVICE_TYPE_KARAOKE           = 8,
    AV_AUDIO_SERVICE_TYPE_NB                   , 
};




typedef struct RcOverride{
    int start_frame;
    int end_frame;
    int qscale; 
    float quality_factor;
} RcOverride;

#if FF_API_MAX_BFRAMES



#define FF_MAX_B_FRAMES 16
#endif










#define CODEC_FLAG_UNALIGNED 0x0001
#define CODEC_FLAG_QSCALE 0x0002  ///< Use fixed qscale.
#define CODEC_FLAG_4MV    0x0004  ///< 4 MV per MB allowed / advanced prediction for H.263.
#define CODEC_FLAG_OUTPUT_CORRUPT 0x0008 ///< Output even those frames that might be corrupted
#define CODEC_FLAG_QPEL   0x0010  ///< Use qpel MC.
#define CODEC_FLAG_GMC    0x0020  ///< Use GMC.
#define CODEC_FLAG_MV0    0x0040  ///< Always try a MB with MV=<0,0>.





#define CODEC_FLAG_INPUT_PRESERVED 0x0100
#define CODEC_FLAG_PASS1           0x0200   ///< Use internal 2pass ratecontrol in first pass mode.
#define CODEC_FLAG_PASS2           0x0400   ///< Use internal 2pass ratecontrol in second pass mode.
#define CODEC_FLAG_GRAY            0x2000   ///< Only decode/encode grayscale.
#if FF_API_EMU_EDGE




#define CODEC_FLAG_EMU_EDGE        0x4000
#endif
#define CODEC_FLAG_PSNR            0x8000   ///< error[?] variables will be set during encoding.
#define CODEC_FLAG_TRUNCATED       0x00010000 /** Input bitstream might be truncated at a random
                                                  location instead of only at frame boundaries. */
#define CODEC_FLAG_NORMALIZE_AQP  0x00020000 ///< Normalize adaptive quantization.
#define CODEC_FLAG_INTERLACED_DCT 0x00040000 ///< Use interlaced DCT.
#define CODEC_FLAG_LOW_DELAY      0x00080000 ///< Force low delay.
#define CODEC_FLAG_GLOBAL_HEADER  0x00400000 ///< Place global headers in extradata instead of every keyframe.
#define CODEC_FLAG_BITEXACT       0x00800000 ///< Use only bitexact stuff (except (I)DCT).

#define CODEC_FLAG_AC_PRED        0x01000000 ///< H.263 advanced intra coding / MPEG-4 AC prediction
#define CODEC_FLAG_LOOP_FILTER    0x00000800 ///< loop filter
#define CODEC_FLAG_INTERLACED_ME  0x20000000 ///< interlaced motion estimation
#define CODEC_FLAG_CLOSED_GOP     0x80000000
#define CODEC_FLAG2_FAST          0x00000001 ///< Allow non spec compliant speedup tricks.
#define CODEC_FLAG2_NO_OUTPUT     0x00000004 ///< Skip bitstream encoding.
#define CODEC_FLAG2_LOCAL_HEADER  0x00000008 ///< Place global headers at every keyframe instead of in extradata.
#define CODEC_FLAG2_IGNORE_CROP   0x00010000 ///< Discard cropping information from SPS.

#define CODEC_FLAG2_CHUNKS        0x00008000 ///< Input bitstream might be truncated at a packet boundaries instead of only at frame boundaries.








#define CODEC_CAP_DRAW_HORIZ_BAND 0x0001 ///< Decoder can use draw_horiz_band callback.





#define CODEC_CAP_DR1             0x0002
#define CODEC_CAP_TRUNCATED       0x0008
#if FF_API_XVMC

#define CODEC_CAP_HWACCEL         0x0010
#endif 























#define CODEC_CAP_DELAY           0x0020




#define CODEC_CAP_SMALL_LAST_FRAME 0x0040
#if FF_API_CAP_VDPAU



#define CODEC_CAP_HWACCEL_VDPAU    0x0080
#endif











#define CODEC_CAP_SUBFRAMES        0x0100




#define CODEC_CAP_EXPERIMENTAL     0x0200



#define CODEC_CAP_CHANNEL_CONF     0x0400
#if FF_API_NEG_LINESIZES



#define CODEC_CAP_NEG_LINESIZES    0x0800
#endif



#define CODEC_CAP_FRAME_THREADS    0x1000



#define CODEC_CAP_SLICE_THREADS    0x2000



#define CODEC_CAP_PARAM_CHANGE     0x4000



#define CODEC_CAP_AUTO_THREADS     0x8000



#define CODEC_CAP_VARIABLE_FRAME_SIZE 0x10000

#if FF_API_MB_TYPE

#define MB_TYPE_INTRA4x4   0x0001
#define MB_TYPE_INTRA16x16 0x0002 //FIXME H.264-specific
#define MB_TYPE_INTRA_PCM  0x0004 //FIXME H.264-specific
#define MB_TYPE_16x16      0x0008
#define MB_TYPE_16x8       0x0010
#define MB_TYPE_8x16       0x0020
#define MB_TYPE_8x8        0x0040
#define MB_TYPE_INTERLACED 0x0080
#define MB_TYPE_DIRECT2    0x0100 //FIXME
#define MB_TYPE_ACPRED     0x0200
#define MB_TYPE_GMC        0x0400
#define MB_TYPE_SKIP       0x0800
#define MB_TYPE_P0L0       0x1000
#define MB_TYPE_P1L0       0x2000
#define MB_TYPE_P0L1       0x4000
#define MB_TYPE_P1L1       0x8000
#define MB_TYPE_L0         (MB_TYPE_P0L0 | MB_TYPE_P1L0)
#define MB_TYPE_L1         (MB_TYPE_P0L1 | MB_TYPE_P1L1)
#define MB_TYPE_L0L1       (MB_TYPE_L0   | MB_TYPE_L1)
#define MB_TYPE_QUANT      0x00010000
#define MB_TYPE_CBP        0x00020000

#endif






typedef struct AVPanScan{
    




    int id;

    




    int width;
    int height;

    




    int16_t position[3][2];
}AVPanScan;

#if FF_API_QSCALE_TYPE
#define FF_QSCALE_TYPE_MPEG1 0
#define FF_QSCALE_TYPE_MPEG2 1
#define FF_QSCALE_TYPE_H264  2
#define FF_QSCALE_TYPE_VP56  3
#endif

#if FF_API_GET_BUFFER
#define FF_BUFFER_TYPE_INTERNAL 1
#define FF_BUFFER_TYPE_USER     2 ///< direct rendering buffers (image is (de)allocated by user)
#define FF_BUFFER_TYPE_SHARED   4 ///< Buffer from somewhere else; don't deallocate image (data/base), all other tables are not shared.
#define FF_BUFFER_TYPE_COPY     8 ///< Just a (modified) copy of some other buffer, don't deallocate anything.

#define FF_BUFFER_HINTS_VALID    0x01 // Buffer hints value is meaningful (if 0 ignore).
#define FF_BUFFER_HINTS_READABLE 0x02 // Codec will read from buffer.
#define FF_BUFFER_HINTS_PRESERVE 0x04 // User must not alter buffer content.
#define FF_BUFFER_HINTS_REUSABLE 0x08 // Codec will reuse the buffer (update).
#endif




#define AV_GET_BUFFER_FLAG_REF (1 << 0)







enum AVPacketSideDataType {
    AV_PKT_DATA_PALETTE,
    AV_PKT_DATA_NEW_EXTRADATA,

    














    AV_PKT_DATA_PARAM_CHANGE,

    


















    AV_PKT_DATA_H263_MB_INFO,
};























typedef struct AVPacket {
    




    AVBufferRef *buf;
    








    int64_t pts;
    




    int64_t dts;
    uint8_t *data;
    int   size;
    int   stream_index;
    


    int   flags;
    



    struct {
        uint8_t *data;
        int      size;
        enum AVPacketSideDataType type;
    } *side_data;
    int side_data_elems;

    



    int   duration;
#if FF_API_DESTRUCT_PACKET
    attribute_deprecated
    void  (*destruct)(struct AVPacket *);
    attribute_deprecated
    void  *priv;
#endif
    int64_t pos;                            

    
















    int64_t convergence_duration;
} AVPacket;
#define AV_PKT_FLAG_KEY     0x0001 ///< The packet contains a keyframe
#define AV_PKT_FLAG_CORRUPT 0x0002 ///< The packet content is corrupted

enum AVSideDataParamChangeFlags {
    AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT  = 0x0001,
    AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT = 0x0002,
    AV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE    = 0x0004,
    AV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS     = 0x0008,
};




struct AVCodecInternal;

enum AVFieldOrder {
    AV_FIELD_UNKNOWN,
    AV_FIELD_PROGRESSIVE,
    AV_FIELD_TT,          
    AV_FIELD_BB,          
    AV_FIELD_TB,          
    AV_FIELD_BT,          
};








typedef struct AVCodecContext {
    



    const AVClass *av_class;
    int log_level_offset;

    enum AVMediaType codec_type; 
    const struct AVCodec  *codec;
    char             codec_name[32];
    enum AVCodecID     codec_id; 

    












    unsigned int codec_tag;

    





    unsigned int stream_codec_tag;

    void *priv_data;

    





    struct AVCodecInternal *internal;

    




    void *opaque;

    




    int bit_rate;

    





    int bit_rate_tolerance;

    





    int global_quality;

    



    int compression_level;
#define FF_COMPRESSION_DEFAULT -1

    




    int flags;

    




    int flags2;

    










    uint8_t *extradata;
    int extradata_size;

    







    AVRational time_base;

    






    int ticks_per_frame;

    




















    int delay;


    
    







    int width, height;

    







    int coded_width, coded_height;

#if FF_API_ASPECT_EXTENDED
#define FF_ASPECT_EXTENDED 15
#endif

    




    int gop_size;

    






    enum AVPixelFormat pix_fmt;

    






    int me_method;

    






















    void (*draw_horiz_band)(struct AVCodecContext *s,
                            const AVFrame *src, int offset[AV_NUM_DATA_POINTERS],
                            int y, int type, int height);

    








    enum AVPixelFormat (*get_format)(struct AVCodecContext *s, const enum AVPixelFormat * fmt);

    





    int max_b_frames;

    






    float b_quant_factor;

    
    int rc_strategy;
#define FF_RC_STRATEGY_XVID 1

    int b_frame_strategy;

    




    float b_quant_offset;

    





    int has_b_frames;

    




    int mpeg_quant;

    






    float i_quant_factor;

    




    float i_quant_offset;

    




    float lumi_masking;

    




    float temporal_cplx_masking;

    




    float spatial_cplx_masking;

    




    float p_masking;

    




    float dark_masking;

    




    int slice_count;
    




     int prediction_method;
#define FF_PRED_LEFT   0
#define FF_PRED_PLANE  1
#define FF_PRED_MEDIAN 2

    




    int *slice_offset;

    






    AVRational sample_aspect_ratio;

    




    int me_cmp;
    




    int me_sub_cmp;
    




    int mb_cmp;
    




    int ildct_cmp;
#define FF_CMP_SAD    0
#define FF_CMP_SSE    1
#define FF_CMP_SATD   2
#define FF_CMP_DCT    3
#define FF_CMP_PSNR   4
#define FF_CMP_BIT    5
#define FF_CMP_RD     6
#define FF_CMP_ZERO   7
#define FF_CMP_VSAD   8
#define FF_CMP_VSSE   9
#define FF_CMP_NSSE   10
#define FF_CMP_DCTMAX 13
#define FF_CMP_DCT264 14
#define FF_CMP_CHROMA 256

    




    int dia_size;

    




    int last_predictor_count;

    




    int pre_me;

    




    int me_pre_cmp;

    




    int pre_dia_size;

    




    int me_subpel_quality;

    







    int dtg_active_format;
#define FF_DTG_AFD_SAME         8
#define FF_DTG_AFD_4_3          9
#define FF_DTG_AFD_16_9         10
#define FF_DTG_AFD_14_9         11
#define FF_DTG_AFD_4_3_SP_14_9  13
#define FF_DTG_AFD_16_9_SP_14_9 14
#define FF_DTG_AFD_SP_4_3       15

    






    int me_range;

    




    int intra_quant_bias;
#define FF_DEFAULT_QUANT_BIAS 999999

    




    int inter_quant_bias;

    




    int slice_flags;
#define SLICE_FLAG_CODED_ORDER    0x0001 ///< draw_horiz_band() is called in coded order instead of display
#define SLICE_FLAG_ALLOW_FIELD    0x0002 ///< allow draw_horiz_band() with field slices (MPEG2 field pics)
#define SLICE_FLAG_ALLOW_PLANE    0x0004 ///< allow draw_horiz_band() with 1 component at a time (SVQ1)

#if FF_API_XVMC
    





    attribute_deprecated int xvmc_acceleration;
#endif 

    




    int mb_decision;
#define FF_MB_DECISION_SIMPLE 0        ///< uses mb_cmp
#define FF_MB_DECISION_BITS   1        ///< chooses the one which needs the fewest bits
#define FF_MB_DECISION_RD     2        ///< rate distortion

    




    uint16_t *intra_matrix;

    




    uint16_t *inter_matrix;

    





    int scenechange_threshold;

    




    int noise_reduction;

    






    int me_threshold;

    




    int mb_threshold;

    




    int intra_dc_precision;

    




    int skip_top;

    




    int skip_bottom;

    





    float border_masking;

    




    int mb_lmin;

    




    int mb_lmax;

    




    int me_penalty_compensation;

    




    int bidir_refine;

    




    int brd_scale;

    




    int keyint_min;

    




    int refs;

    




    int chromaoffset;

    




    int scenechange_factor;

    





    int mv0_threshold;

    




    int b_sensitivity;

    




    enum AVColorPrimaries color_primaries;

    




    enum AVColorTransferCharacteristic color_trc;

    




    enum AVColorSpace colorspace;

    




    enum AVColorRange color_range;

    




    enum AVChromaLocation chroma_sample_location;

    






    int slices;

    



    enum AVFieldOrder field_order;

    
    int sample_rate; 
    int channels;    

    




    enum AVSampleFormat sample_fmt;  

    
    








    int frame_size;

    








    int frame_number;

    



    int block_align;

    




    int cutoff;

#if FF_API_REQUEST_CHANNELS
    





    attribute_deprecated int request_channels;
#endif

    




    uint64_t channel_layout;

    




    uint64_t request_channel_layout;

    




    enum AVAudioServiceType audio_service_type;

    




    enum AVSampleFormat request_sample_fmt;

#if FF_API_GET_BUFFER
    





























































    attribute_deprecated
    int (*get_buffer)(struct AVCodecContext *c, AVFrame *pic);

    










    attribute_deprecated
    void (*release_buffer)(struct AVCodecContext *c, AVFrame *pic);

    











    attribute_deprecated
    int (*reget_buffer)(struct AVCodecContext *c, AVFrame *pic);
#endif

    













































































    int (*get_buffer2)(struct AVCodecContext *s, AVFrame *frame, int flags);

    










    int refcounted_frames;

    
    float qcompress;  
    float qblur;      

    




    int qmin;

    




    int qmax;

    




    int max_qdiff;

    





    float rc_qsquish;

    float rc_qmod_amp;
    int rc_qmod_freq;

    




    int rc_buffer_size;

    




    int rc_override_count;
    RcOverride *rc_override;

    




    const char *rc_eq;

    




    int rc_max_rate;

    




    int rc_min_rate;

    float rc_buffer_aggressivity;

    




    float rc_initial_cplx;

    




    float rc_max_available_vbv_use;

    




    float rc_min_vbv_overflow_use;

    




    int rc_initial_buffer_occupancy;

#define FF_CODER_TYPE_VLC       0
#define FF_CODER_TYPE_AC        1
#define FF_CODER_TYPE_RAW       2
#define FF_CODER_TYPE_RLE       3
#define FF_CODER_TYPE_DEFLATE   4
    




    int coder_type;

    




    int context_model;

    




    int lmin;

    




    int lmax;

    




    int frame_skip_threshold;

    




    int frame_skip_factor;

    




    int frame_skip_exp;

    




    int frame_skip_cmp;

    




    int trellis;

    



    int min_prediction_order;

    



    int max_prediction_order;

    




    int64_t timecode_frame_start;

    
    
    
    
    
    
    void (*rtp_callback)(struct AVCodecContext *avctx, void *data, int size, int mb_nb);

    int rtp_payload_size;   
                            
                            
                            
                            
                            

    
    int mv_bits;
    int header_bits;
    int i_tex_bits;
    int p_tex_bits;
    int i_count;
    int p_count;
    int skip_count;
    int misc_bits;

    




    int frame_bits;

    




    char *stats_out;

    





    char *stats_in;

    




    int workaround_bugs;
#define FF_BUG_AUTODETECT       1  ///< autodetection
#if FF_API_OLD_MSMPEG4
#define FF_BUG_OLD_MSMPEG4      2
#endif
#define FF_BUG_XVID_ILACE       4
#define FF_BUG_UMP4             8
#define FF_BUG_NO_PADDING       16
#define FF_BUG_AMV              32
#if FF_API_AC_VLC
#define FF_BUG_AC_VLC           0  ///< Will be removed, libavcodec can now handle these non-compliant files by default.
#endif
#define FF_BUG_QPEL_CHROMA      64
#define FF_BUG_STD_QPEL         128
#define FF_BUG_QPEL_CHROMA2     256
#define FF_BUG_DIRECT_BLOCKSIZE 512
#define FF_BUG_EDGE             1024
#define FF_BUG_HPEL_CHROMA      2048
#define FF_BUG_DC_CLIP          4096
#define FF_BUG_MS               8192 ///< Work around various bugs in Microsoft's broken decoders.
#define FF_BUG_TRUNCATED       16384

    











    int strict_std_compliance;
#define FF_COMPLIANCE_VERY_STRICT   2 ///< Strictly conform to an older more strict version of the spec or reference software.
#define FF_COMPLIANCE_STRICT        1 ///< Strictly conform to all the things in the spec no matter what consequences.
#define FF_COMPLIANCE_NORMAL        0
#define FF_COMPLIANCE_UNOFFICIAL   -1 ///< Allow unofficial extensions
#define FF_COMPLIANCE_EXPERIMENTAL -2 ///< Allow nonstandardized experimental things.

    




    int error_concealment;
#define FF_EC_GUESS_MVS   1
#define FF_EC_DEBLOCK     2

    




    int debug;
#define FF_DEBUG_PICT_INFO   1
#define FF_DEBUG_RC          2
#define FF_DEBUG_BITSTREAM   4
#define FF_DEBUG_MB_TYPE     8
#define FF_DEBUG_QP          16
#if FF_API_DEBUG_MV



#define FF_DEBUG_MV          32
#endif
#define FF_DEBUG_DCT_COEFF   0x00000040
#define FF_DEBUG_SKIP        0x00000080
#define FF_DEBUG_STARTCODE   0x00000100
#define FF_DEBUG_PTS         0x00000200
#define FF_DEBUG_ER          0x00000400
#define FF_DEBUG_MMCO        0x00000800
#define FF_DEBUG_BUGS        0x00001000
#if FF_API_DEBUG_MV
#define FF_DEBUG_VIS_QP      0x00002000
#define FF_DEBUG_VIS_MB_TYPE 0x00004000
#endif
#define FF_DEBUG_BUFFERS     0x00008000
#define FF_DEBUG_THREADS     0x00010000

#if FF_API_DEBUG_MV
    


    attribute_deprecated
    int debug_mv;
#define FF_DEBUG_VIS_MV_P_FOR  0x00000001 //visualize forward predicted MVs of P frames
#define FF_DEBUG_VIS_MV_B_FOR  0x00000002 //visualize forward predicted MVs of B frames
#define FF_DEBUG_VIS_MV_B_BACK 0x00000004 //visualize backward predicted MVs of B frames
#endif

    




    int err_recognition;







#define AV_EF_CRCCHECK  (1<<0)
#define AV_EF_BITSTREAM (1<<1)
#define AV_EF_BUFFER    (1<<2)
#define AV_EF_EXPLODE   (1<<3)

    






    int64_t reordered_opaque;

    




    struct AVHWAccel *hwaccel;

    









    void *hwaccel_context;

    




    uint64_t error[AV_NUM_DATA_POINTERS];

    




    int dct_algo;
#define FF_DCT_AUTO    0
#define FF_DCT_FASTINT 1
#define FF_DCT_INT     2
#define FF_DCT_MMX     3
#define FF_DCT_ALTIVEC 5
#define FF_DCT_FAAN    6

    




    int idct_algo;
#define FF_IDCT_AUTO          0
#define FF_IDCT_INT           1
#define FF_IDCT_SIMPLE        2
#define FF_IDCT_SIMPLEMMX     3
#define FF_IDCT_ARM           7
#define FF_IDCT_ALTIVEC       8
#define FF_IDCT_SH4           9
#define FF_IDCT_SIMPLEARM     10
#define FF_IDCT_IPP           13
#define FF_IDCT_XVIDMMX       14
#define FF_IDCT_SIMPLEARMV5TE 16
#define FF_IDCT_SIMPLEARMV6   17
#define FF_IDCT_SIMPLEVIS     18
#define FF_IDCT_FAAN          20
#define FF_IDCT_SIMPLENEON    22
#if FF_API_ARCH_ALPHA
#define FF_IDCT_SIMPLEALPHA   23
#endif

    




     int bits_per_coded_sample;

    




    int bits_per_raw_sample;

#if FF_API_LOWRES
    






    attribute_deprecated int lowres;
#endif

    




    AVFrame *coded_frame;

    





    int thread_count;

    







    int thread_type;
#define FF_THREAD_FRAME   1 ///< Decode more than one frame at once
#define FF_THREAD_SLICE   2 ///< Decode more than one part of a single frame at once

    




    int active_thread_type;

    







    int thread_safe_callbacks;

    








    int (*execute)(struct AVCodecContext *c, int (*func)(struct AVCodecContext *c2, void *arg), void *arg2, int *ret, int count, int size);

    

















    int (*execute2)(struct AVCodecContext *c, int (*func)(struct AVCodecContext *c2, void *arg, int jobnr, int threadnr), void *arg2, int *ret, int count);

#if FF_API_THREAD_OPAQUE
    


    attribute_deprecated
    void *thread_opaque;
#endif

    




     int nsse_weight;

    




     int profile;
#define FF_PROFILE_UNKNOWN -99
#define FF_PROFILE_RESERVED -100

#define FF_PROFILE_AAC_MAIN 0
#define FF_PROFILE_AAC_LOW  1
#define FF_PROFILE_AAC_SSR  2
#define FF_PROFILE_AAC_LTP  3
#define FF_PROFILE_AAC_HE   4
#define FF_PROFILE_AAC_HE_V2 28
#define FF_PROFILE_AAC_LD   22
#define FF_PROFILE_AAC_ELD  38
#define FF_PROFILE_MPEG2_AAC_LOW 128
#define FF_PROFILE_MPEG2_AAC_HE  131

#define FF_PROFILE_DTS         20
#define FF_PROFILE_DTS_ES      30
#define FF_PROFILE_DTS_96_24   40
#define FF_PROFILE_DTS_HD_HRA  50
#define FF_PROFILE_DTS_HD_MA   60

#define FF_PROFILE_MPEG2_422    0
#define FF_PROFILE_MPEG2_HIGH   1
#define FF_PROFILE_MPEG2_SS     2
#define FF_PROFILE_MPEG2_SNR_SCALABLE  3
#define FF_PROFILE_MPEG2_MAIN   4
#define FF_PROFILE_MPEG2_SIMPLE 5

#define FF_PROFILE_H264_CONSTRAINED  (1<<9)  // 8+1; constraint_set1_flag
#define FF_PROFILE_H264_INTRA        (1<<11) // 8+3; constraint_set3_flag

#define FF_PROFILE_H264_BASELINE             66
#define FF_PROFILE_H264_CONSTRAINED_BASELINE (66|FF_PROFILE_H264_CONSTRAINED)
#define FF_PROFILE_H264_MAIN                 77
#define FF_PROFILE_H264_EXTENDED             88
#define FF_PROFILE_H264_HIGH                 100
#define FF_PROFILE_H264_HIGH_10              110
#define FF_PROFILE_H264_HIGH_10_INTRA        (110|FF_PROFILE_H264_INTRA)
#define FF_PROFILE_H264_HIGH_422             122
#define FF_PROFILE_H264_HIGH_422_INTRA       (122|FF_PROFILE_H264_INTRA)
#define FF_PROFILE_H264_HIGH_444             144
#define FF_PROFILE_H264_HIGH_444_PREDICTIVE  244
#define FF_PROFILE_H264_HIGH_444_INTRA       (244|FF_PROFILE_H264_INTRA)
#define FF_PROFILE_H264_CAVLC_444            44

#define FF_PROFILE_VC1_SIMPLE   0
#define FF_PROFILE_VC1_MAIN     1
#define FF_PROFILE_VC1_COMPLEX  2
#define FF_PROFILE_VC1_ADVANCED 3

#define FF_PROFILE_MPEG4_SIMPLE                     0
#define FF_PROFILE_MPEG4_SIMPLE_SCALABLE            1
#define FF_PROFILE_MPEG4_CORE                       2
#define FF_PROFILE_MPEG4_MAIN                       3
#define FF_PROFILE_MPEG4_N_BIT                      4
#define FF_PROFILE_MPEG4_SCALABLE_TEXTURE           5
#define FF_PROFILE_MPEG4_SIMPLE_FACE_ANIMATION      6
#define FF_PROFILE_MPEG4_BASIC_ANIMATED_TEXTURE     7
#define FF_PROFILE_MPEG4_HYBRID                     8
#define FF_PROFILE_MPEG4_ADVANCED_REAL_TIME         9
#define FF_PROFILE_MPEG4_CORE_SCALABLE             10
#define FF_PROFILE_MPEG4_ADVANCED_CODING           11
#define FF_PROFILE_MPEG4_ADVANCED_CORE             12
#define FF_PROFILE_MPEG4_ADVANCED_SCALABLE_TEXTURE 13
#define FF_PROFILE_MPEG4_SIMPLE_STUDIO             14
#define FF_PROFILE_MPEG4_ADVANCED_SIMPLE           15

#define FF_PROFILE_JPEG2000_CSTREAM_RESTRICTION_0   0
#define FF_PROFILE_JPEG2000_CSTREAM_RESTRICTION_1   1
#define FF_PROFILE_JPEG2000_CSTREAM_NO_RESTRICTION  2
#define FF_PROFILE_JPEG2000_DCINEMA_2K              3
#define FF_PROFILE_JPEG2000_DCINEMA_4K              4


#define FF_PROFILE_HEVC_MAIN                        1
#define FF_PROFILE_HEVC_MAIN_10                     2
#define FF_PROFILE_HEVC_MAIN_STILL_PICTURE          3

    




     int level;
#define FF_LEVEL_UNKNOWN -99

    




    enum AVDiscard skip_loop_filter;

    




    enum AVDiscard skip_idct;

    




    enum AVDiscard skip_frame;

    







    uint8_t *subtitle_header;
    int subtitle_header_size;

#if FF_API_ERROR_RATE
    



    attribute_deprecated
    int error_rate;
#endif

#if FF_API_CODEC_PKT
    


    attribute_deprecated
    AVPacket *pkt;
#endif

    





    uint64_t vbv_delay;
} AVCodecContext;




typedef struct AVProfile {
    int profile;
    const char *name; 
} AVProfile;

typedef struct AVCodecDefault AVCodecDefault;

struct AVSubtitle;




typedef struct AVCodec {
    





    const char *name;
    



    const char *long_name;
    enum AVMediaType type;
    enum AVCodecID id;
    



    int capabilities;
    const AVRational *supported_framerates; 
    const enum AVPixelFormat *pix_fmts;     
    const int *supported_samplerates;       
    const enum AVSampleFormat *sample_fmts; 
    const uint64_t *channel_layouts;         
#if FF_API_LOWRES
    attribute_deprecated uint8_t max_lowres; 
#endif
    const AVClass *priv_class;              
    const AVProfile *profiles;              

    






    int priv_data_size;
    struct AVCodec *next;
    



    




    int (*init_thread_copy)(AVCodecContext *);
    






    int (*update_thread_context)(AVCodecContext *dst, const AVCodecContext *src);
    

    


    const AVCodecDefault *defaults;

    


    void (*init_static_data)(struct AVCodec *codec);

    int (*init)(AVCodecContext *);
    int (*encode_sub)(AVCodecContext *, uint8_t *buf, int buf_size,
                      const struct AVSubtitle *sub);
    









    int (*encode2)(AVCodecContext *avctx, AVPacket *avpkt, const AVFrame *frame,
                   int *got_packet_ptr);
    int (*decode)(AVCodecContext *, void *outdata, int *outdata_size, AVPacket *avpkt);
    int (*close)(AVCodecContext *);
    



    void (*flush)(AVCodecContext *);
} AVCodec;




typedef struct AVHWAccel {
    




    const char *name;

    




    enum AVMediaType type;

    




    enum AVCodecID id;

    




    enum AVPixelFormat pix_fmt;

    



    int capabilities;

    struct AVHWAccel *next;

    













    int (*start_frame)(AVCodecContext *avctx, const uint8_t *buf, uint32_t buf_size);

    










    int (*decode_slice)(AVCodecContext *avctx, const uint8_t *buf, uint32_t buf_size);

    








    int (*end_frame)(AVCodecContext *avctx);

    






    int priv_data_size;
} AVHWAccel;












typedef struct AVPicture {
    uint8_t *data[AV_NUM_DATA_POINTERS];
    int linesize[AV_NUM_DATA_POINTERS];     
} AVPicture;





#define AVPALETTE_SIZE 1024
#define AVPALETTE_COUNT 256

enum AVSubtitleType {
    SUBTITLE_NONE,

    SUBTITLE_BITMAP,                

    



    SUBTITLE_TEXT,

    



    SUBTITLE_ASS,
};

#define AV_SUBTITLE_FLAG_FORCED 0x00000001

typedef struct AVSubtitleRect {
    int x;         
    int y;         
    int w;         
    int h;         
    int nb_colors; 

    



    AVPicture pict;
    enum AVSubtitleType type;

    char *text;                     

    




    char *ass;
    int flags;
} AVSubtitleRect;

typedef struct AVSubtitle {
    uint16_t format; 
    uint32_t start_display_time; 
    uint32_t end_display_time; 
    unsigned num_rects;
    AVSubtitleRect **rects;
    int64_t pts;    
} AVSubtitle;






AVCodec *av_codec_next(const AVCodec *c);




unsigned avcodec_version(void);




const char *avcodec_configuration(void);




const char *avcodec_license(void);









void avcodec_register(AVCodec *codec);











void avcodec_register_all(void);
















AVCodecContext *avcodec_alloc_context3(const AVCodec *codec);










int avcodec_get_context_defaults3(AVCodecContext *s, const AVCodec *codec);







const AVClass *avcodec_get_class(void);












int avcodec_copy_context(AVCodecContext *dest, const AVCodecContext *src);

#if FF_API_AVFRAME_LAVC



attribute_deprecated
AVFrame *avcodec_alloc_frame(void);








attribute_deprecated
void avcodec_get_frame_defaults(AVFrame *frame);













attribute_deprecated
void avcodec_free_frame(AVFrame **frame);
#endif





































int avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);










int avcodec_close(AVCodecContext *avctx);






void avsubtitle_free(AVSubtitle *sub);










#if FF_API_DESTRUCT_PACKET




attribute_deprecated
void av_destruct_packet(AVPacket *pkt);
#endif









void av_init_packet(AVPacket *pkt);









int av_new_packet(AVPacket *pkt, int size);







void av_shrink_packet(AVPacket *pkt, int size);







int av_grow_packet(AVPacket *pkt, int grow_by);














int av_packet_from_data(AVPacket *pkt, uint8_t *data, int size);





int av_dup_packet(AVPacket *pkt);






void av_free_packet(AVPacket *pkt);









uint8_t* av_packet_new_side_data(AVPacket *pkt, enum AVPacketSideDataType type,
                                 int size);









int av_packet_shrink_side_data(AVPacket *pkt, enum AVPacketSideDataType type,
                               int size);









uint8_t* av_packet_get_side_data(AVPacket *pkt, enum AVPacketSideDataType type,
                                 int *size);







void av_packet_free_side_data(AVPacket *pkt);

















int av_packet_ref(AVPacket *dst, AVPacket *src);









void av_packet_unref(AVPacket *pkt);









void av_packet_move_ref(AVPacket *dst, AVPacket *src);













int av_packet_copy_props(AVPacket *dst, const AVPacket *src);
















AVCodec *avcodec_find_decoder(enum AVCodecID id);







AVCodec *avcodec_find_decoder_by_name(const char *name);

#if FF_API_GET_BUFFER
attribute_deprecated int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic);
attribute_deprecated void avcodec_default_release_buffer(AVCodecContext *s, AVFrame *pic);
attribute_deprecated int avcodec_default_reget_buffer(AVCodecContext *s, AVFrame *pic);
#endif






int avcodec_default_get_buffer2(AVCodecContext *s, AVFrame *frame, int flags);

#if FF_API_EMU_EDGE










attribute_deprecated
unsigned avcodec_get_edge_width(void);
#endif








void avcodec_align_dimensions(AVCodecContext *s, int *width, int *height);








void avcodec_align_dimensions2(AVCodecContext *s, int *width, int *height,
                               int linesize_align[AV_NUM_DATA_POINTERS]);

















































int avcodec_decode_audio4(AVCodecContext *avctx, AVFrame *frame,
                          int *got_frame_ptr, AVPacket *avpkt);









































int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture,
                         int *got_picture_ptr,
                         AVPacket *avpkt);

















int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub,
                            int *got_sub_ptr,
                            AVPacket *avpkt);






enum AVPictureStructure {
    AV_PICTURE_STRUCTURE_UNKNOWN,      
    AV_PICTURE_STRUCTURE_TOP_FIELD,    
    AV_PICTURE_STRUCTURE_BOTTOM_FIELD, 
    AV_PICTURE_STRUCTURE_FRAME,        
};

typedef struct AVCodecParserContext {
    void *priv_data;
    struct AVCodecParser *parser;
    int64_t frame_offset; 
    int64_t cur_offset; 

    int64_t next_frame_offset; 
    
    int pict_type; 
    








    int repeat_pict; 
    int64_t pts;     
    int64_t dts;     

    
    int64_t last_pts;
    int64_t last_dts;
    int fetch_timestamp;

#define AV_PARSER_PTS_NB 4
    int cur_frame_start_index;
    int64_t cur_frame_offset[AV_PARSER_PTS_NB];
    int64_t cur_frame_pts[AV_PARSER_PTS_NB];
    int64_t cur_frame_dts[AV_PARSER_PTS_NB];

    int flags;
#define PARSER_FLAG_COMPLETE_FRAMES           0x0001
#define PARSER_FLAG_ONCE                      0x0002

#define PARSER_FLAG_FETCHED_OFFSET            0x0004

    int64_t offset;      
    int64_t cur_frame_end[AV_PARSER_PTS_NB];

    





    int key_frame;

    
















    int64_t convergence_duration;

    
    








    int dts_sync_point;

    












    int dts_ref_dts_delta;

    











    int pts_dts_delta;

    




    int64_t cur_frame_pos[AV_PARSER_PTS_NB];

    


    int64_t pos;

    


    int64_t last_pos;

    




    int duration;

    enum AVFieldOrder field_order;

    







    enum AVPictureStructure picture_structure;

    





    int output_picture_number;
} AVCodecParserContext;

typedef struct AVCodecParser {
    int codec_ids[5]; 
    int priv_data_size;
    int (*parser_init)(AVCodecParserContext *s);
    int (*parser_parse)(AVCodecParserContext *s,
                        AVCodecContext *avctx,
                        const uint8_t **poutbuf, int *poutbuf_size,
                        const uint8_t *buf, int buf_size);
    void (*parser_close)(AVCodecParserContext *s);
    int (*split)(AVCodecContext *avctx, const uint8_t *buf, int buf_size);
    struct AVCodecParser *next;
} AVCodecParser;

AVCodecParser *av_parser_next(AVCodecParser *c);

void av_register_codec_parser(AVCodecParser *parser);
AVCodecParserContext *av_parser_init(int codec_id);





























int av_parser_parse2(AVCodecParserContext *s,
                     AVCodecContext *avctx,
                     uint8_t **poutbuf, int *poutbuf_size,
                     const uint8_t *buf, int buf_size,
                     int64_t pts, int64_t dts,
                     int64_t pos);





int av_parser_change(AVCodecParserContext *s,
                     AVCodecContext *avctx,
                     uint8_t **poutbuf, int *poutbuf_size,
                     const uint8_t *buf, int buf_size, int keyframe);
void av_parser_close(AVCodecParserContext *s);

















AVCodec *avcodec_find_encoder(enum AVCodecID id);







AVCodec *avcodec_find_encoder_by_name(const char *name);







































int avcodec_encode_audio2(AVCodecContext *avctx, AVPacket *avpkt,
                          const AVFrame *frame, int *got_packet_ptr);



































int avcodec_encode_video2(AVCodecContext *avctx, AVPacket *avpkt,
                          const AVFrame *frame, int *got_packet_ptr);

int avcodec_encode_subtitle(AVCodecContext *avctx, uint8_t *buf, int buf_size,
                            const AVSubtitle *sub);






















int avpicture_alloc(AVPicture *picture, enum AVPixelFormat pix_fmt, int width, int height);








void avpicture_free(AVPicture *picture);





















int avpicture_fill(AVPicture *picture, uint8_t *ptr,
                   enum AVPixelFormat pix_fmt, int width, int height);
















int avpicture_layout(const AVPicture* src, enum AVPixelFormat pix_fmt,
                     int width, int height,
                     unsigned char *dest, int dest_size);













int avpicture_get_size(enum AVPixelFormat pix_fmt, int width, int height);

#if FF_API_DEINTERLACE





attribute_deprecated
int avpicture_deinterlace(AVPicture *dst, const AVPicture *src,
                          enum AVPixelFormat pix_fmt, int width, int height);
#endif



void av_picture_copy(AVPicture *dst, const AVPicture *src,
                     enum AVPixelFormat pix_fmt, int width, int height);




int av_picture_crop(AVPicture *dst, const AVPicture *src,
                    enum AVPixelFormat pix_fmt, int top_band, int left_band);




int av_picture_pad(AVPicture *dst, const AVPicture *src, int height, int width, enum AVPixelFormat pix_fmt,
            int padtop, int padbottom, int padleft, int padright, int *color);

























void attribute_deprecated avcodec_get_chroma_sub_sample(enum AVPixelFormat pix_fmt, int *h_shift, int *v_shift);






unsigned int avcodec_pix_fmt_to_codec_tag(enum AVPixelFormat pix_fmt);

#define FF_LOSS_RESOLUTION  0x0001 /**< loss due to resolution change */
#define FF_LOSS_DEPTH       0x0002 /**< loss due to color depth change */
#define FF_LOSS_COLORSPACE  0x0004 /**< loss due to color space conversion */
#define FF_LOSS_ALPHA       0x0008 /**< loss of alpha bits */
#define FF_LOSS_COLORQUANT  0x0010 /**< loss due to color quantization */
#define FF_LOSS_CHROMA      0x0020 /**< loss of chroma (e.g. RGB to gray conversion) */


















int avcodec_get_pix_fmt_loss(enum AVPixelFormat dst_pix_fmt, enum AVPixelFormat src_pix_fmt,
                             int has_alpha);


















enum AVPixelFormat avcodec_find_best_pix_fmt2(enum AVPixelFormat *pix_fmt_list,
                                              enum AVPixelFormat src_pix_fmt,
                                              int has_alpha, int *loss_ptr);

enum AVPixelFormat avcodec_default_get_format(struct AVCodecContext *s, const enum AVPixelFormat * fmt);





#if FF_API_SET_DIMENSIONS



attribute_deprecated
void avcodec_set_dimensions(AVCodecContext *s, int width, int height);
#endif










size_t av_get_codec_tag_string(char *buf, size_t buf_size, unsigned int codec_tag);

void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode);








const char *av_get_profile_name(const AVCodec *codec, int profile);

int avcodec_default_execute(AVCodecContext *c, int (*func)(AVCodecContext *c2, void *arg2),void *arg, int *ret, int count, int size);
int avcodec_default_execute2(AVCodecContext *c, int (*func)(AVCodecContext *c2, void *arg2, int, int),void *arg, int *ret, int count);


















int avcodec_fill_audio_frame(AVFrame *frame, int nb_channels,
                             enum AVSampleFormat sample_fmt, const uint8_t *buf,
                             int buf_size, int align);










void avcodec_flush_buffers(AVCodecContext *avctx);







int av_get_bits_per_sample(enum AVCodecID codec_id);









int av_get_exact_bits_per_sample(enum AVCodecID codec_id);









int av_get_audio_frame_duration(AVCodecContext *avctx, int frame_bytes);


typedef struct AVBitStreamFilterContext {
    void *priv_data;
    struct AVBitStreamFilter *filter;
    AVCodecParserContext *parser;
    struct AVBitStreamFilterContext *next;
} AVBitStreamFilterContext;


typedef struct AVBitStreamFilter {
    const char *name;
    int priv_data_size;
    int (*filter)(AVBitStreamFilterContext *bsfc,
                  AVCodecContext *avctx, const char *args,
                  uint8_t **poutbuf, int *poutbuf_size,
                  const uint8_t *buf, int buf_size, int keyframe);
    void (*close)(AVBitStreamFilterContext *bsfc);
    struct AVBitStreamFilter *next;
} AVBitStreamFilter;

void av_register_bitstream_filter(AVBitStreamFilter *bsf);
AVBitStreamFilterContext *av_bitstream_filter_init(const char *name);
int av_bitstream_filter_filter(AVBitStreamFilterContext *bsfc,
                               AVCodecContext *avctx, const char *args,
                               uint8_t **poutbuf, int *poutbuf_size,
                               const uint8_t *buf, int buf_size, int keyframe);
void av_bitstream_filter_close(AVBitStreamFilterContext *bsf);

AVBitStreamFilter *av_bitstream_filter_next(AVBitStreamFilter *f);










void av_fast_padded_malloc(void *ptr, unsigned int *size, size_t min_size);








unsigned int av_xiphlacing(unsigned char *s, unsigned int v);

#if FF_API_MISSING_SAMPLE













attribute_deprecated
void av_log_missing_feature(void *avc, const char *feature, int want_sample);










attribute_deprecated
void av_log_ask_for_sample(void *avc, const char *msg, ...) av_printf_format(2, 3);
#endif 




void av_register_hwaccel(AVHWAccel *hwaccel);






AVHWAccel *av_hwaccel_next(AVHWAccel *hwaccel);





enum AVLockOp {
  AV_LOCK_CREATE,  
  AV_LOCK_OBTAIN,  
  AV_LOCK_RELEASE, 
  AV_LOCK_DESTROY, 
};














int av_lockmgr_register(int (*cb)(void **mutex, enum AVLockOp op));




enum AVMediaType avcodec_get_type(enum AVCodecID codec_id);





int avcodec_is_open(AVCodecContext *s);




int av_codec_is_encoder(const AVCodec *codec);




int av_codec_is_decoder(const AVCodec *codec);




const AVCodecDescriptor *avcodec_descriptor_get(enum AVCodecID id);








const AVCodecDescriptor *avcodec_descriptor_next(const AVCodecDescriptor *prev);





const AVCodecDescriptor *avcodec_descriptor_get_by_name(const char *name);





#endif 
