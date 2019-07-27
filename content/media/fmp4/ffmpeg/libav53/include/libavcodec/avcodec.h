



















#ifndef AVCODEC_AVCODEC_H
#define AVCODEC_AVCODEC_H






#include <errno.h>
#include "libavutil/samplefmt.h"
#include "libavutil/avutil.h"
#include "libavutil/cpu.h"
#include "libavutil/dict.h"
#include "libavutil/log.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"

#include "libavcodec/version.h"












































enum CodecID {
    CODEC_ID_NONE,

    
    CODEC_ID_MPEG1VIDEO,
    CODEC_ID_MPEG2VIDEO, 
    CODEC_ID_MPEG2VIDEO_XVMC,
    CODEC_ID_H261,
    CODEC_ID_H263,
    CODEC_ID_RV10,
    CODEC_ID_RV20,
    CODEC_ID_MJPEG,
    CODEC_ID_MJPEGB,
    CODEC_ID_LJPEG,
    CODEC_ID_SP5X,
    CODEC_ID_JPEGLS,
    CODEC_ID_MPEG4,
    CODEC_ID_RAWVIDEO,
    CODEC_ID_MSMPEG4V1,
    CODEC_ID_MSMPEG4V2,
    CODEC_ID_MSMPEG4V3,
    CODEC_ID_WMV1,
    CODEC_ID_WMV2,
    CODEC_ID_H263P,
    CODEC_ID_H263I,
    CODEC_ID_FLV1,
    CODEC_ID_SVQ1,
    CODEC_ID_SVQ3,
    CODEC_ID_DVVIDEO,
    CODEC_ID_HUFFYUV,
    CODEC_ID_CYUV,
    CODEC_ID_H264,
    CODEC_ID_INDEO3,
    CODEC_ID_VP3,
    CODEC_ID_THEORA,
    CODEC_ID_ASV1,
    CODEC_ID_ASV2,
    CODEC_ID_FFV1,
    CODEC_ID_4XM,
    CODEC_ID_VCR1,
    CODEC_ID_CLJR,
    CODEC_ID_MDEC,
    CODEC_ID_ROQ,
    CODEC_ID_INTERPLAY_VIDEO,
    CODEC_ID_XAN_WC3,
    CODEC_ID_XAN_WC4,
    CODEC_ID_RPZA,
    CODEC_ID_CINEPAK,
    CODEC_ID_WS_VQA,
    CODEC_ID_MSRLE,
    CODEC_ID_MSVIDEO1,
    CODEC_ID_IDCIN,
    CODEC_ID_8BPS,
    CODEC_ID_SMC,
    CODEC_ID_FLIC,
    CODEC_ID_TRUEMOTION1,
    CODEC_ID_VMDVIDEO,
    CODEC_ID_MSZH,
    CODEC_ID_ZLIB,
    CODEC_ID_QTRLE,
    CODEC_ID_SNOW,
    CODEC_ID_TSCC,
    CODEC_ID_ULTI,
    CODEC_ID_QDRAW,
    CODEC_ID_VIXL,
    CODEC_ID_QPEG,
    CODEC_ID_PNG,
    CODEC_ID_PPM,
    CODEC_ID_PBM,
    CODEC_ID_PGM,
    CODEC_ID_PGMYUV,
    CODEC_ID_PAM,
    CODEC_ID_FFVHUFF,
    CODEC_ID_RV30,
    CODEC_ID_RV40,
    CODEC_ID_VC1,
    CODEC_ID_WMV3,
    CODEC_ID_LOCO,
    CODEC_ID_WNV1,
    CODEC_ID_AASC,
    CODEC_ID_INDEO2,
    CODEC_ID_FRAPS,
    CODEC_ID_TRUEMOTION2,
    CODEC_ID_BMP,
    CODEC_ID_CSCD,
    CODEC_ID_MMVIDEO,
    CODEC_ID_ZMBV,
    CODEC_ID_AVS,
    CODEC_ID_SMACKVIDEO,
    CODEC_ID_NUV,
    CODEC_ID_KMVC,
    CODEC_ID_FLASHSV,
    CODEC_ID_CAVS,
    CODEC_ID_JPEG2000,
    CODEC_ID_VMNC,
    CODEC_ID_VP5,
    CODEC_ID_VP6,
    CODEC_ID_VP6F,
    CODEC_ID_TARGA,
    CODEC_ID_DSICINVIDEO,
    CODEC_ID_TIERTEXSEQVIDEO,
    CODEC_ID_TIFF,
    CODEC_ID_GIF,
#if LIBAVCODEC_VERSION_MAJOR == 53
    CODEC_ID_FFH264,
#endif
    CODEC_ID_DXA,
    CODEC_ID_DNXHD,
    CODEC_ID_THP,
    CODEC_ID_SGI,
    CODEC_ID_C93,
    CODEC_ID_BETHSOFTVID,
    CODEC_ID_PTX,
    CODEC_ID_TXD,
    CODEC_ID_VP6A,
    CODEC_ID_AMV,
    CODEC_ID_VB,
    CODEC_ID_PCX,
    CODEC_ID_SUNRAST,
    CODEC_ID_INDEO4,
    CODEC_ID_INDEO5,
    CODEC_ID_MIMIC,
    CODEC_ID_RL2,
#if LIBAVCODEC_VERSION_MAJOR == 53
    CODEC_ID_8SVX_EXP,
    CODEC_ID_8SVX_FIB,
#endif
    CODEC_ID_ESCAPE124,
    CODEC_ID_DIRAC,
    CODEC_ID_BFI,
    CODEC_ID_CMV,
    CODEC_ID_MOTIONPIXELS,
    CODEC_ID_TGV,
    CODEC_ID_TGQ,
    CODEC_ID_TQI,
    CODEC_ID_AURA,
    CODEC_ID_AURA2,
    CODEC_ID_V210X,
    CODEC_ID_TMV,
    CODEC_ID_V210,
    CODEC_ID_DPX,
    CODEC_ID_MAD,
    CODEC_ID_FRWU,
    CODEC_ID_FLASHSV2,
    CODEC_ID_CDGRAPHICS,
    CODEC_ID_R210,
    CODEC_ID_ANM,
    CODEC_ID_BINKVIDEO,
    CODEC_ID_IFF_ILBM,
    CODEC_ID_IFF_BYTERUN1,
    CODEC_ID_KGV1,
    CODEC_ID_YOP,
    CODEC_ID_VP8,
    CODEC_ID_PICTOR,
    CODEC_ID_ANSI,
    CODEC_ID_A64_MULTI,
    CODEC_ID_A64_MULTI5,
    CODEC_ID_R10K,
    CODEC_ID_MXPEG,
    CODEC_ID_LAGARITH,
    CODEC_ID_PRORES,
    CODEC_ID_JV,
    CODEC_ID_DFA,
    CODEC_ID_WMV3IMAGE,
    CODEC_ID_VC1IMAGE,
#if LIBAVCODEC_VERSION_MAJOR == 53
    CODEC_ID_G723_1,
    CODEC_ID_G729,
#endif
    CODEC_ID_UTVIDEO,
    CODEC_ID_BMV_VIDEO,
    CODEC_ID_VBLE,
    CODEC_ID_DXTORY,
    CODEC_ID_V410,

    
    CODEC_ID_FIRST_AUDIO = 0x10000,     
    CODEC_ID_PCM_S16LE = 0x10000,
    CODEC_ID_PCM_S16BE,
    CODEC_ID_PCM_U16LE,
    CODEC_ID_PCM_U16BE,
    CODEC_ID_PCM_S8,
    CODEC_ID_PCM_U8,
    CODEC_ID_PCM_MULAW,
    CODEC_ID_PCM_ALAW,
    CODEC_ID_PCM_S32LE,
    CODEC_ID_PCM_S32BE,
    CODEC_ID_PCM_U32LE,
    CODEC_ID_PCM_U32BE,
    CODEC_ID_PCM_S24LE,
    CODEC_ID_PCM_S24BE,
    CODEC_ID_PCM_U24LE,
    CODEC_ID_PCM_U24BE,
    CODEC_ID_PCM_S24DAUD,
    CODEC_ID_PCM_ZORK,
    CODEC_ID_PCM_S16LE_PLANAR,
    CODEC_ID_PCM_DVD,
    CODEC_ID_PCM_F32BE,
    CODEC_ID_PCM_F32LE,
    CODEC_ID_PCM_F64BE,
    CODEC_ID_PCM_F64LE,
    CODEC_ID_PCM_BLURAY,
    CODEC_ID_PCM_LXF,
    CODEC_ID_S302M,
    CODEC_ID_PCM_S8_PLANAR,

    
    CODEC_ID_ADPCM_IMA_QT = 0x11000,
    CODEC_ID_ADPCM_IMA_WAV,
    CODEC_ID_ADPCM_IMA_DK3,
    CODEC_ID_ADPCM_IMA_DK4,
    CODEC_ID_ADPCM_IMA_WS,
    CODEC_ID_ADPCM_IMA_SMJPEG,
    CODEC_ID_ADPCM_MS,
    CODEC_ID_ADPCM_4XM,
    CODEC_ID_ADPCM_XA,
    CODEC_ID_ADPCM_ADX,
    CODEC_ID_ADPCM_EA,
    CODEC_ID_ADPCM_G726,
    CODEC_ID_ADPCM_CT,
    CODEC_ID_ADPCM_SWF,
    CODEC_ID_ADPCM_YAMAHA,
    CODEC_ID_ADPCM_SBPRO_4,
    CODEC_ID_ADPCM_SBPRO_3,
    CODEC_ID_ADPCM_SBPRO_2,
    CODEC_ID_ADPCM_THP,
    CODEC_ID_ADPCM_IMA_AMV,
    CODEC_ID_ADPCM_EA_R1,
    CODEC_ID_ADPCM_EA_R3,
    CODEC_ID_ADPCM_EA_R2,
    CODEC_ID_ADPCM_IMA_EA_SEAD,
    CODEC_ID_ADPCM_IMA_EA_EACS,
    CODEC_ID_ADPCM_EA_XAS,
    CODEC_ID_ADPCM_EA_MAXIS_XA,
    CODEC_ID_ADPCM_IMA_ISS,
    CODEC_ID_ADPCM_G722,

    
    CODEC_ID_AMR_NB = 0x12000,
    CODEC_ID_AMR_WB,

    
    CODEC_ID_RA_144 = 0x13000,
    CODEC_ID_RA_288,

    
    CODEC_ID_ROQ_DPCM = 0x14000,
    CODEC_ID_INTERPLAY_DPCM,
    CODEC_ID_XAN_DPCM,
    CODEC_ID_SOL_DPCM,

    
    CODEC_ID_MP2 = 0x15000,
    CODEC_ID_MP3, 
    CODEC_ID_AAC,
    CODEC_ID_AC3,
    CODEC_ID_DTS,
    CODEC_ID_VORBIS,
    CODEC_ID_DVAUDIO,
    CODEC_ID_WMAV1,
    CODEC_ID_WMAV2,
    CODEC_ID_MACE3,
    CODEC_ID_MACE6,
    CODEC_ID_VMDAUDIO,
#if LIBAVCODEC_VERSION_MAJOR == 53
    CODEC_ID_SONIC,
    CODEC_ID_SONIC_LS,
#endif
    CODEC_ID_FLAC,
    CODEC_ID_MP3ADU,
    CODEC_ID_MP3ON4,
    CODEC_ID_SHORTEN,
    CODEC_ID_ALAC,
    CODEC_ID_WESTWOOD_SND1,
    CODEC_ID_GSM, 
    CODEC_ID_QDM2,
    CODEC_ID_COOK,
    CODEC_ID_TRUESPEECH,
    CODEC_ID_TTA,
    CODEC_ID_SMACKAUDIO,
    CODEC_ID_QCELP,
    CODEC_ID_WAVPACK,
    CODEC_ID_DSICINAUDIO,
    CODEC_ID_IMC,
    CODEC_ID_MUSEPACK7,
    CODEC_ID_MLP,
    CODEC_ID_GSM_MS, 
    CODEC_ID_ATRAC3,
    CODEC_ID_VOXWARE,
    CODEC_ID_APE,
    CODEC_ID_NELLYMOSER,
    CODEC_ID_MUSEPACK8,
    CODEC_ID_SPEEX,
    CODEC_ID_WMAVOICE,
    CODEC_ID_WMAPRO,
    CODEC_ID_WMALOSSLESS,
    CODEC_ID_ATRAC3P,
    CODEC_ID_EAC3,
    CODEC_ID_SIPR,
    CODEC_ID_MP1,
    CODEC_ID_TWINVQ,
    CODEC_ID_TRUEHD,
    CODEC_ID_MP4ALS,
    CODEC_ID_ATRAC1,
    CODEC_ID_BINKAUDIO_RDFT,
    CODEC_ID_BINKAUDIO_DCT,
    CODEC_ID_AAC_LATM,
    CODEC_ID_QDMC,
    CODEC_ID_CELT,
#if LIBAVCODEC_VERSION_MAJOR > 53
    CODEC_ID_G723_1,
    CODEC_ID_G729,
    CODEC_ID_8SVX_EXP,
    CODEC_ID_8SVX_FIB,
#endif
    CODEC_ID_BMV_AUDIO,

    
    CODEC_ID_FIRST_SUBTITLE = 0x17000,          
    CODEC_ID_DVD_SUBTITLE = 0x17000,
    CODEC_ID_DVB_SUBTITLE,
    CODEC_ID_TEXT,  
    CODEC_ID_XSUB,
    CODEC_ID_SSA,
    CODEC_ID_MOV_TEXT,
    CODEC_ID_HDMV_PGS_SUBTITLE,
    CODEC_ID_DVB_TELETEXT,
    CODEC_ID_SRT,

    
    CODEC_ID_FIRST_UNKNOWN = 0x18000,           
    CODEC_ID_TTF = 0x18000,

    CODEC_ID_PROBE = 0x19000, 

    CODEC_ID_MPEG2TS = 0x20000, 

    CODEC_ID_MPEG4SYSTEMS = 0x20001, 

    CODEC_ID_FFMETADATA = 0x21000,   
};

#if FF_API_OLD_SAMPLE_FMT
#define SampleFormat AVSampleFormat

#define SAMPLE_FMT_NONE AV_SAMPLE_FMT_NONE
#define SAMPLE_FMT_U8   AV_SAMPLE_FMT_U8
#define SAMPLE_FMT_S16  AV_SAMPLE_FMT_S16
#define SAMPLE_FMT_S32  AV_SAMPLE_FMT_S32
#define SAMPLE_FMT_FLT  AV_SAMPLE_FMT_FLT
#define SAMPLE_FMT_DBL  AV_SAMPLE_FMT_DBL
#define SAMPLE_FMT_NB   AV_SAMPLE_FMT_NB
#endif

#if FF_API_OLD_AUDIOCONVERT
#include "libavutil/audioconvert.h"


#define CH_FRONT_LEFT            AV_CH_FRONT_LEFT
#define CH_FRONT_RIGHT           AV_CH_FRONT_RIGHT
#define CH_FRONT_CENTER          AV_CH_FRONT_CENTER
#define CH_LOW_FREQUENCY         AV_CH_LOW_FREQUENCY
#define CH_BACK_LEFT             AV_CH_BACK_LEFT
#define CH_BACK_RIGHT            AV_CH_BACK_RIGHT
#define CH_FRONT_LEFT_OF_CENTER  AV_CH_FRONT_LEFT_OF_CENTER
#define CH_FRONT_RIGHT_OF_CENTER AV_CH_FRONT_RIGHT_OF_CENTER
#define CH_BACK_CENTER           AV_CH_BACK_CENTER
#define CH_SIDE_LEFT             AV_CH_SIDE_LEFT
#define CH_SIDE_RIGHT            AV_CH_SIDE_RIGHT
#define CH_TOP_CENTER            AV_CH_TOP_CENTER
#define CH_TOP_FRONT_LEFT        AV_CH_TOP_FRONT_LEFT
#define CH_TOP_FRONT_CENTER      AV_CH_TOP_FRONT_CENTER
#define CH_TOP_FRONT_RIGHT       AV_CH_TOP_FRONT_RIGHT
#define CH_TOP_BACK_LEFT         AV_CH_TOP_BACK_LEFT
#define CH_TOP_BACK_CENTER       AV_CH_TOP_BACK_CENTER
#define CH_TOP_BACK_RIGHT        AV_CH_TOP_BACK_RIGHT
#define CH_STEREO_LEFT           AV_CH_STEREO_LEFT
#define CH_STEREO_RIGHT          AV_CH_STEREO_RIGHT




#define CH_LAYOUT_NATIVE         AV_CH_LAYOUT_NATIVE


#define CH_LAYOUT_MONO           AV_CH_LAYOUT_MONO
#define CH_LAYOUT_STEREO         AV_CH_LAYOUT_STEREO
#define CH_LAYOUT_2_1            AV_CH_LAYOUT_2_1
#define CH_LAYOUT_SURROUND       AV_CH_LAYOUT_SURROUND
#define CH_LAYOUT_4POINT0        AV_CH_LAYOUT_4POINT0
#define CH_LAYOUT_2_2            AV_CH_LAYOUT_2_2
#define CH_LAYOUT_QUAD           AV_CH_LAYOUT_QUAD
#define CH_LAYOUT_5POINT0        AV_CH_LAYOUT_5POINT0
#define CH_LAYOUT_5POINT1        AV_CH_LAYOUT_5POINT1
#define CH_LAYOUT_5POINT0_BACK   AV_CH_LAYOUT_5POINT0_BACK
#define CH_LAYOUT_5POINT1_BACK   AV_CH_LAYOUT_5POINT1_BACK
#define CH_LAYOUT_7POINT0        AV_CH_LAYOUT_7POINT0
#define CH_LAYOUT_7POINT1        AV_CH_LAYOUT_7POINT1
#define CH_LAYOUT_7POINT1_WIDE   AV_CH_LAYOUT_7POINT1_WIDE
#define CH_LAYOUT_STEREO_DOWNMIX AV_CH_LAYOUT_STEREO_DOWNMIX
#endif

#if FF_API_OLD_DECODE_AUDIO

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#endif








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
    ME_ITER,        
    ME_TESA,        
};

enum AVDiscard{
    

    AVDISCARD_NONE   =-16, 
    AVDISCARD_DEFAULT=  0, 
    AVDISCARD_NONREF =  8, 
    AVDISCARD_BIDIR  = 16, 
    AVDISCARD_NONKEY = 32, 
    AVDISCARD_ALL    = 48, 
};

enum AVColorPrimaries{
    AVCOL_PRI_BT709      =1, 
    AVCOL_PRI_UNSPECIFIED=2,
    AVCOL_PRI_BT470M     =4,
    AVCOL_PRI_BT470BG    =5, 
    AVCOL_PRI_SMPTE170M  =6, 
    AVCOL_PRI_SMPTE240M  =7, 
    AVCOL_PRI_FILM       =8,
    AVCOL_PRI_NB           , 
};

enum AVColorTransferCharacteristic{
    AVCOL_TRC_BT709      =1, 
    AVCOL_TRC_UNSPECIFIED=2,
    AVCOL_TRC_GAMMA22    =4, 
    AVCOL_TRC_GAMMA28    =5, 
    AVCOL_TRC_NB           , 
};

enum AVColorSpace{
    AVCOL_SPC_RGB        =0,
    AVCOL_SPC_BT709      =1, 
    AVCOL_SPC_UNSPECIFIED=2,
    AVCOL_SPC_FCC        =4,
    AVCOL_SPC_BT470BG    =5, 
    AVCOL_SPC_SMPTE170M  =6, 
    AVCOL_SPC_SMPTE240M  =7,
    AVCOL_SPC_NB           , 
};

enum AVColorRange{
    AVCOL_RANGE_UNSPECIFIED=0,
    AVCOL_RANGE_MPEG       =1, 
    AVCOL_RANGE_JPEG       =2, 
    AVCOL_RANGE_NB           , 
};






enum AVChromaLocation{
    AVCHROMA_LOC_UNSPECIFIED=0,
    AVCHROMA_LOC_LEFT       =1, 
    AVCHROMA_LOC_CENTER     =2, 
    AVCHROMA_LOC_TOPLEFT    =3, 
    AVCHROMA_LOC_TOP        =4,
    AVCHROMA_LOC_BOTTOMLEFT =5,
    AVCHROMA_LOC_BOTTOM     =6,
    AVCHROMA_LOC_NB           , 
};

#if FF_API_FLAC_GLOBAL_OPTS



enum AVLPCType {
    AV_LPC_TYPE_DEFAULT     = -1, 
    AV_LPC_TYPE_NONE        =  0, 
    AV_LPC_TYPE_FIXED       =  1, 
    AV_LPC_TYPE_LEVINSON    =  2, 
    AV_LPC_TYPE_CHOLESKY    =  3, 
    AV_LPC_TYPE_NB              , 
};
#endif

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

#define FF_MAX_B_FRAMES 16






#define CODEC_FLAG_QSCALE 0x0002  ///< Use fixed qscale.
#define CODEC_FLAG_4MV    0x0004  ///< 4 MV per MB allowed / advanced prediction for H.263.
#define CODEC_FLAG_QPEL   0x0010  ///< Use qpel MC.
#define CODEC_FLAG_GMC    0x0020  ///< Use GMC.
#define CODEC_FLAG_MV0    0x0040  ///< Always try a MB with MV=<0,0>.





#define CODEC_FLAG_INPUT_PRESERVED 0x0100
#define CODEC_FLAG_PASS1           0x0200   ///< Use internal 2pass ratecontrol in first pass mode.
#define CODEC_FLAG_PASS2           0x0400   ///< Use internal 2pass ratecontrol in second pass mode.
#define CODEC_FLAG_GRAY            0x2000   ///< Only decode/encode grayscale.
#define CODEC_FLAG_EMU_EDGE        0x4000   ///< Don't draw edges.
#define CODEC_FLAG_PSNR            0x8000   ///< error[?] variables will be set during encoding.
#define CODEC_FLAG_TRUNCATED       0x00010000 /** Input bitstream might be truncated at a random
                                                  location instead of only at frame boundaries. */
#define CODEC_FLAG_NORMALIZE_AQP  0x00020000 ///< Normalize adaptive quantization.
#define CODEC_FLAG_INTERLACED_DCT 0x00040000 ///< Use interlaced DCT.
#define CODEC_FLAG_LOW_DELAY      0x00080000 ///< Force low delay.
#define CODEC_FLAG_GLOBAL_HEADER  0x00400000 ///< Place global headers in extradata instead of every keyframe.
#define CODEC_FLAG_BITEXACT       0x00800000 ///< Use only bitexact stuff (except (I)DCT).

#define CODEC_FLAG_AC_PRED        0x01000000 ///< H.263 advanced intra coding / MPEG-4 AC prediction
#define CODEC_FLAG_CBP_RD         0x04000000 ///< Use rate distortion optimization for cbp.
#define CODEC_FLAG_QP_RD          0x08000000 ///< Use rate distortion optimization for qp selectioon.
#define CODEC_FLAG_LOOP_FILTER    0x00000800 ///< loop filter
#define CODEC_FLAG_INTERLACED_ME  0x20000000 ///< interlaced motion estimation
#define CODEC_FLAG_CLOSED_GOP     0x80000000
#define CODEC_FLAG2_FAST          0x00000001 ///< Allow non spec compliant speedup tricks.
#define CODEC_FLAG2_STRICT_GOP    0x00000002 ///< Strictly enforce GOP size.
#define CODEC_FLAG2_NO_OUTPUT     0x00000004 ///< Skip bitstream encoding.
#define CODEC_FLAG2_LOCAL_HEADER  0x00000008 ///< Place global headers at every keyframe instead of in extradata.
#define CODEC_FLAG2_SKIP_RD       0x00004000 ///< RD optimal MB level residual skipping
#define CODEC_FLAG2_CHUNKS        0x00008000 ///< Input bitstream might be truncated at a packet boundaries instead of only at frame boundaries.





#if FF_API_MPEGVIDEO_GLOBAL_OPTS
#define CODEC_FLAG_OBMC           0x00000001 ///< OBMC
#define CODEC_FLAG_H263P_AIV      0x00000008 ///< H.263 alternative inter VLC
#define CODEC_FLAG_PART   0x0080  ///< Use data partitioning.
#define CODEC_FLAG_ALT_SCAN       0x00100000 ///< Use alternate scan.
#define CODEC_FLAG_H263P_UMV      0x02000000 ///< unlimited motion vector
#define CODEC_FLAG_H263P_SLICE_STRUCT 0x10000000
#define CODEC_FLAG_SVCD_SCAN_OFFSET 0x40000000 ///< Will reserve space for SVCD scan offset user data.
#define CODEC_FLAG2_INTRA_VLC     0x00000800 ///< Use MPEG-2 intra VLC table.
#define CODEC_FLAG2_DROP_FRAME_TIMECODE 0x00002000 ///< timecode is in drop frame format.
#define CODEC_FLAG2_NON_LINEAR_QUANT 0x00010000 ///< Use MPEG-2 nonlinear quantizer.
#endif
#if FF_API_MJPEG_GLOBAL_OPTS
#define CODEC_FLAG_EXTERN_HUFF     0x1000   ///< Use external Huffman table (for MJPEG).
#endif
#if FF_API_X264_GLOBAL_OPTS
#define CODEC_FLAG2_BPYRAMID      0x00000010 ///< H.264 allow B-frames to be used as references.
#define CODEC_FLAG2_WPRED         0x00000020 ///< H.264 weighted biprediction for B-frames
#define CODEC_FLAG2_MIXED_REFS    0x00000040 ///< H.264 one reference per partition, as opposed to one reference per macroblock
#define CODEC_FLAG2_8X8DCT        0x00000080 ///< H.264 high profile 8x8 transform
#define CODEC_FLAG2_FASTPSKIP     0x00000100 ///< H.264 fast pskip
#define CODEC_FLAG2_AUD           0x00000200 ///< H.264 access unit delimiters
#define CODEC_FLAG2_BRDO          0x00000400 ///< B-frame rate-distortion optimization
#define CODEC_FLAG2_MBTREE        0x00040000 ///< Use macroblock tree ratecontrol (x264 only)
#define CODEC_FLAG2_PSY           0x00080000 ///< Use psycho visual optimizations.
#define CODEC_FLAG2_SSIM          0x00100000 ///< Compute SSIM during encoding, error[] values are undefined.
#define CODEC_FLAG2_INTRA_REFRESH 0x00200000 ///< Use periodic insertion of intra blocks instead of keyframes.
#endif
#if FF_API_SNOW_GLOBAL_OPTS
#define CODEC_FLAG2_MEMC_ONLY     0x00001000 ///< Only do ME/MC (I frames -> ref, P frame -> ME+MC).
#endif
#if FF_API_LAME_GLOBAL_OPTS
#define CODEC_FLAG2_BIT_RESERVOIR 0x00020000 ///< Use a bit reservoir when encoding if possible
#endif











#define CODEC_CAP_DRAW_HORIZ_BAND 0x0001 ///< Decoder can use draw_horiz_band callback.





#define CODEC_CAP_DR1             0x0002
#if FF_API_PARSE_FRAME

#define CODEC_CAP_PARSE_ONLY      0x0004
#endif
#define CODEC_CAP_TRUNCATED       0x0008

#define CODEC_CAP_HWACCEL         0x0010























#define CODEC_CAP_DELAY           0x0020




#define CODEC_CAP_SMALL_LAST_FRAME 0x0040



#define CODEC_CAP_HWACCEL_VDPAU    0x0080











#define CODEC_CAP_SUBFRAMES        0x0100




#define CODEC_CAP_EXPERIMENTAL     0x0200



#define CODEC_CAP_CHANNEL_CONF     0x0400



#define CODEC_CAP_NEG_LINESIZES    0x0800



#define CODEC_CAP_FRAME_THREADS    0x1000



#define CODEC_CAP_SLICE_THREADS    0x2000



#define CODEC_CAP_PARAM_CHANGE     0x4000



#define CODEC_CAP_AUTO_THREADS     0x8000



#define CODEC_CAP_VARIABLE_FRAME_SIZE 0x10000


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







typedef struct AVPanScan{
    




    int id;

    




    int width;
    int height;

    




    int16_t position[3][2];
}AVPanScan;

#define FF_QSCALE_TYPE_MPEG1 0
#define FF_QSCALE_TYPE_MPEG2 1
#define FF_QSCALE_TYPE_H264  2
#define FF_QSCALE_TYPE_VP56  3

#define FF_BUFFER_TYPE_INTERNAL 1
#define FF_BUFFER_TYPE_USER     2 ///< direct rendering buffers (image is (de)allocated by user)
#define FF_BUFFER_TYPE_SHARED   4 ///< Buffer from somewhere else; don't deallocate image (data/base), all other tables are not shared.
#define FF_BUFFER_TYPE_COPY     8 ///< Just a (modified) copy of some other buffer, don't deallocate anything.

#if FF_API_OLD_FF_PICT_TYPES

#define FF_I_TYPE  AV_PICTURE_TYPE_I  ///< Intra
#define FF_P_TYPE  AV_PICTURE_TYPE_P  ///< Predicted
#define FF_B_TYPE  AV_PICTURE_TYPE_B  ///< Bi-dir predicted
#define FF_S_TYPE  AV_PICTURE_TYPE_S  ///< S(GMC)-VOP MPEG4
#define FF_SI_TYPE AV_PICTURE_TYPE_SI ///< Switching Intra
#define FF_SP_TYPE AV_PICTURE_TYPE_SP ///< Switching Predicted
#define FF_BI_TYPE AV_PICTURE_TYPE_BI
#endif

#define FF_BUFFER_HINTS_VALID    0x01 // Buffer hints value is meaningful (if 0 ignore).
#define FF_BUFFER_HINTS_READABLE 0x02 // Codec will read from buffer.
#define FF_BUFFER_HINTS_PRESERVE 0x04 // User must not alter buffer content.
#define FF_BUFFER_HINTS_REUSABLE 0x08 // Codec will reuse the buffer (update).

enum AVPacketSideDataType {
    AV_PKT_DATA_PALETTE,
    AV_PKT_DATA_NEW_EXTRADATA,
    AV_PKT_DATA_PARAM_CHANGE,
};

typedef struct AVPacket {
    








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
    void  (*destruct)(struct AVPacket *);
    void  *priv;
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








typedef struct AVFrame {
#if FF_API_DATA_POINTERS
#define AV_NUM_DATA_POINTERS 4
#else
#define AV_NUM_DATA_POINTERS 8
#endif
    





    uint8_t *data[AV_NUM_DATA_POINTERS];

    








    int linesize[AV_NUM_DATA_POINTERS];

    





    uint8_t *base[AV_NUM_DATA_POINTERS];
    




    int key_frame;

    




    enum AVPictureType pict_type;

    





    int64_t pts;

    




    int coded_picture_number;
    




    int display_picture_number;

    




    int quality;

#if FF_API_AVFRAME_AGE
    


    attribute_deprecated int age;
#endif

    







    int reference;

    




    int8_t *qscale_table;
    




    int qstride;

    





    uint8_t *mbskip_table;

    











    int16_t (*motion_val[2])[2];

    





    uint32_t *mb_type;

    





    uint8_t motion_subsample_log2;

    




    void *opaque;

    




    uint64_t error[AV_NUM_DATA_POINTERS];

    





    int type;

    





    int repeat_pict;

    


    int qscale_type;

    




    int interlaced_frame;

    




    int top_field_first;

    




    AVPanScan *pan_scan;

    




    int palette_has_changed;

    




    int buffer_hints;

    




    short *dct_coeff;

    





    int8_t *ref_index[2];

    










    int64_t reordered_opaque;

    




    void *hwaccel_picture_private;

    




    int64_t pkt_pts;

    




    int64_t pkt_dts;

    




    struct AVCodecContext *owner;

    




    void *thread_opaque;

    




    int nb_samples;

    

















    uint8_t **extended_data;

    




    AVRational sample_aspect_ratio;

    




    int width, height;

    






    int format;
} AVFrame;

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
    




    int bit_rate;

    





    int bit_rate_tolerance;

    




    int flags;

    







    int sub_id;

    






    int me_method;

    










    uint8_t *extradata;
    int extradata_size;

    







    AVRational time_base;

    
    






    int width, height;

#define FF_ASPECT_EXTENDED 15

    




    int gop_size;

    






    enum PixelFormat pix_fmt;

    






















    void (*draw_horiz_band)(struct AVCodecContext *s,
                            const AVFrame *src, int offset[AV_NUM_DATA_POINTERS],
                            int y, int type, int height);

    
    int sample_rate; 
    int channels;    

    




    enum AVSampleFormat sample_fmt;  

    
    


    int frame_size;
    int frame_number;   

    





    int delay;

    
    float qcompress;  
    float qblur;      

    




    int qmin;

    




    int qmax;

    




    int max_qdiff;

    





    int max_b_frames;

    






    float b_quant_factor;

    
    int rc_strategy;
#define FF_RC_STRATEGY_XVID 1

    int b_frame_strategy;

    struct AVCodec *codec;

    void *priv_data;

    int rtp_payload_size;   
                            
                            
                            
                            
                            


    
    
    
    
    
    
    void (*rtp_callback)(struct AVCodecContext *avctx, void *data, int size, int mb_nb);

    
    int mv_bits;
    int header_bits;
    int i_tex_bits;
    int p_tex_bits;
    int i_count;
    int p_count;
    int skip_count;
    int misc_bits;

    




    int frame_bits;

    




    void *opaque;

    char codec_name[32];
    enum AVMediaType codec_type; 
    enum CodecID codec_id; 

    












    unsigned int codec_tag;

    




    int workaround_bugs;
#define FF_BUG_AUTODETECT       1  ///< autodetection
#define FF_BUG_OLD_MSMPEG4      2
#define FF_BUG_XVID_ILACE       4
#define FF_BUG_UMP4             8
#define FF_BUG_NO_PADDING       16
#define FF_BUG_AMV              32
#define FF_BUG_AC_VLC           0  ///< Will be removed, libavcodec can now handle these non-compliant files by default.
#define FF_BUG_QPEL_CHROMA      64
#define FF_BUG_STD_QPEL         128
#define FF_BUG_QPEL_CHROMA2     256
#define FF_BUG_DIRECT_BLOCKSIZE 512
#define FF_BUG_EDGE             1024
#define FF_BUG_HPEL_CHROMA      2048
#define FF_BUG_DC_CLIP          4096
#define FF_BUG_MS               8192 ///< Work around various bugs in Microsoft's broken decoders.
#define FF_BUG_TRUNCATED       16384


    




    int luma_elim_threshold;

    




    int chroma_elim_threshold;

    











    int strict_std_compliance;
#define FF_COMPLIANCE_VERY_STRICT   2 ///< Strictly conform to an older more strict version of the spec or reference software.
#define FF_COMPLIANCE_STRICT        1 ///< Strictly conform to all the things in the spec no matter what consequences.
#define FF_COMPLIANCE_NORMAL        0
#define FF_COMPLIANCE_UNOFFICIAL   -1 ///< Allow unofficial extensions
#define FF_COMPLIANCE_EXPERIMENTAL -2 ///< Allow nonstandardized experimental things.

    




    float b_quant_offset;

#if FF_API_ER
    





    attribute_deprecated int error_recognition;
#define FF_ER_CAREFUL         1
#define FF_ER_COMPLIANT       2
#define FF_ER_AGGRESSIVE      3
#define FF_ER_VERY_AGGRESSIVE 4
#define FF_ER_EXPLODE         5
#endif 

    






















































    int (*get_buffer)(struct AVCodecContext *c, AVFrame *pic);

    








    void (*release_buffer)(struct AVCodecContext *c, AVFrame *pic);

    





    int has_b_frames;

    



    int block_align;

#if FF_API_PARSE_FRAME
    





    attribute_deprecated int parse_only;
#endif

    




    int mpeg_quant;

    




    char *stats_out;

    





    char *stats_in;

    





    float rc_qsquish;

    float rc_qmod_amp;
    int rc_qmod_freq;

    




    RcOverride *rc_override;
    int rc_override_count;

    




    const char *rc_eq;

    




    int rc_max_rate;

    




    int rc_min_rate;

    




    int rc_buffer_size;
    float rc_buffer_aggressivity;

    






    float i_quant_factor;

    




    float i_quant_offset;

    




    float rc_initial_cplx;

    




    int dct_algo;
#define FF_DCT_AUTO    0
#define FF_DCT_FASTINT 1
#define FF_DCT_INT     2
#define FF_DCT_MMX     3
#define FF_DCT_MLIB    4
#define FF_DCT_ALTIVEC 5
#define FF_DCT_FAAN    6

    




    float lumi_masking;

    




    float temporal_cplx_masking;

    




    float spatial_cplx_masking;

    




    float p_masking;

    




    float dark_masking;

    




    int idct_algo;
#define FF_IDCT_AUTO          0
#define FF_IDCT_INT           1
#define FF_IDCT_SIMPLE        2
#define FF_IDCT_SIMPLEMMX     3
#define FF_IDCT_LIBMPEG2MMX   4
#define FF_IDCT_PS2           5
#define FF_IDCT_MLIB          6
#define FF_IDCT_ARM           7
#define FF_IDCT_ALTIVEC       8
#define FF_IDCT_SH4           9
#define FF_IDCT_SIMPLEARM     10
#define FF_IDCT_H264          11
#define FF_IDCT_VP3           12
#define FF_IDCT_IPP           13
#define FF_IDCT_XVIDMMX       14
#define FF_IDCT_CAVS          15
#define FF_IDCT_SIMPLEARMV5TE 16
#define FF_IDCT_SIMPLEARMV6   17
#define FF_IDCT_SIMPLEVIS     18
#define FF_IDCT_WMV2          19
#define FF_IDCT_FAAN          20
#define FF_IDCT_EA            21
#define FF_IDCT_SIMPLENEON    22
#define FF_IDCT_SIMPLEALPHA   23
#define FF_IDCT_BINK          24

    




    int slice_count;
    




    int *slice_offset;

    




    int error_concealment;
#define FF_EC_GUESS_MVS   1
#define FF_EC_DEBLOCK     2

    







    unsigned dsp_mask;

    




     int bits_per_coded_sample;

    




     int prediction_method;
#define FF_PRED_LEFT   0
#define FF_PRED_PLANE  1
#define FF_PRED_MEDIAN 2

    






    AVRational sample_aspect_ratio;

    




    AVFrame *coded_frame;

    




    int debug;
#define FF_DEBUG_PICT_INFO   1
#define FF_DEBUG_RC          2
#define FF_DEBUG_BITSTREAM   4
#define FF_DEBUG_MB_TYPE     8
#define FF_DEBUG_QP          16
#define FF_DEBUG_MV          32
#define FF_DEBUG_DCT_COEFF   0x00000040
#define FF_DEBUG_SKIP        0x00000080
#define FF_DEBUG_STARTCODE   0x00000100
#define FF_DEBUG_PTS         0x00000200
#define FF_DEBUG_ER          0x00000400
#define FF_DEBUG_MMCO        0x00000800
#define FF_DEBUG_BUGS        0x00001000
#define FF_DEBUG_VIS_QP      0x00002000
#define FF_DEBUG_VIS_MB_TYPE 0x00004000
#define FF_DEBUG_BUFFERS     0x00008000
#define FF_DEBUG_THREADS     0x00010000

    




    int debug_mv;
#define FF_DEBUG_VIS_MV_P_FOR  0x00000001 //visualize forward predicted MVs of P frames
#define FF_DEBUG_VIS_MV_B_FOR  0x00000002 //visualize forward predicted MVs of B frames
#define FF_DEBUG_VIS_MV_B_BACK 0x00000004 //visualize backward predicted MVs of B frames

    




    uint64_t error[AV_NUM_DATA_POINTERS];

    




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
#define FF_CMP_W53    11
#define FF_CMP_W97    12
#define FF_CMP_DCTMAX 13
#define FF_CMP_DCT264 14
#define FF_CMP_CHROMA 256

    




    int dia_size;

    




    int last_predictor_count;

    




    int pre_me;

    




    int me_pre_cmp;

    




    int pre_dia_size;

    




    int me_subpel_quality;

    








    enum PixelFormat (*get_format)(struct AVCodecContext *s, const enum PixelFormat * fmt);

    







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

    





    int color_table_id;

#if FF_API_INTERNAL_CONTEXT
    




    attribute_deprecated int internal_buffer_count;

    




    attribute_deprecated void *internal_buffer;
#endif

    





    int global_quality;

#define FF_CODER_TYPE_VLC       0
#define FF_CODER_TYPE_AC        1
#define FF_CODER_TYPE_RAW       2
#define FF_CODER_TYPE_RLE       3
#define FF_CODER_TYPE_DEFLATE   4
    




    int coder_type;

    




    int context_model;
#if 0
    




    uint8_t * (*realloc)(struct AVCodecContext *s, uint8_t *buf, int buf_size);
#endif

    




    int slice_flags;
#define SLICE_FLAG_CODED_ORDER    0x0001 ///< draw_horiz_band() is called in coded order instead of display
#define SLICE_FLAG_ALLOW_FIELD    0x0002 ///< allow draw_horiz_band() with field slices (MPEG2 field pics)
#define SLICE_FLAG_ALLOW_PLANE    0x0004 ///< allow draw_horiz_band() with 1 component at a time (SVQ1)

    




    int xvmc_acceleration;

    




    int mb_decision;
#define FF_MB_DECISION_SIMPLE 0        ///< uses mb_cmp
#define FF_MB_DECISION_BITS   1        ///< chooses the one which needs the fewest bits
#define FF_MB_DECISION_RD     2        ///< rate distortion

    




    uint16_t *intra_matrix;

    




    uint16_t *inter_matrix;

    





    unsigned int stream_codec_tag;

    





    int scenechange_threshold;

    




    int lmin;

    




    int lmax;

#if FF_API_PALETTE_CONTROL
    




    struct AVPaletteControl *palctrl;
#endif

    




    int noise_reduction;

    











    int (*reget_buffer)(struct AVCodecContext *c, AVFrame *pic);

    




    int rc_initial_buffer_occupancy;

    




    int inter_threshold;

    




    int flags2;

    




    int error_rate;

#if FF_API_ANTIALIAS_ALGO
    




    attribute_deprecated int antialias_algo;
#define FF_AA_AUTO    0
#define FF_AA_FASTINT 1 //not implemented yet
#define FF_AA_INT     2
#define FF_AA_FLOAT   3
#endif

    




    int quantizer_noise_shaping;

    





    int thread_count;

    








    int (*execute)(struct AVCodecContext *c, int (*func)(struct AVCodecContext *c2, void *arg), void *arg2, int *ret, int count, int size);

    





    void *thread_opaque;

    






     int me_threshold;

    




     int mb_threshold;

    




     int intra_dc_precision;

    




     int nsse_weight;

    




     int skip_top;

    




     int skip_bottom;

    




     int profile;
#define FF_PROFILE_UNKNOWN -99
#define FF_PROFILE_RESERVED -100

#define FF_PROFILE_AAC_MAIN 0
#define FF_PROFILE_AAC_LOW  1
#define FF_PROFILE_AAC_SSR  2
#define FF_PROFILE_AAC_LTP  3

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

    




     int level;
#define FF_LEVEL_UNKNOWN -99

    




     int lowres;

    




    int coded_width, coded_height;

    




    int frame_skip_threshold;

    




    int frame_skip_factor;

    




    int frame_skip_exp;

    




    int frame_skip_cmp;

    





    float border_masking;

    




    int mb_lmin;

    




    int mb_lmax;

    




    int me_penalty_compensation;

    




    enum AVDiscard skip_loop_filter;

    




    enum AVDiscard skip_idct;

    




    enum AVDiscard skip_frame;

    




    int bidir_refine;

    




    int brd_scale;

#if FF_API_X264_GLOBAL_OPTS
    





    attribute_deprecated float crf;

    





    attribute_deprecated int cqp;
#endif

    




    int keyint_min;

    




    int refs;

    




    int chromaoffset;

#if FF_API_X264_GLOBAL_OPTS
    




    attribute_deprecated int bframebias;
#endif

    




    int trellis;

#if FF_API_X264_GLOBAL_OPTS
    




    attribute_deprecated float complexityblur;

    





    attribute_deprecated int deblockalpha;

    





    attribute_deprecated int deblockbeta;

    




    attribute_deprecated int partitions;
#define X264_PART_I4X4 0x001  /* Analyze i4x4 */
#define X264_PART_I8X8 0x002  /* Analyze i8x8 (requires 8x8 transform) */
#define X264_PART_P8X8 0x010  /* Analyze p16x8, p8x16 and p8x8 */
#define X264_PART_P4X4 0x020  /* Analyze p8x4, p4x8, p4x4 */
#define X264_PART_B8X8 0x100  /* Analyze b16x8, b8x16 and b8x8 */

    




    attribute_deprecated int directpred;
#endif

    




    int cutoff;

    




    int scenechange_factor;

    





    int mv0_threshold;

    




    int b_sensitivity;

    



    int compression_level;
#define FF_COMPRESSION_DEFAULT -1

    



    int min_prediction_order;

    



    int max_prediction_order;

#if FF_API_FLAC_GLOBAL_OPTS
    





    




    attribute_deprecated int lpc_coeff_precision;

    




    attribute_deprecated int prediction_order_method;

    



    attribute_deprecated int min_partition_order;

    



    attribute_deprecated int max_partition_order;
    


#endif

    




    int64_t timecode_frame_start;

#if FF_API_REQUEST_CHANNELS
    





    int request_channels;
#endif

#if FF_API_DRC_SCALE
    






    attribute_deprecated float drc_scale;
#endif

    






    int64_t reordered_opaque;

    




    int bits_per_raw_sample;

    




    uint64_t channel_layout;

    




    uint64_t request_channel_layout;

    




    float rc_max_available_vbv_use;

    




    float rc_min_vbv_overflow_use;

    




    struct AVHWAccel *hwaccel;

    






    int ticks_per_frame;

    









    void *hwaccel_context;

    




    enum AVColorPrimaries color_primaries;

    




    enum AVColorTransferCharacteristic color_trc;

    




    enum AVColorSpace colorspace;

    




    enum AVColorRange color_range;

    




    enum AVChromaLocation chroma_sample_location;

    

















    int (*execute2)(struct AVCodecContext *c, int (*func)(struct AVCodecContext *c2, void *arg, int jobnr, int threadnr), void *arg2, int *ret, int count);

#if FF_API_X264_GLOBAL_OPTS
    







    attribute_deprecated int weighted_p_pred;

    







    attribute_deprecated int aq_mode;

    





    attribute_deprecated float aq_strength;

    





    attribute_deprecated float psy_rd;

    





    attribute_deprecated float psy_trellis;

    





    attribute_deprecated int rc_lookahead;

    






    attribute_deprecated float crf_max;
#endif

    int log_level_offset;

#if FF_API_FLAC_GLOBAL_OPTS
    




    attribute_deprecated enum AVLPCType lpc_type;

    




    attribute_deprecated int lpc_passes;
#endif

    






    int slices;

    







    uint8_t *subtitle_header;
    int subtitle_header_size;

    






    AVPacket *pkt;

#if FF_API_INTERNAL_CONTEXT
    








    attribute_deprecated int is_copy;
#endif

    







    int thread_type;
#define FF_THREAD_FRAME   1 ///< Decode more than one frame at once
#define FF_THREAD_SLICE   2 ///< Decode more than one part of a single frame at once

    




    int active_thread_type;

    







    int thread_safe_callbacks;

    





    uint64_t vbv_delay;

    




    enum AVAudioServiceType audio_service_type;

    




    enum AVSampleFormat request_sample_fmt;

    




    int err_recognition;
#define AV_EF_CRCCHECK  (1<<0)
#define AV_EF_BITSTREAM (1<<1)
#define AV_EF_BUFFER    (1<<2)
#define AV_EF_EXPLODE   (1<<3)

    





    struct AVCodecInternal *internal;

    



    enum AVFieldOrder field_order;
} AVCodecContext;




typedef struct AVProfile {
    int profile;
    const char *name; 
} AVProfile;

typedef struct AVCodecDefault AVCodecDefault;




typedef struct AVCodec {
    





    const char *name;
    enum AVMediaType type;
    enum CodecID id;
    int priv_data_size;
    int (*init)(AVCodecContext *);
    int (*encode)(AVCodecContext *, uint8_t *buf, int buf_size, void *data);
    int (*close)(AVCodecContext *);
    int (*decode)(AVCodecContext *, void *outdata, int *outdata_size, AVPacket *avpkt);
    



    int capabilities;
    struct AVCodec *next;
    



    void (*flush)(AVCodecContext *);
    const AVRational *supported_framerates; 
    const enum PixelFormat *pix_fmts;       
    



    const char *long_name;
    const int *supported_samplerates;       
    const enum AVSampleFormat *sample_fmts; 
    const uint64_t *channel_layouts;         
    uint8_t max_lowres;                     
    const AVClass *priv_class;              
    const AVProfile *profiles;              

    



    




    int (*init_thread_copy)(AVCodecContext *);
    






    int (*update_thread_context)(AVCodecContext *dst, const AVCodecContext *src);
    

    


    const AVCodecDefault *defaults;

    


    void (*init_static_data)(struct AVCodec *codec);

    









    int (*encode2)(AVCodecContext *avctx, AVPacket *avpkt, const AVFrame *frame,
                   int *got_packet_ptr);
} AVCodec;




typedef struct AVHWAccel {
    




    const char *name;

    




    enum AVMediaType type;

    




    enum CodecID id;

    




    enum PixelFormat pix_fmt;

    



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
#if FF_API_PALETTE_CONTROL








typedef struct AVPaletteControl {

    

    int palette_changed;

    



    unsigned int palette[AVPALETTE_COUNT];

} AVPaletteControl attribute_deprecated;
#endif

enum AVSubtitleType {
    SUBTITLE_NONE,

    SUBTITLE_BITMAP,                

    



    SUBTITLE_TEXT,

    



    SUBTITLE_ASS,
};

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
} AVSubtitleRect;

typedef struct AVSubtitle {
    uint16_t format; 
    uint32_t start_display_time; 
    uint32_t end_display_time; 
    unsigned num_rects;
    AVSubtitleRect **rects;
    int64_t pts;    
} AVSubtitle;






attribute_deprecated void av_destruct_packet_nofree(AVPacket *pkt);




void av_destruct_packet(AVPacket *pkt);






void av_init_packet(AVPacket *pkt);









int av_new_packet(AVPacket *pkt, int size);







void av_shrink_packet(AVPacket *pkt, int size);







int av_grow_packet(AVPacket *pkt, int grow_by);





int av_dup_packet(AVPacket *pkt);






void av_free_packet(AVPacket *pkt);









uint8_t* av_packet_new_side_data(AVPacket *pkt, enum AVPacketSideDataType type,
                                 int size);









uint8_t* av_packet_get_side_data(AVPacket *pkt, enum AVPacketSideDataType type,
                                 int *size);



struct ReSampleContext;
struct AVResampleContext;

typedef struct ReSampleContext ReSampleContext;

















ReSampleContext *av_audio_resample_init(int output_channels, int input_channels,
                                        int output_rate, int input_rate,
                                        enum AVSampleFormat sample_fmt_out,
                                        enum AVSampleFormat sample_fmt_in,
                                        int filter_length, int log2_phase_count,
                                        int linear, double cutoff);

int audio_resample(ReSampleContext *s, short *output, short *input, int nb_samples);







void audio_resample_close(ReSampleContext *s);











struct AVResampleContext *av_resample_init(int out_rate, int in_rate, int filter_length, int log2_phase_count, int linear, double cutoff);










int av_resample(struct AVResampleContext *c, short *dst, short *src, int *consumed, int src_size, int dst_size, int update_ctx);














void av_resample_compensate(struct AVResampleContext *c, int sample_delta, int compensation_distance);
void av_resample_close(struct AVResampleContext *c);












int avpicture_alloc(AVPicture *picture, enum PixelFormat pix_fmt, int width, int height);








void avpicture_free(AVPicture *picture);





















int avpicture_fill(AVPicture *picture, uint8_t *ptr,
                   enum PixelFormat pix_fmt, int width, int height);
















int avpicture_layout(const AVPicture* src, enum PixelFormat pix_fmt, int width, int height,
                     unsigned char *dest, int dest_size);













int avpicture_get_size(enum PixelFormat pix_fmt, int width, int height);
void avcodec_get_chroma_sub_sample(enum PixelFormat pix_fmt, int *h_shift, int *v_shift);

#if FF_API_GET_PIX_FMT_NAME



attribute_deprecated
const char *avcodec_get_pix_fmt_name(enum PixelFormat pix_fmt);
#endif

void avcodec_set_dimensions(AVCodecContext *s, int width, int height);






unsigned int avcodec_pix_fmt_to_codec_tag(enum PixelFormat pix_fmt);








size_t av_get_codec_tag_string(char *buf, size_t buf_size, unsigned int codec_tag);

#define FF_LOSS_RESOLUTION  0x0001 /**< loss due to resolution change */
#define FF_LOSS_DEPTH       0x0002 /**< loss due to color depth change */
#define FF_LOSS_COLORSPACE  0x0004 /**< loss due to color space conversion */
#define FF_LOSS_ALPHA       0x0008 /**< loss of alpha bits */
#define FF_LOSS_COLORQUANT  0x0010 /**< loss due to color quantization */
#define FF_LOSS_CHROMA      0x0020 /**< loss of chroma (e.g. RGB to gray conversion) */


















int avcodec_get_pix_fmt_loss(enum PixelFormat dst_pix_fmt, enum PixelFormat src_pix_fmt,
                             int has_alpha);























enum PixelFormat avcodec_find_best_pix_fmt(int64_t pix_fmt_mask, enum PixelFormat src_pix_fmt,
                              int has_alpha, int *loss_ptr);

#if FF_API_GET_ALPHA_INFO
#define FF_ALPHA_TRANSP       0x0001 /* image has some totally transparent pixels */
#define FF_ALPHA_SEMI_TRANSP  0x0002 /* image has some transparent pixels */





attribute_deprecated
int img_get_alpha_info(const AVPicture *src,
                       enum PixelFormat pix_fmt, int width, int height);
#endif



int avpicture_deinterlace(AVPicture *dst, const AVPicture *src,
                          enum PixelFormat pix_fmt, int width, int height);








AVCodec *av_codec_next(AVCodec *c);




unsigned avcodec_version(void);




const char *avcodec_configuration(void);




const char *avcodec_license(void);

#if FF_API_AVCODEC_INIT




attribute_deprecated
void avcodec_init(void);
#endif









void avcodec_register(AVCodec *codec);







AVCodec *avcodec_find_encoder(enum CodecID id);







AVCodec *avcodec_find_encoder_by_name(const char *name);







AVCodec *avcodec_find_decoder(enum CodecID id);







AVCodec *avcodec_find_decoder_by_name(const char *name);
void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode);








const char *av_get_profile_name(const AVCodec *codec, int profile);

#if FF_API_ALLOC_CONTEXT






attribute_deprecated
void avcodec_get_context_defaults(AVCodecContext *s);



attribute_deprecated
void avcodec_get_context_defaults2(AVCodecContext *s, enum AVMediaType);
#endif










int avcodec_get_context_defaults3(AVCodecContext *s, AVCodec *codec);

#if FF_API_ALLOC_CONTEXT









attribute_deprecated
AVCodecContext *avcodec_alloc_context(void);



attribute_deprecated
AVCodecContext *avcodec_alloc_context2(enum AVMediaType);
#endif













AVCodecContext *avcodec_alloc_context3(AVCodec *codec);












int avcodec_copy_context(AVCodecContext *dest, const AVCodecContext *src);






void avcodec_get_frame_defaults(AVFrame *pic);








AVFrame *avcodec_alloc_frame(void);

int avcodec_default_get_buffer(AVCodecContext *s, AVFrame *pic);
void avcodec_default_release_buffer(AVCodecContext *s, AVFrame *pic);
int avcodec_default_reget_buffer(AVCodecContext *s, AVFrame *pic);








unsigned avcodec_get_edge_width(void);









void avcodec_align_dimensions(AVCodecContext *s, int *width, int *height);









void avcodec_align_dimensions2(AVCodecContext *s, int *width, int *height,
                               int linesize_align[AV_NUM_DATA_POINTERS]);

enum PixelFormat avcodec_default_get_format(struct AVCodecContext *s, const enum PixelFormat * fmt);

#if FF_API_THREAD_INIT



attribute_deprecated
int avcodec_thread_init(AVCodecContext *s, int thread_count);
#endif

int avcodec_default_execute(AVCodecContext *c, int (*func)(AVCodecContext *c2, void *arg2),void *arg, int *ret, int count, int size);
int avcodec_default_execute2(AVCodecContext *c, int (*func)(AVCodecContext *c2, void *arg2, int, int),void *arg, int *ret, int count);


#if FF_API_AVCODEC_OPEN





























attribute_deprecated
int avcodec_open(AVCodecContext *avctx, AVCodec *codec);
#endif





































int avcodec_open2(AVCodecContext *avctx, AVCodec *codec, AVDictionary **options);

#if FF_API_OLD_DECODE_AUDIO






















































attribute_deprecated int avcodec_decode_audio3(AVCodecContext *avctx, int16_t *samples,
                         int *frame_size_ptr,
                         AVPacket *avpkt);
#endif


































int avcodec_decode_audio4(AVCodecContext *avctx, AVFrame *frame,
                          int *got_frame_ptr, AVPacket *avpkt);










































int avcodec_decode_video2(AVCodecContext *avctx, AVFrame *picture,
                         int *got_picture_ptr,
                         AVPacket *avpkt);

















int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub,
                            int *got_sub_ptr,
                            AVPacket *avpkt);






void avsubtitle_free(AVSubtitle *sub);

#if FF_API_OLD_ENCODE_AUDIO
























int attribute_deprecated avcodec_encode_audio(AVCodecContext *avctx,
                                              uint8_t *buf, int buf_size,
                                              const short *samples);
#endif







































int avcodec_encode_audio2(AVCodecContext *avctx, AVPacket *avpkt,
                          const AVFrame *frame, int *got_packet_ptr);

















int avcodec_fill_audio_frame(AVFrame *frame, int nb_channels,
                             enum AVSampleFormat sample_fmt, const uint8_t *buf,
                             int buf_size, int align);













int avcodec_encode_video(AVCodecContext *avctx, uint8_t *buf, int buf_size,
                         const AVFrame *pict);
int avcodec_encode_subtitle(AVCodecContext *avctx, uint8_t *buf, int buf_size,
                            const AVSubtitle *sub);










int avcodec_close(AVCodecContext *avctx);











void avcodec_register_all(void);




void avcodec_flush_buffers(AVCodecContext *avctx);

void avcodec_default_free_buffers(AVCodecContext *s);



#if FF_API_OLD_FF_PICT_TYPES







attribute_deprecated
char av_get_pict_type_char(int pict_type);
#endif







int av_get_bits_per_sample(enum CodecID codec_id);

#if FF_API_OLD_SAMPLE_FMT



attribute_deprecated
int av_get_bits_per_sample_format(enum AVSampleFormat sample_fmt);
#endif


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








void *av_fast_realloc(void *ptr, unsigned int *size, size_t min_size);













void av_fast_malloc(void *ptr, unsigned int *size, size_t min_size);




void av_picture_copy(AVPicture *dst, const AVPicture *src,
                     enum PixelFormat pix_fmt, int width, int height);




int av_picture_crop(AVPicture *dst, const AVPicture *src,
                    enum PixelFormat pix_fmt, int top_band, int left_band);




int av_picture_pad(AVPicture *dst, const AVPicture *src, int height, int width, enum PixelFormat pix_fmt,
            int padtop, int padbottom, int padleft, int padright, int *color);








unsigned int av_xiphlacing(unsigned char *s, unsigned int v);













void av_log_missing_feature(void *avc, const char *feature, int want_sample);









void av_log_ask_for_sample(void *avc, const char *msg, ...) av_printf_format(2, 3);




void av_register_hwaccel(AVHWAccel *hwaccel);






AVHWAccel *av_hwaccel_next(AVHWAccel *hwaccel);





enum AVLockOp {
  AV_LOCK_CREATE,  
  AV_LOCK_OBTAIN,  
  AV_LOCK_RELEASE, 
  AV_LOCK_DESTROY, 
};














int av_lockmgr_register(int (*cb)(void **mutex, enum AVLockOp op));




enum AVMediaType avcodec_get_type(enum CodecID codec_id);







const AVClass *avcodec_get_class(void);





int avcodec_is_open(AVCodecContext *s);

#endif 
