
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_SETTINGS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_SETTINGS_H_


#define FS                                      16000


#define INITIAL_FRAMESAMPLES     960


#define MAXFFTSIZE 2048
#define NFACTOR 11






#define FRAMESIZE                               30

#define FRAMESAMPLES                            480 /* ((FRAMESIZE*FS)/1000) */
#define FRAMESAMPLES_HALF      240
#define FRAMESAMPLES_QUARTER                    120





#define MAX_FRAMESAMPLES      960
#define MAX_SWBFRAMESAMPLES                     (MAX_FRAMESAMPLES * 2)

#define FRAMESAMPLES_10ms                       ((10*FS)/1000)
#define SWBFRAMESAMPLES_10ms                    (FRAMESAMPLES_10ms * 2)

#define FRAMESAMPLES_30ms            480

#define SUBFRAMES                               6

#define UPDATE                                  80

#define HALF_SUBFRAMELEN                        (UPDATE/2)

#define QLOOKAHEAD                              24    /* 3 ms */

#define AR_ORDER                                6

#define LP_ORDER                                0


#define WINLEN                                  256

#define ORDERLO                                 12

#define ORDERHI                                 6

#define UB_LPC_ORDER                            4
#define UB_LPC_VEC_PER_FRAME                    2
#define UB16_LPC_VEC_PER_FRAME                  4
#define UB_ACTIVE_SUBFRAMES                     2
#define UB_MAX_LPC_ORDER                        6
#define UB_INTERPOL_SEGMENTS                    1
#define UB16_INTERPOL_SEGMENTS                  3
#define LB_TOTAL_DELAY_SAMPLES                 48
enum ISACBandwidth {isac8kHz = 8, isac12kHz = 12, isac16kHz = 16};
enum ISACBand{isacLowerBand = 0, isacUpperBand = 1};
#define UB_LPC_GAIN_DIM                 SUBFRAMES
#define FB_STATE_SIZE_WORD32                    6



#define POSTQORDER                              3

#define QORDER                                  3

#define QORDER_ALL                              (POSTQORDER+QORDER-1)

#define ALLPASSSECTIONS                         2



#define STREAM_SIZE_MAX     600 /* The old maximum size still needed for the decoding */
#define STREAM_SIZE_MAX_30  200 /* 200 bytes = 53.4 kbit/s @ 30 ms.framelength */
#define STREAM_SIZE_MAX_60  400 /* 400 bytes = 53.4 kbit/s @ 60 ms.framelength */


#define BIT_COUNTER_SIZE                        30

#define MAX_AR_MODEL_ORDER                      12//50



#define PITCH_FRAME_LEN                         (FRAMESAMPLES_HALF) /* 30 ms  */
#define PITCH_MAX_LAG                           140     /* 57 Hz  */
#define PITCH_MIN_LAG                           20              /* 400 Hz */
#define PITCH_MAX_GAIN                          0.45
#define PITCH_MAX_GAIN_06                       0.27                /* PITCH_MAX_GAIN*0.6 */
#define PITCH_MAX_GAIN_Q12      1843
#define PITCH_LAG_SPAN2                         (PITCH_MAX_LAG/2-PITCH_MIN_LAG/2+5)
#define PITCH_CORR_LEN2                         60     /* 15 ms  */
#define PITCH_CORR_STEP2                        (PITCH_FRAME_LEN/4)
#define PITCH_BW                 11     /* half the band width of correlation surface */
#define PITCH_SUBFRAMES                         4
#define PITCH_GRAN_PER_SUBFRAME                 5
#define PITCH_SUBFRAME_LEN                      (PITCH_FRAME_LEN/PITCH_SUBFRAMES)
#define PITCH_UPDATE                            (PITCH_SUBFRAME_LEN/PITCH_GRAN_PER_SUBFRAME)

#define PITCH_MAX_NUM_PEAKS                  10
#define PITCH_PEAK_DECAY               0.85

#define PITCH_WLPCORDER                   6
#define PITCH_WLPCWINLEN               PITCH_FRAME_LEN
#define PITCH_WLPCASYM                   0.3                 /* asymmetry parameter */
#define PITCH_WLPCBUFLEN               PITCH_WLPCWINLEN

#define PITCH_BUFFSIZE                   (PITCH_MAX_LAG + 50)  /* Extra 50 for fraction and LP filters */
#define PITCH_INTBUFFSIZE               (PITCH_FRAME_LEN+PITCH_BUFFSIZE)

#define PITCH_UPSTEP                1.5

#define PITCH_DOWNSTEP                   0.67
#define PITCH_FRACS                             8
#define PITCH_FRACORDER                         9
#define PITCH_DAMPORDER                         5
#define PITCH_FILTDELAY                         1.5f

#define PITCH_GAIN_STEPSIZE                     0.125




#define HPORDER                                 2


#define LOG2EXP                                 1.44269504088896       /* log2(exp) */
#define PI                                      3.14159265358979


#define MAX_PAYLOAD_LIMIT_ITERATION             5


#define RCU_BOTTLENECK_BPS                      16000
#define RCU_TRANSCODING_SCALE                   0.40f
#define RCU_TRANSCODING_SCALE_INVERSE           2.5f

#define RCU_TRANSCODING_SCALE_UB                0.50f
#define RCU_TRANSCODING_SCALE_UB_INVERSE        2.0f




#define ISAC_MEMORY_ALLOCATION_FAILED    6010
#define ISAC_MODE_MISMATCH       6020
#define ISAC_DISALLOWED_BOTTLENECK     6030
#define ISAC_DISALLOWED_FRAME_LENGTH    6040
#define ISAC_UNSUPPORTED_SAMPLING_FREQUENCY         6050


#define ISAC_RANGE_ERROR_BW_ESTIMATOR    6240

#define ISAC_ENCODER_NOT_INITIATED     6410
#define ISAC_DISALLOWED_CODING_MODE     6420
#define ISAC_DISALLOWED_FRAME_MODE_ENCODER   6430
#define ISAC_DISALLOWED_BITSTREAM_LENGTH            6440
#define ISAC_PAYLOAD_LARGER_THAN_LIMIT              6450
#define ISAC_DISALLOWED_ENCODER_BANDWIDTH           6460

#define ISAC_DECODER_NOT_INITIATED     6610
#define ISAC_EMPTY_PACKET       6620
#define ISAC_DISALLOWED_FRAME_MODE_DECODER   6630
#define ISAC_RANGE_ERROR_DECODE_FRAME_LENGTH  6640
#define ISAC_RANGE_ERROR_DECODE_BANDWIDTH   6650
#define ISAC_RANGE_ERROR_DECODE_PITCH_GAIN   6660
#define ISAC_RANGE_ERROR_DECODE_PITCH_LAG   6670
#define ISAC_RANGE_ERROR_DECODE_LPC     6680
#define ISAC_RANGE_ERROR_DECODE_SPECTRUM   6690
#define ISAC_LENGTH_MISMATCH      6730
#define ISAC_RANGE_ERROR_DECODE_BANDWITH            6740
#define ISAC_DISALLOWED_BANDWIDTH_MODE_DECODER      6750

#define ISAC_INCOMPATIBLE_FORMATS     6810

#endif 
