















#ifndef BOCSU_H
#define BOCSU_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

U_NAMESPACE_BEGIN

class ByteSink;

U_NAMESPACE_END
























































#define SLOPE_MIN           3
#define SLOPE_MAX           0xff
#define SLOPE_MIDDLE        0x81

#define SLOPE_TAIL_COUNT    (SLOPE_MAX-SLOPE_MIN+1)

#define SLOPE_MAX_BYTES     4
























#define SLOPE_SINGLE        80
#define SLOPE_LEAD_2        42
#define SLOPE_LEAD_3        3
#define SLOPE_LEAD_4        1


#define SLOPE_REACH_POS_1   SLOPE_SINGLE
#define SLOPE_REACH_NEG_1   (-SLOPE_SINGLE)


#define SLOPE_REACH_POS_2   (SLOPE_LEAD_2*SLOPE_TAIL_COUNT+(SLOPE_LEAD_2-1))
#define SLOPE_REACH_NEG_2   (-SLOPE_REACH_POS_2-1)


#define SLOPE_REACH_POS_3   (SLOPE_LEAD_3*SLOPE_TAIL_COUNT*SLOPE_TAIL_COUNT+(SLOPE_LEAD_3-1)*SLOPE_TAIL_COUNT+(SLOPE_TAIL_COUNT-1))
#define SLOPE_REACH_NEG_3   (-SLOPE_REACH_POS_3-1)


#define SLOPE_START_POS_2   (SLOPE_MIDDLE+SLOPE_SINGLE+1)
#define SLOPE_START_POS_3   (SLOPE_START_POS_2+SLOPE_LEAD_2)

#define SLOPE_START_NEG_2   (SLOPE_MIDDLE+SLOPE_REACH_NEG_1)
#define SLOPE_START_NEG_3   (SLOPE_START_NEG_2-SLOPE_LEAD_2)






#define NEGDIVMOD(n, d, m) { \
    (m)=(n)%(d); \
    (n)/=(d); \
    if((m)<0) { \
        --(n); \
        (m)+=(d); \
    } \
}

U_CFUNC void
u_writeIdenticalLevelRun(const UChar *s, int32_t length, icu::ByteSink &sink);

U_CFUNC int32_t
u_writeIdenticalLevelRunTwoChars(UChar32 first, UChar32 second, uint8_t *p);

U_CFUNC uint8_t *
u_writeDiff(int32_t diff, uint8_t *p);

#endif 

#endif
