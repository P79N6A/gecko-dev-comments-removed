





































#include "mpi.h"
#include "mplogic.h"
#include "ecl.h"
#include "ecp.h"
#include "ecl-priv.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>



int ec_twoTo(int e);


#define BITSIZE 160


#define M_TimeOperation(op, k) { \
	double dStart, dNow, dUserTime; \
	struct rusage ru; \
	int i; \
	getrusage(RUSAGE_SELF, &ru); \
	dStart = (double)ru.ru_utime.tv_sec+(double)ru.ru_utime.tv_usec*0.000001; \
	for (i = 0; i < k; i++) { \
		{ op; } \
	}; \
	getrusage(RUSAGE_SELF, &ru); \
	dNow = (double)ru.ru_utime.tv_sec+(double)ru.ru_utime.tv_usec*0.000001; \
	dUserTime = dNow-dStart; \
	if (dUserTime) printf("    %-45s\n      k: %6i, t: %6.2f sec\n", #op, k, dUserTime); \
}





mp_err
main(void)
{
	signed char naf[BITSIZE + 1];
	ECGroup *group = NULL;
	mp_int k;
	mp_int *scalar;
	int i, count;
	int res;
	int w = 5;
	char s[1000];

	
	group = ECGroup_fromName(ECCurve_SECG_PRIME_160R1);
	scalar = &group->genx;

	
	ec_compute_wNAF(naf, BITSIZE, scalar, w);

	
	mp_init(&k);				

	for (i = BITSIZE; i >= 0; i--) {
		mp_add(&k, &k, &k);
		
		if (naf[i] >= 0) {
			mp_add_d(&k, naf[i], &k);
		} else {
			mp_sub_d(&k, -naf[i], &k);
		}
	}

	if (mp_cmp(&k, scalar) != 0) {
		printf("Error:  incorrect NAF value.\n");
		MP_CHECKOK(mp_toradix(&k, s, 16));
		printf("NAF value   %s\n", s);
		MP_CHECKOK(mp_toradix(scalar, s, 16));
		printf("original value   %s\n", s);
		goto CLEANUP;
	}

	
	for (i = 0; i <= BITSIZE; i++) {
		if (naf[i] % 2 == 0 && naf[i] != 0) {
			printf("Error:  Even non-zero digit found.\n");
			goto CLEANUP;
		}
		if (naf[i] < -(ec_twoTo(w - 1)) || naf[i] >= ec_twoTo(w - 1)) {
			printf("Error:  Magnitude of naf digit too large.\n");
			goto CLEANUP;
		}
	}

	
	count = w - 1;
	for (i = 0; i <= BITSIZE; i++) {
		if (naf[i] != 0) {
			if (count < w - 1) {
				printf("Error:  Sparsity failed.\n");
				goto CLEANUP;
			}
			count = 0;
		} else
			count++;
	}

	
	M_TimeOperation(ec_compute_wNAF(naf, BITSIZE, scalar, w), 10000);

	printf("Test passed.\n");
  CLEANUP:
	ECGroup_free(group);
	return MP_OKAY;
}
