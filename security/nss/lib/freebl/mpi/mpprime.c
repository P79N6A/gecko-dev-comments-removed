










































#include "mpi-priv.h"
#include "mpprime.h"
#include "mplogic.h"
#include <stdlib.h>
#include <string.h>

#define SMALL_TABLE 0 /* determines size of hard-wired prime table */

#define RANDOM() rand()

#include "primes.c"  






mp_err    s_mpp_divp(mp_int *a, const mp_digit *vec, int size, int *which);









mp_err  mpp_divis(mp_int *a, mp_int *b)
{
  mp_err  res;
  mp_int  rem;

  if((res = mp_init(&rem)) != MP_OKAY)
    return res;

  if((res = mp_mod(a, b, &rem)) != MP_OKAY)
    goto CLEANUP;

  if(mp_cmp_z(&rem) == 0)
    res = MP_YES;
  else
    res = MP_NO;

CLEANUP:
  mp_clear(&rem);
  return res;

} 











mp_err  mpp_divis_d(mp_int *a, mp_digit d)
{
  mp_err     res;
  mp_digit   rem;

  ARGCHK(a != NULL, MP_BADARG);

  if(d == 0)
    return MP_NO;

  if((res = mp_mod_d(a, d, &rem)) != MP_OKAY)
    return res;

  if(rem == 0)
    return MP_YES;
  else
    return MP_NO;

} 
















mp_err  mpp_random(mp_int *a)

{
  mp_digit  next = 0;
  unsigned int       ix, jx;

  ARGCHK(a != NULL, MP_BADARG);

  for(ix = 0; ix < USED(a); ix++) {
    for(jx = 0; jx < sizeof(mp_digit); jx++) {
      next = (next << CHAR_BIT) | (RANDOM() & UCHAR_MAX);
    }
    DIGIT(a, ix) = next;
  }

  return MP_OKAY;

} 





mp_err  mpp_random_size(mp_int *a, mp_size prec)
{
  mp_err   res;

  ARGCHK(a != NULL && prec > 0, MP_BADARG);
  
  if((res = s_mp_pad(a, prec)) != MP_OKAY)
    return res;

  return mpp_random(a);

} 













mp_err  mpp_divis_vector(mp_int *a, const mp_digit *vec, int size, int *which)
{
  ARGCHK(a != NULL && vec != NULL && size > 0, MP_BADARG);
  
  return s_mpp_divp(a, vec, size, which);

} 












mp_err  mpp_divis_primes(mp_int *a, mp_digit *np)
{
  int     size, which;
  mp_err  res;

  ARGCHK(a != NULL && np != NULL, MP_BADARG);

  size = (int)*np;
  if(size > prime_tab_size)
    size = prime_tab_size;

  res = mpp_divis_vector(a, prime_tab, size, &which);
  if(res == MP_YES) 
    *np = prime_tab[which];

  return res;

} 












mp_err  mpp_fermat(mp_int *a, mp_digit w)
{
  mp_int  base, test;
  mp_err  res;
  
  if((res = mp_init(&base)) != MP_OKAY)
    return res;

  mp_set(&base, w);

  if((res = mp_init(&test)) != MP_OKAY)
    goto TEST;

  
  if((res = mp_exptmod(&base, a, a, &test)) != MP_OKAY)
    goto CLEANUP;

  
  if(mp_cmp(&base, &test) == 0)
    res = MP_YES;
  else
    res = MP_NO;

 CLEANUP:
  mp_clear(&test);
 TEST:
  mp_clear(&base);

  return res;

} 











mp_err mpp_fermat_list(mp_int *a, const mp_digit *primes, mp_size nPrimes)
{
  mp_err rv = MP_YES;

  while (nPrimes-- > 0 && rv == MP_YES) {
    rv = mpp_fermat(a, *primes++);
  }
  return rv;
}












mp_err  mpp_pprime(mp_int *a, int nt)
{
  mp_err   res;
  mp_int   x, amo, m, z;	
  int      iter;
  unsigned int jx;
  mp_size  b;

  ARGCHK(a != NULL, MP_BADARG);

  MP_DIGITS(&x) = 0;
  MP_DIGITS(&amo) = 0;
  MP_DIGITS(&m) = 0;
  MP_DIGITS(&z) = 0;

  
  MP_CHECKOK( mp_init(&amo));
  
  MP_CHECKOK( mp_sub_d(a, 1, &amo) );

  b = mp_trailing_zeros(&amo);
  if (!b) { 
    res = MP_NO;
    goto CLEANUP;
  }

  MP_CHECKOK( mp_init_size(&x, MP_USED(a)) );
  MP_CHECKOK( mp_init(&z) );
  MP_CHECKOK( mp_init(&m) );
  MP_CHECKOK( mp_div_2d(&amo, b, &m, 0) );

  
  for(iter = 0; iter < nt; iter++) {

    
    s_mp_pad(&x, USED(a));
    mpp_random(&x);
    MP_CHECKOK( mp_mod(&x, a, &x) );
    if(mp_cmp_d(&x, 1) <= 0) {
      iter--;    
      continue;  
    }

    
    MP_CHECKOK( mp_exptmod(&x, &m, a, &z) );
    
    if(mp_cmp_d(&z, 1) == 0 || mp_cmp(&z, &amo) == 0) {
      res = MP_YES;
      continue;
    }
    
    res = MP_NO;  
    for (jx = 1; jx < b; jx++) {
      
      MP_CHECKOK( mp_sqrmod(&z, a, &z) );
      res = MP_NO;	

      if(mp_cmp_d(&z, 1) == 0) {
	break;
      }
      if(mp_cmp(&z, &amo) == 0) {
	res = MP_YES;
	break;
      } 
    } 

    



    if(res == MP_NO)
      break;

  } 
  
CLEANUP:
  mp_clear(&m);
  mp_clear(&z);
  mp_clear(&x);
  mp_clear(&amo);
  return res;

} 













mp_err mpp_sieve(mp_int *trial, const mp_digit *primes, mp_size nPrimes, 
		 unsigned char *sieve, mp_size nSieve)
{
  mp_err       res;
  mp_digit     rem;
  mp_size      ix;
  unsigned long offset;

  memset(sieve, 0, nSieve);

  for(ix = 0; ix < nPrimes; ix++) {
    mp_digit prime = primes[ix];
    mp_size  i;
    if((res = mp_mod_d(trial, prime, &rem)) != MP_OKAY) 
      return res;

    if (rem == 0) {
      offset = 0;
    } else {
      offset = prime - (rem / 2);
    }
    for (i = offset; i < nSieve ; i += prime) {
      sieve[i] = 1;
    }
  }

  return MP_OKAY;
}

#define SIEVE_SIZE 32*1024

mp_err mpp_make_prime(mp_int *start, mp_size nBits, mp_size strong,
		      unsigned long * nTries)
{
  mp_digit      np;
  mp_err        res;
  int           i	= 0;
  mp_int        trial;
  mp_int        q;
  mp_size       num_tests;
  unsigned char *sieve;
  
  ARGCHK(start != 0, MP_BADARG);
  ARGCHK(nBits > 16, MP_RANGE);

  sieve = malloc(SIEVE_SIZE);
  ARGCHK(sieve != NULL, MP_MEM);

  MP_DIGITS(&trial) = 0;
  MP_DIGITS(&q) = 0;
  MP_CHECKOK( mp_init(&trial) );
  MP_CHECKOK( mp_init(&q)     );
  
  if (nBits >= 1300) {
    num_tests = 2;
  } else if (nBits >= 850) {
    num_tests = 3;
  } else if (nBits >= 650) {
    num_tests = 4;
  } else if (nBits >= 550) {
    num_tests = 5;
  } else if (nBits >= 450) {
    num_tests = 6;
  } else if (nBits >= 400) {
    num_tests = 7;
  } else if (nBits >= 350) {
    num_tests = 8;
  } else if (nBits >= 300) {
    num_tests = 9;
  } else if (nBits >= 250) {
    num_tests = 12;
  } else if (nBits >= 200) {
    num_tests = 15;
  } else if (nBits >= 150) {
    num_tests = 18;
  } else if (nBits >= 100) {
    num_tests = 27;
  } else
    num_tests = 50;

  if (strong) 
    --nBits;
  MP_CHECKOK( mpl_set_bit(start, nBits - 1, 1) );
  MP_CHECKOK( mpl_set_bit(start,         0, 1) );
  for (i = mpl_significant_bits(start) - 1; i >= nBits; --i) {
    MP_CHECKOK( mpl_set_bit(start, i, 0) );
  }
  
  MP_CHECKOK(mpp_sieve(start, prime_tab + 1, prime_tab_size - 1, 
		       sieve, SIEVE_SIZE) );

#ifdef DEBUG_SIEVE
  res = 0;
  for (i = 0; i < SIEVE_SIZE; ++i) {
    if (!sieve[i])
      ++res;
  }
  fprintf(stderr,"sieve found %d potential primes.\n", res);
#define FPUTC(x,y) fputc(x,y)
#else
#define FPUTC(x,y) 
#endif

  res = MP_NO;
  for(i = 0; i < SIEVE_SIZE; ++i) {
    if (sieve[i])	
      continue;
    MP_CHECKOK( mp_add_d(start, 2 * i, &trial) );
    FPUTC('.', stderr);
    
    res = mpp_fermat(&trial, 2);
    if (res != MP_OKAY) {
      if (res == MP_NO)
	continue;	
      goto CLEANUP;
    }
      
    FPUTC('+', stderr);
    
    res = mpp_pprime(&trial, num_tests);
    if (res != MP_OKAY) {
      if (res == MP_NO)
	continue;	
      goto CLEANUP;
    }
    FPUTC('!', stderr);

    if (!strong) 
      break;	

    



    MP_CHECKOK( mp_mul_2(&trial, &q) );
    MP_CHECKOK( mp_add_d(&q, 1, &q)  );

    
    np = prime_tab_size;
    res = mpp_divis_primes(&q, &np);
    if (res == MP_YES) { 
      mp_clear(&q);
      continue;
    }
    if (res != MP_NO) 
      goto CLEANUP;

    
    res = mpp_fermat(&q, 2);
    if (res != MP_YES) {
      mp_clear(&q);
      if (res == MP_NO)
	continue;	
      goto CLEANUP;
    }

    
    res = mpp_pprime(&q, num_tests);
    if (res != MP_YES) {
      mp_clear(&q);
      if (res == MP_NO)
	continue;	
      goto CLEANUP;
    }

    
    mp_exch(&q, &trial);
    mp_clear(&q);
    break;

  } 
  if (res == MP_YES) 
    mp_exch(&trial, start);
CLEANUP:
  mp_clear(&trial);
  mp_clear(&q);
  if (nTries)
    *nTries += i;
  if (sieve != NULL) {
  	memset(sieve, 0, SIEVE_SIZE);
  	free (sieve);
  }
  return res;
}














mp_err    s_mpp_divp(mp_int *a, const mp_digit *vec, int size, int *which)
{
  mp_err    res;
  mp_digit  rem;

  int     ix;

  for(ix = 0; ix < size; ix++) {
    if((res = mp_mod_d(a, vec[ix], &rem)) != MP_OKAY) 
      return res;

    if(rem == 0) {
      if(which)
	*which = ix;
      return MP_YES;
    }
  }

  return MP_NO;

} 





