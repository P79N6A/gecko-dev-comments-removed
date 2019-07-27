



















#ifndef AVUTIL_MATHEMATICS_H
#define AVUTIL_MATHEMATICS_H

#include <stdint.h>
#include <math.h>
#include "attributes.h"
#include "rational.h"
#include "intfloat.h"

#ifndef M_LOG2_10
#define M_LOG2_10      3.32192809488736234787  /* log_2 10 */
#endif
#ifndef M_PHI
#define M_PHI          1.61803398874989484820   /* phi / golden ratio */
#endif
#ifndef NAN
#define NAN            av_int2float(0x7fc00000)
#endif
#ifndef INFINITY
#define INFINITY       av_int2float(0x7f800000)
#endif







enum AVRounding {
    AV_ROUND_ZERO     = 0, 
    AV_ROUND_INF      = 1, 
    AV_ROUND_DOWN     = 2, 
    AV_ROUND_UP       = 3, 
    AV_ROUND_NEAR_INF = 5, 
};






int64_t av_const av_gcd(int64_t a, int64_t b);





int64_t av_rescale(int64_t a, int64_t b, int64_t c) av_const;





int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding) av_const;




int64_t av_rescale_q(int64_t a, AVRational bq, AVRational cq) av_const;




int64_t av_rescale_q_rnd(int64_t a, AVRational bq, AVRational cq,
                         enum AVRounding) av_const;







int av_compare_ts(int64_t ts_a, AVRational tb_a, int64_t ts_b, AVRational tb_b);











int64_t av_compare_mod(uint64_t a, uint64_t b, uint64_t mod);





#endif 
