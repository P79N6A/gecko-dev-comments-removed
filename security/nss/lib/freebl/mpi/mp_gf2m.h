



#ifndef _MP_GF2M_H_
#define _MP_GF2M_H_

#include "mpi.h"

mp_err mp_badd(const mp_int *a, const mp_int *b, mp_int *c);
mp_err mp_bmul(const mp_int *a, const mp_int *b, mp_int *c);






mp_err mp_bmod(const mp_int *a, const unsigned int p[], mp_int *r);
mp_err mp_bmulmod(const mp_int *a, const mp_int *b, const unsigned int p[], 
    mp_int *r);
mp_err mp_bsqrmod(const mp_int *a, const unsigned int p[], mp_int *r);
mp_err mp_bdivmod(const mp_int *y, const mp_int *x, const mp_int *pp, 
    const unsigned int p[], mp_int *r);

int mp_bpoly2arr(const mp_int *a, unsigned int p[], int max);
mp_err mp_barr2poly(const unsigned int p[], mp_int *a);

#endif 
