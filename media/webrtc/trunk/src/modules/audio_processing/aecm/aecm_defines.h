









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AECM_AECM_DEFINES_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AECM_AECM_DEFINES_H_

#define AECM_DYNAMIC_Q




#define FRAME_LEN       80             /* Total frame length, 10 ms. */

#ifdef AECM_SHORT
#define PART_LEN        32             /* Length of partition. */
#define PART_LEN_SHIFT  6              /* Length of (PART_LEN * 2) in base 2. */
#else
#define PART_LEN        64             /* Length of partition. */
#define PART_LEN_SHIFT  7              /* Length of (PART_LEN * 2) in base 2. */
#endif

#define PART_LEN1       (PART_LEN + 1)  /* Unique fft coefficients. */
#define PART_LEN2       (PART_LEN << 1) /* Length of partition * 2. */
#define PART_LEN4       (PART_LEN << 2) /* Length of partition * 4. */
#define FAR_BUF_LEN     PART_LEN4       /* Length of buffers. */
#define MAX_DELAY       100


#ifdef AECM_SHORT
#define CONV_LEN        1024         /* Convergence length used at startup. */
#else
#define CONV_LEN        512          /* Convergence length used at startup. */
#endif
#define CONV_LEN2       (CONV_LEN << 1) /* Used at startup. */


#define MAX_BUF_LEN     64           /* History length of energy signals. */
#define FAR_ENERGY_MIN  1025         /* Lowest Far energy level: At least 2 */
                                     
#define FAR_ENERGY_DIFF 929          /* Allowed difference between max */
                                     
#define ENERGY_DEV_OFFSET       0    /* The energy error offset in Q8. */
#define ENERGY_DEV_TOL  400          /* The energy estimation tolerance (Q8). */
#define FAR_ENERGY_VAD_REGION   230  /* Far VAD tolerance region. */


#define MU_MIN          10          /* Min stepsize 2^-MU_MIN (far end energy */
                                    
#define MU_MAX          1           /* Max stepsize 2^-MU_MAX (far end energy */
                                    
#define MU_DIFF         9           /* MU_MIN - MU_MAX */


#define MIN_MSE_COUNT   20 /* Min number of consecutive blocks with enough */
                           
#define MIN_MSE_DIFF    29 /* The ratio between adapted and stored channel to */
                           
#define MSE_RESOLUTION  5           /* MSE parameter resolution. */
#define RESOLUTION_CHANNEL16    12  /* W16 Channel in Q-RESOLUTION_CHANNEL16. */
#define RESOLUTION_CHANNEL32    28  /* W32 Channel in Q-RESOLUTION_CHANNEL. */
#define CHANNEL_VAD     16          /* Minimum energy in frequency band */
                                    


#define RESOLUTION_SUPGAIN      8     /* Channel in Q-(RESOLUTION_SUPGAIN). */
#define SUPGAIN_DEFAULT (1 << RESOLUTION_SUPGAIN)  /* Default. */
#define SUPGAIN_ERROR_PARAM_A   3072  /* Estimation error parameter */
                                      
#define SUPGAIN_ERROR_PARAM_B   1536  /* Estimation error parameter */
                                      
#define SUPGAIN_ERROR_PARAM_D   SUPGAIN_DEFAULT /* Estimation error parameter */
                                
#define SUPGAIN_EPC_DT  200     /* SUPGAIN_ERROR_PARAM_C * ENERGY_DEV_TOL */


#define CORR_WIDTH      31      /* Number of samples to correlate over. */
#define CORR_MAX        16      /* Maximum correlation offset. */
#define CORR_MAX_BUF    63
#define CORR_DEV        4
#define CORR_MAX_LEVEL  20
#define CORR_MAX_LOW    4
#define CORR_BUF_LEN    (CORR_MAX << 1) + 1


#define ONE_Q14         (1 << 14)


#define NLP_COMP_LOW    3277    /* 0.2 in Q14 */
#define NLP_COMP_HIGH   ONE_Q14 /* 1 in Q14 */

#endif
