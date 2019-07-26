



#include "cpr.h"




























int cpr_crypto_rand (uint8_t *buf, int *len)
{
#ifdef CPR_WIN_RAND_DEBUG
    


    int      i, total_bytes;

    if ((buf == NULL) || (len == NULL)) { 
        
        return (0);
    }

    total_bytes = *len;
    for (i = 0, i < total_bytes ; i++) {
        *buf = rand() & 0xff;
        buf++;
    }
    return (1);

#else
    


    if (len != NULL) {
        
        *len = 0;
    }
    return (0);

#endif
}
