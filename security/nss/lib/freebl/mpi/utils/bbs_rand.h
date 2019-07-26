









#ifndef _H_BBSRAND_
#define _H_BBSRAND_

#include <limits.h>
#include "mpi.h"

#define  BBS_RAND_MAX    UINT_MAX


extern int bbs_seed_size;

void         bbs_srand(unsigned char *data, int len);
unsigned int bbs_rand(void);

#endif 
