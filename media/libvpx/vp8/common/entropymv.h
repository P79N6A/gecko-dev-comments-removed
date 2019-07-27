










#ifndef VP8_COMMON_ENTROPYMV_H_
#define VP8_COMMON_ENTROPYMV_H_

#include "treecoder.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    mv_max  = 1023,              
    MVvals = (2 * mv_max) + 1,   
    mvfp_max  = 255,              
    MVfpvals = (2 * mvfp_max) +1, 

    mvlong_width = 10,       
    mvnum_short = 8,         

    

    mvpis_short = 0,         
    MVPsign,                
    MVPshort,               

    MVPbits = MVPshort + mvnum_short - 1, 
    MVPcount = MVPbits + mvlong_width    
};

typedef struct mv_context
{
    vp8_prob prob[MVPcount];  
} MV_CONTEXT;

extern const MV_CONTEXT vp8_mv_update_probs[2], vp8_default_mv_context[2];

#ifdef __cplusplus
}  
#endif

#endif
