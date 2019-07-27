






#ifndef SIEVE_H
#define SIEVE_H

#ifndef U_LOTS_OF_TIMES
#define U_LOTS_OF_TIMES 1000000
#endif

#include "unicode/utypes.h"



U_INTERNAL double uprv_calcSieveTime(void);








U_INTERNAL double uprv_getMeanTime(double *times, uint32_t *timeCount, double *marginOfError);






U_INTERNAL double uprv_getSieveTime(double *marginOfError);

#endif
