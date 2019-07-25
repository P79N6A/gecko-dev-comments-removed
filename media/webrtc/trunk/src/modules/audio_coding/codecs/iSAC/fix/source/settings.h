
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_SETTINGS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_SETTINGS_H_



#define FS                                      16000

#define FS_1_HALF        (WebRtc_UWord32) 24000

#define FS3          (WebRtc_UWord32) 48000

#define FS8          (WebRtc_UWord32) 128000


#define INITIAL_FRAMESAMPLES     960


#define FRAMESIZE                               30

#define FRAMESAMPLES                            480     /* ((FRAMESIZE*FS)/1000) */
#define FRAMESAMPLES_HALF       240

#define MAX_FRAMESAMPLES      960

#define FRAMESAMPLES_10ms                       160      /* ((10*FS)/1000) */

#define SAMPLES_PER_MSEC      16

#define SUBFRAMES                               6

#define UPDATE                                  80

#define HALF_SUBFRAMELEN                        40    /* (UPDATE/2) */

#define QLOOKAHEAD                              24    /* 3 ms */


#define AR_ORDER                                6
#define MAX_ORDER                               13
#define LEVINSON_MAX_ORDER                  12


#define WINLEN                                  256

#define ORDERLO                                 12

#define ORDERHI                                 6

#define KLT_NUM_AVG_GAIN                        0
#define KLT_NUM_AVG_SHAPE                       0
#define KLT_NUM_MODELS                          3
#define LPC_SHAPE_ORDER                         18    /* (ORDERLO + ORDERHI) */

#define KLT_ORDER_GAIN                          12    /* (2 * SUBFRAMES) */
#define KLT_ORDER_SHAPE                         108   /*  (LPC_SHAPE_ORDER * SUBFRAMES) */




#define POSTQORDER                              3

#define QORDER                                  3

#define ALLPASSSECTIONS                         2

#define NUMBEROFCOMPOSITEAPSECTIONS             4


#define NUMBEROFCHANNELAPSECTIONS               2



#define DPMIN_Q10                            -10240   /* -10.00 in Q10 */
#define DPMAX_Q10                             10240   /* 10.00 in Q10 */
#define MINBITS_Q10                           10240   /* 10.0 in Q10 */



#define STREAM_MAXW16       300 /* The old maximum size still needed for the decoding */
#define STREAM_MAXW16_30MS  100 /* 100 Word16 = 200 bytes = 53.4 kbit/s @ 30 ms.framelength */
#define STREAM_MAXW16_60MS  200 /* 200 Word16 = 400 bytes = 53.4 kbit/s @ 60 ms.framelength */





#define MAX_AR_MODEL_ORDER                      12


#define MAX_PAYLOAD_LIMIT_ITERATION           1



#define MIN_ISAC_BW                           10000     /* Minimum bandwidth in bits per sec */
#define MAX_ISAC_BW                           32000     /* Maxmum bandwidth in bits per sec */
#define MIN_ISAC_MD                           5         /* Minimum Max Delay in ?? */
#define MAX_ISAC_MD                           25        /* Maxmum Max Delay in ?? */
#define DELAY_CORRECTION_MAX      717
#define DELAY_CORRECTION_MED      819
#define Thld_30_60         18000
#define Thld_60_30         27000


#define HEADER_SIZE                           35       /* bytes */
#define INIT_FRAME_LEN                        60
#define INIT_BN_EST                           20000
#define INIT_BN_EST_Q7                        2560000  /* 20 kbps in Q7 */
#define INIT_REC_BN_EST_Q5                    789312   /* INIT_BN_EST + INIT_HDR_RATE in Q5 */



#define INIT_HDR_RATE                    4666

#define BURST_LEN                             3

#define BURST_INTERVAL                        800

#define INIT_BURST_LEN                        5

#define INIT_RATE                             10240000 /* INIT_BN_EST in Q9 */



#define PITCH_FRAME_LEN                         240  /* (FRAMESAMPLES/2) 30 ms  */
#define PITCH_MAX_LAG                           140       /* 57 Hz  */
#define PITCH_MIN_LAG                           20                /* 400 Hz */
#define PITCH_MIN_LAG_Q8                        5120 /* 256 * PITCH_MIN_LAG */
#define OFFSET_Q8                               768  /* 256 * 3 */

#define PITCH_MAX_GAIN_Q12      1843                  /* 0.45 */
#define PITCH_LAG_SPAN2                         65   /* (PITCH_MAX_LAG/2-PITCH_MIN_LAG/2+5) */
#define PITCH_CORR_LEN2                         60     /* 15 ms  */
#define PITCH_CORR_STEP2                        60   /* (PITCH_FRAME_LEN/4) */
#define PITCH_SUBFRAMES                         4
#define PITCH_SUBFRAME_LEN                      60   /* (PITCH_FRAME_LEN/PITCH_SUBFRAMES) */


#define PITCH_BUFFSIZE                   190  /* (PITCH_MAX_LAG + 50) Extra 50 for fraction and LP filters */
#define PITCH_INTBUFFSIZE               430  /* (PITCH_FRAME_LEN+PITCH_BUFFSIZE) */
#define PITCH_FRACS                             8
#define PITCH_FRACORDER                         9
#define PITCH_DAMPORDER                         5



#define HPORDER                                 2



#define DECAY_RATE               10               /* Q15, 20% of decay every lost frame apllied linearly sample by sample*/
#define PLC_WAS_USED              1
#define PLC_NOT_USED              3
#define RECOVERY_OVERLAP         80
#define RESAMP_RES              256
#define RESAMP_RES_BIT            8





#define ISAC_MEMORY_ALLOCATION_FAILED    6010
#define ISAC_MODE_MISMATCH       6020
#define ISAC_DISALLOWED_BOTTLENECK     6030
#define ISAC_DISALLOWED_FRAME_LENGTH    6040

#define ISAC_RANGE_ERROR_BW_ESTIMATOR    6240

#define ISAC_ENCODER_NOT_INITIATED     6410
#define ISAC_DISALLOWED_CODING_MODE     6420
#define ISAC_DISALLOWED_FRAME_MODE_ENCODER   6430
#define ISAC_DISALLOWED_BITSTREAM_LENGTH            6440
#define ISAC_PAYLOAD_LARGER_THAN_LIMIT              6450

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

#define ISAC_INCOMPATIBLE_FORMATS     6810


#endif 
