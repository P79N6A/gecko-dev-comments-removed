












#define MP_USING_CACHE_SAFE_MOD_EXP 1 
#include <string.h>
#include "mpi-priv.h"
#include "mplogic.h"
#include "mpprime.h"
#ifdef MP_USING_MONT_MULF
#include "montmulf.h"
#endif
#include <stddef.h> 



#ifdef MP_CHAR_STORE_SLOW
#if !defined(MP_IS_BIG_ENDIAN) && !defined(MP_IS_LITTLE_ENDIAN)
#error "You must define MP_IS_BIG_ENDIAN or MP_IS_LITTLE_ENDIAN\n" \
       "  if you define MP_CHAR_STORE_SLOW."
#endif
#endif

#define STATIC

#define MAX_ODD_INTS    32   /* 2 ** (WINDOW_BITS - 1) */




mp_err s_mp_redc(mp_int *T, mp_mont_modulus *mmm)
{
  mp_err res;
  mp_size i;

  i = (MP_USED(&mmm->N) << 1) + 1;
  MP_CHECKOK( s_mp_pad(T, i) );
  for (i = 0; i < MP_USED(&mmm->N); ++i ) {
    mp_digit m_i = MP_DIGIT(T, i) * mmm->n0prime;
    
    MP_CHECKOK( s_mp_mul_d_add_offset(&mmm->N, m_i, T, i) );
  }
  s_mp_clamp(T);

  
  s_mp_rshd( T, MP_USED(&mmm->N) );

  if ((res = s_mp_cmp(T, &mmm->N)) >= 0) {
    
    MP_CHECKOK( s_mp_sub(T, &mmm->N) );
#ifdef DEBUG
    if ((res = mp_cmp(T, &mmm->N)) >= 0) {
      res = MP_UNDEF;
      goto CLEANUP;
    }
#endif
  }
  res = MP_OKAY;
CLEANUP:
  return res;
}

#if !defined(MP_MONT_USE_MP_MUL)






mp_err s_mp_mul_mont(const mp_int *a, const mp_int *b, mp_int *c, 
	           mp_mont_modulus *mmm)
{
  mp_digit *pb;
  mp_digit m_i;
  mp_err   res;
  mp_size  ib; 
  mp_size  useda, usedb;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (MP_USED(a) < MP_USED(b)) {
    const mp_int *xch = b;	
    b = a;
    a = xch;
  }

  MP_USED(c) = 1; MP_DIGIT(c, 0) = 0;
  ib = (MP_USED(&mmm->N) << 1) + 1;
  if((res = s_mp_pad(c, ib)) != MP_OKAY)
    goto CLEANUP;

  useda = MP_USED(a);
  pb = MP_DIGITS(b);
  s_mpv_mul_d(MP_DIGITS(a), useda, *pb++, MP_DIGITS(c));
  s_mp_setz(MP_DIGITS(c) + useda + 1, ib - (useda + 1));
  m_i = MP_DIGIT(c, 0) * mmm->n0prime;
  s_mp_mul_d_add_offset(&mmm->N, m_i, c, 0);

  
  usedb = MP_USED(b);
  for (ib = 1; ib < usedb; ib++) {
    mp_digit b_i    = *pb++;

    
    if (b_i)
      s_mpv_mul_d_add_prop(MP_DIGITS(a), useda, b_i, MP_DIGITS(c) + ib);
    m_i = MP_DIGIT(c, ib) * mmm->n0prime;
    s_mp_mul_d_add_offset(&mmm->N, m_i, c, ib);
  }
  if (usedb < MP_USED(&mmm->N)) {
    for (usedb = MP_USED(&mmm->N); ib < usedb; ++ib ) {
      m_i = MP_DIGIT(c, ib) * mmm->n0prime;
      s_mp_mul_d_add_offset(&mmm->N, m_i, c, ib);
    }
  }
  s_mp_clamp(c);
  s_mp_rshd( c, MP_USED(&mmm->N) ); 
  if (s_mp_cmp(c, &mmm->N) >= 0) {
    MP_CHECKOK( s_mp_sub(c, &mmm->N) );
  }
  res = MP_OKAY;

CLEANUP:
  return res;
}
#endif

STATIC
mp_err s_mp_to_mont(const mp_int *x, mp_mont_modulus *mmm, mp_int *xMont)
{
  mp_err res;

  
  MP_CHECKOK( mp_copy( x, xMont ) );
  MP_CHECKOK( s_mp_lshd( xMont, MP_USED(&mmm->N) ) );	
  MP_CHECKOK( mp_div(xMont, &mmm->N, 0, xMont) );	
CLEANUP:
  return res;
}

#ifdef MP_USING_MONT_MULF




#ifndef MP_FORCE_CACHE_SAFE
#undef MP_USING_CACHE_SAFE_MOD_EXP
#endif

unsigned int mp_using_mont_mulf = 1;


#define SQR \
  conv_i32_to_d32_and_d16(dm1, d16Tmp, mResult, nLen); \
  mont_mulf_noconv(mResult, dm1, d16Tmp, \
		   dTmp, dn, MP_DIGITS(modulus), nLen, dn0)


#define MUL(x) \
  conv_i32_to_d32(dm1, mResult, nLen); \
  mont_mulf_noconv(mResult, dm1, oddPowers[x], \
		   dTmp, dn, MP_DIGITS(modulus), nLen, dn0)


mp_err mp_exptmod_f(const mp_int *   montBase, 
                    const mp_int *   exponent, 
		    const mp_int *   modulus, 
		    mp_int *         result, 
		    mp_mont_modulus *mmm, 
		    int              nLen, 
		    mp_size          bits_in_exponent, 
		    mp_size          window_bits,
		    mp_size          odd_ints)
{
  mp_digit *mResult;
  double   *dBuf = 0, *dm1, *dn, *dSqr, *d16Tmp, *dTmp;
  double    dn0;
  mp_size   i;
  mp_err    res;
  int       expOff;
  int       dSize = 0, oddPowSize, dTmpSize;
  mp_int    accum1;
  double   *oddPowers[MAX_ODD_INTS];

  

  MP_DIGITS(&accum1) = 0;

  for (i = 0; i < MAX_ODD_INTS; ++i)
    oddPowers[i] = 0;

  MP_CHECKOK( mp_init_size(&accum1, 3 * nLen + 2) );

  mp_set(&accum1, 1);
  MP_CHECKOK( s_mp_to_mont(&accum1, mmm, &accum1) );
  MP_CHECKOK( s_mp_pad(&accum1, nLen) );

  oddPowSize = 2 * nLen + 1;
  dTmpSize   = 2 * oddPowSize;
  dSize = sizeof(double) * (nLen * 4 + 1 + 
			    ((odd_ints + 1) * oddPowSize) + dTmpSize);
  dBuf   = (double *)malloc(dSize);
  dm1    = dBuf;		
  dn     = dBuf   + nLen;	
  dSqr   = dn     + nLen;    	
  d16Tmp = dSqr   + nLen;	
  dTmp   = d16Tmp + oddPowSize;

  for (i = 0; i < odd_ints; ++i) {
      oddPowers[i] = dTmp;
      dTmp += oddPowSize;
  }
  mResult = (mp_digit *)(dTmp + dTmpSize);	

  
  conv_i32_to_d32(dn, MP_DIGITS(modulus), nLen);
  dn0 = (double)(mmm->n0prime & 0xffff);

  
  conv_i32_to_d32_and_d16(dm1, oddPowers[0], MP_DIGITS(montBase), nLen);
  mont_mulf_noconv(mResult, dm1, oddPowers[0], 
		   dTmp, dn, MP_DIGITS(modulus), nLen, dn0);
  conv_i32_to_d32(dSqr, mResult, nLen);

  for (i = 1; i < odd_ints; ++i) {
    mont_mulf_noconv(mResult, dSqr, oddPowers[i - 1], 
		     dTmp, dn, MP_DIGITS(modulus), nLen, dn0);
    conv_i32_to_d16(oddPowers[i], mResult, nLen);
  }

  s_mp_copy(MP_DIGITS(&accum1), mResult, nLen); 

  for (expOff = bits_in_exponent - window_bits; expOff >= 0; expOff -= window_bits) {
    mp_size smallExp;
    MP_CHECKOK( mpl_get_bits(exponent, expOff, window_bits) );
    smallExp = (mp_size)res;

    if (window_bits == 1) {
      if (!smallExp) {
	SQR;
      } else if (smallExp & 1) {
	SQR; MUL(0); 
      } else {
	abort();
      }
    } else if (window_bits == 4) {
      if (!smallExp) {
	SQR; SQR; SQR; SQR;
      } else if (smallExp & 1) {
	SQR; SQR; SQR; SQR; MUL(smallExp/2); 
      } else if (smallExp & 2) {
	SQR; SQR; SQR; MUL(smallExp/4); SQR; 
      } else if (smallExp & 4) {
	SQR; SQR; MUL(smallExp/8); SQR; SQR; 
      } else if (smallExp & 8) {
	SQR; MUL(smallExp/16); SQR; SQR; SQR; 
      } else {
	abort();
      }
    } else if (window_bits == 5) {
      if (!smallExp) {
	SQR; SQR; SQR; SQR; SQR; 
      } else if (smallExp & 1) {
	SQR; SQR; SQR; SQR; SQR; MUL(smallExp/2);
      } else if (smallExp & 2) {
	SQR; SQR; SQR; SQR; MUL(smallExp/4); SQR;
      } else if (smallExp & 4) {
	SQR; SQR; SQR; MUL(smallExp/8); SQR; SQR;
      } else if (smallExp & 8) {
	SQR; SQR; MUL(smallExp/16); SQR; SQR; SQR;
      } else if (smallExp & 0x10) {
	SQR; MUL(smallExp/32); SQR; SQR; SQR; SQR;
      } else {
	abort();
      }
    } else if (window_bits == 6) {
      if (!smallExp) {
	SQR; SQR; SQR; SQR; SQR; SQR;
      } else if (smallExp & 1) {
	SQR; SQR; SQR; SQR; SQR; SQR; MUL(smallExp/2); 
      } else if (smallExp & 2) {
	SQR; SQR; SQR; SQR; SQR; MUL(smallExp/4); SQR; 
      } else if (smallExp & 4) {
	SQR; SQR; SQR; SQR; MUL(smallExp/8); SQR; SQR; 
      } else if (smallExp & 8) {
	SQR; SQR; SQR; MUL(smallExp/16); SQR; SQR; SQR; 
      } else if (smallExp & 0x10) {
	SQR; SQR; MUL(smallExp/32); SQR; SQR; SQR; SQR; 
      } else if (smallExp & 0x20) {
	SQR; MUL(smallExp/64); SQR; SQR; SQR; SQR; SQR; 
      } else {
	abort();
      }
    } else {
      abort();
    }
  }

  s_mp_copy(mResult, MP_DIGITS(&accum1), nLen); 

  res = s_mp_redc(&accum1, mmm);
  mp_exch(&accum1, result);

CLEANUP:
  mp_clear(&accum1);
  if (dBuf) {
    if (dSize)
      memset(dBuf, 0, dSize);
    free(dBuf);
  }

  return res;
}
#undef SQR
#undef MUL
#endif

#define SQR(a,b) \
  MP_CHECKOK( mp_sqr(a, b) );\
  MP_CHECKOK( s_mp_redc(b, mmm) )

#if defined(MP_MONT_USE_MP_MUL)
#define MUL(x,a,b) \
  MP_CHECKOK( mp_mul(a, oddPowers + (x), b) ); \
  MP_CHECKOK( s_mp_redc(b, mmm) ) 
#else
#define MUL(x,a,b) \
  MP_CHECKOK( s_mp_mul_mont(a, oddPowers + (x), b, mmm) )
#endif

#define SWAPPA ptmp = pa1; pa1 = pa2; pa2 = ptmp


mp_err mp_exptmod_i(const mp_int *   montBase, 
                    const mp_int *   exponent, 
		    const mp_int *   modulus, 
		    mp_int *         result, 
		    mp_mont_modulus *mmm, 
		    int              nLen, 
		    mp_size          bits_in_exponent, 
		    mp_size          window_bits,
		    mp_size          odd_ints)
{
  mp_int *pa1, *pa2, *ptmp;
  mp_size i;
  mp_err  res;
  int     expOff;
  mp_int  accum1, accum2, power2, oddPowers[MAX_ODD_INTS];

  
  

  MP_DIGITS(&accum1) = 0;
  MP_DIGITS(&accum2) = 0;
  MP_DIGITS(&power2) = 0;
  for (i = 0; i < MAX_ODD_INTS; ++i) {
    MP_DIGITS(oddPowers + i) = 0;
  }

  MP_CHECKOK( mp_init_size(&accum1, 3 * nLen + 2) );
  MP_CHECKOK( mp_init_size(&accum2, 3 * nLen + 2) );

  MP_CHECKOK( mp_init_copy(&oddPowers[0], montBase) );

  mp_init_size(&power2, nLen + 2 * MP_USED(montBase) + 2);
  MP_CHECKOK( mp_sqr(montBase, &power2) );	
  MP_CHECKOK( s_mp_redc(&power2, mmm) );

  for (i = 1; i < odd_ints; ++i) {
    mp_init_size(oddPowers + i, nLen + 2 * MP_USED(&power2) + 2);
    MP_CHECKOK( mp_mul(oddPowers + (i - 1), &power2, oddPowers + i) );
    MP_CHECKOK( s_mp_redc(oddPowers + i, mmm) );
  }

  
  mp_set(&accum1, 1);
  MP_CHECKOK( s_mp_to_mont(&accum1, mmm, &accum1) );
  pa1 = &accum1;
  pa2 = &accum2;

  for (expOff = bits_in_exponent - window_bits; expOff >= 0; expOff -= window_bits) {
    mp_size smallExp;
    MP_CHECKOK( mpl_get_bits(exponent, expOff, window_bits) );
    smallExp = (mp_size)res;

    if (window_bits == 1) {
      if (!smallExp) {
	SQR(pa1,pa2); SWAPPA;
      } else if (smallExp & 1) {
	SQR(pa1,pa2); MUL(0,pa2,pa1);
      } else {
	abort();
      }
    } else if (window_bits == 4) {
      if (!smallExp) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1);
      } else if (smallExp & 1) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	MUL(smallExp/2, pa1,pa2); SWAPPA;
      } else if (smallExp & 2) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); 
	MUL(smallExp/4,pa2,pa1); SQR(pa1,pa2); SWAPPA;
      } else if (smallExp & 4) {
	SQR(pa1,pa2); SQR(pa2,pa1); MUL(smallExp/8,pa1,pa2); 
	SQR(pa2,pa1); SQR(pa1,pa2); SWAPPA;
      } else if (smallExp & 8) {
	SQR(pa1,pa2); MUL(smallExp/16,pa2,pa1); SQR(pa1,pa2); 
	SQR(pa2,pa1); SQR(pa1,pa2); SWAPPA;
      } else {
	abort();
      }
    } else if (window_bits == 5) {
      if (!smallExp) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	SQR(pa1,pa2); SWAPPA;
      } else if (smallExp & 1) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	SQR(pa1,pa2); MUL(smallExp/2,pa2,pa1);
      } else if (smallExp & 2) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	MUL(smallExp/4,pa1,pa2); SQR(pa2,pa1);
      } else if (smallExp & 4) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); 
	MUL(smallExp/8,pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1);
      } else if (smallExp & 8) {
	SQR(pa1,pa2); SQR(pa2,pa1); MUL(smallExp/16,pa1,pa2); 
	SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1);
      } else if (smallExp & 0x10) {
	SQR(pa1,pa2); MUL(smallExp/32,pa2,pa1); SQR(pa1,pa2); 
	SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1);
      } else {
	abort();
      }
    } else if (window_bits == 6) {
      if (!smallExp) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	SQR(pa1,pa2); SQR(pa2,pa1);
      } else if (smallExp & 1) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	SQR(pa1,pa2); SQR(pa2,pa1); MUL(smallExp/2,pa1,pa2); SWAPPA;
      } else if (smallExp & 2) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	SQR(pa1,pa2); MUL(smallExp/4,pa2,pa1); SQR(pa1,pa2); SWAPPA;
      } else if (smallExp & 4) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	MUL(smallExp/8,pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SWAPPA;
      } else if (smallExp & 8) {
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); 
	MUL(smallExp/16,pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	SQR(pa1,pa2); SWAPPA;
      } else if (smallExp & 0x10) {
	SQR(pa1,pa2); SQR(pa2,pa1); MUL(smallExp/32,pa1,pa2); 
	SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SWAPPA;
      } else if (smallExp & 0x20) {
	SQR(pa1,pa2); MUL(smallExp/64,pa2,pa1); SQR(pa1,pa2); 
	SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SWAPPA;
      } else {
	abort();
      }
    } else {
      abort();
    }
  }

  res = s_mp_redc(pa1, mmm);
  mp_exch(pa1, result);

CLEANUP:
  mp_clear(&accum1);
  mp_clear(&accum2);
  mp_clear(&power2);
  for (i = 0; i < odd_ints; ++i) {
    mp_clear(oddPowers + i);
  }
  return res;
}
#undef SQR
#undef MUL

#ifdef MP_USING_CACHE_SAFE_MOD_EXP
unsigned int mp_using_cache_safe_exp = 1;
#endif

mp_err mp_set_safe_modexp(int value) 
{
#ifdef MP_USING_CACHE_SAFE_MOD_EXP
 mp_using_cache_safe_exp = value;
 return MP_OKAY;
#else
 if (value == 0) {
   return MP_OKAY;
 }
 return MP_BADARG;
#endif
}

#ifdef MP_USING_CACHE_SAFE_MOD_EXP
#define WEAVE_WORD_SIZE 4

#ifndef MP_CHAR_STORE_SLOW














































mp_err mpi_to_weave(const mp_int  *bignums, 
                    unsigned char *weaved, 
		    mp_size nDigits,  
		    mp_size nBignums) 
{
  mp_size i;
  unsigned char * endDest = weaved + (nDigits * nBignums * sizeof(mp_digit));

  for (i=0; i < WEAVE_WORD_SIZE; i++) {
    mp_size used = MP_USED(&bignums[i]);
    unsigned char *pSrc   = (unsigned char *)MP_DIGITS(&bignums[i]);
    unsigned char *endSrc = pSrc + (used * sizeof(mp_digit));
    unsigned char *pDest  = weaved + i;

    ARGCHK(MP_SIGN(&bignums[i]) == MP_ZPOS, MP_BADARG);
    ARGCHK(used <= nDigits, MP_BADARG);

    for (; pSrc < endSrc; pSrc++) {
      *pDest = *pSrc;
      pDest += nBignums;
    }
    while (pDest < endDest) {
      *pDest = 0;
      pDest += nBignums;
    }
  }

  return MP_OKAY;
}






mp_err weave_to_mpi(mp_int *a,                
                    const unsigned char *pSrc, 
		    mp_size nDigits,          
		    mp_size nBignums)         
{
  unsigned char *pDest   = (unsigned char *)MP_DIGITS(a);
  unsigned char *endDest = pDest + (nDigits * sizeof(mp_digit));

  MP_SIGN(a) = MP_ZPOS;
  MP_USED(a) = nDigits;

  for (; pDest < endDest; pSrc += nBignums, pDest++) {
    *pDest = *pSrc;
  }
  s_mp_clamp(a);
  return MP_OKAY;
}

#else



typedef unsigned int mp_weave_word;








































mp_err mpi_to_weave(const mp_int *a, unsigned char *b, 
					mp_size b_size, mp_size count)
{
  mp_size i;
  mp_digit *digitsa0;
  mp_digit *digitsa1;
  mp_digit *digitsa2;
  mp_digit *digitsa3;
  mp_size   useda0;
  mp_size   useda1;
  mp_size   useda2;
  mp_size   useda3;
  mp_weave_word *weaved = (mp_weave_word *)b;

  count = count/sizeof(mp_weave_word);

  
#if MP_ARGCHK == 2
  assert(WEAVE_WORD_SIZE == 4); 
  assert(sizeof(mp_weave_word) == 4);
#endif

  digitsa0 = MP_DIGITS(&a[0]);
  digitsa1 = MP_DIGITS(&a[1]);
  digitsa2 = MP_DIGITS(&a[2]);
  digitsa3 = MP_DIGITS(&a[3]);
  useda0 = MP_USED(&a[0]);
  useda1 = MP_USED(&a[1]);
  useda2 = MP_USED(&a[2]);
  useda3 = MP_USED(&a[3]);

  ARGCHK(MP_SIGN(&a[0]) == MP_ZPOS, MP_BADARG);
  ARGCHK(MP_SIGN(&a[1]) == MP_ZPOS, MP_BADARG);
  ARGCHK(MP_SIGN(&a[2]) == MP_ZPOS, MP_BADARG);
  ARGCHK(MP_SIGN(&a[3]) == MP_ZPOS, MP_BADARG);
  ARGCHK(useda0 <= b_size, MP_BADARG);
  ARGCHK(useda1 <= b_size, MP_BADARG);
  ARGCHK(useda2 <= b_size, MP_BADARG);
  ARGCHK(useda3 <= b_size, MP_BADARG);

#define SAFE_FETCH(digit, used, word) ((word) < (used) ? (digit[word]) : 0)

  for (i=0; i < b_size; i++) {
    mp_digit d0 = SAFE_FETCH(digitsa0,useda0,i);
    mp_digit d1 = SAFE_FETCH(digitsa1,useda1,i);
    mp_digit d2 = SAFE_FETCH(digitsa2,useda2,i);
    mp_digit d3 = SAFE_FETCH(digitsa3,useda3,i);
    register mp_weave_word acc;




















#ifdef MP_IS_LITTLE_ENDIAN 
#define MPI_WEAVE_ONE_STEP \
    acc  = (d0 >> (MP_DIGIT_BIT-8))  & 0x000000ff; d0 <<= 8; /*b0*/ \
    acc |= (d1 >> (MP_DIGIT_BIT-16)) & 0x0000ff00; d1 <<= 8; /*b1*/ \
    acc |= (d2 >> (MP_DIGIT_BIT-24)) & 0x00ff0000; d2 <<= 8; /*b2*/ \
    acc |= (d3 >> (MP_DIGIT_BIT-32)) & 0xff000000; d3 <<= 8; /*b3*/ \
    *weaved = acc; weaved += count;
#else 
#define MPI_WEAVE_ONE_STEP \
    acc  = (d0 >> (MP_DIGIT_BIT-32)) & 0xff000000; d0 <<= 8; /*b0*/ \
    acc |= (d1 >> (MP_DIGIT_BIT-24)) & 0x00ff0000; d1 <<= 8; /*b1*/ \
    acc |= (d2 >> (MP_DIGIT_BIT-16)) & 0x0000ff00; d2 <<= 8; /*b2*/ \
    acc |= (d3 >> (MP_DIGIT_BIT-8))  & 0x000000ff; d3 <<= 8; /*b3*/ \
    *weaved = acc; weaved += count;
#endif 
   switch (sizeof(mp_digit)) {
   case 32:
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
   case 16:
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
   case 8:
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
   case 4:
    MPI_WEAVE_ONE_STEP
    MPI_WEAVE_ONE_STEP
   case 2:
    MPI_WEAVE_ONE_STEP
   case 1:
    MPI_WEAVE_ONE_STEP
    break;
   }
  }

  return MP_OKAY;
}




mp_err weave_to_mpi(mp_int *a, const unsigned char *b, 
					mp_size b_size, mp_size count)
{
  mp_digit *pb = MP_DIGITS(a);
  mp_digit *end = &pb[b_size];

  MP_SIGN(a) = MP_ZPOS;
  MP_USED(a) = b_size;

  for (; pb < end; pb++) {
    register mp_digit digit;

    digit = *b << 8; b += count;
#define MPI_UNWEAVE_ONE_STEP  digit |= *b; b += count; digit = digit << 8;
    switch (sizeof(mp_digit)) {
    case 32:
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
    case 16:
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
    case 8:
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
    case 4:
	MPI_UNWEAVE_ONE_STEP 
	MPI_UNWEAVE_ONE_STEP 
    case 2:
	break;
    }
    digit |= *b; b += count; 

    *pb = digit;
  }
  s_mp_clamp(a);
  return MP_OKAY;
}
#endif


#define SQR(a,b) \
  MP_CHECKOK( mp_sqr(a, b) );\
  MP_CHECKOK( s_mp_redc(b, mmm) )

#if defined(MP_MONT_USE_MP_MUL)
#define MUL_NOWEAVE(x,a,b) \
  MP_CHECKOK( mp_mul(a, x, b) ); \
  MP_CHECKOK( s_mp_redc(b, mmm) ) 
#else
#define MUL_NOWEAVE(x,a,b) \
  MP_CHECKOK( s_mp_mul_mont(a, x, b, mmm) )
#endif

#define MUL(x,a,b) \
  MP_CHECKOK( weave_to_mpi(&tmp, powers + (x), nLen, num_powers) ); \
  MUL_NOWEAVE(&tmp,a,b)

#define SWAPPA ptmp = pa1; pa1 = pa2; pa2 = ptmp
#define MP_ALIGN(x,y) ((((ptrdiff_t)(x))+((y)-1))&(((ptrdiff_t)0)-(y)))


mp_err mp_exptmod_safe_i(const mp_int *   montBase, 
                    const mp_int *   exponent, 
		    const mp_int *   modulus, 
		    mp_int *         result, 
		    mp_mont_modulus *mmm, 
		    int              nLen, 
		    mp_size          bits_in_exponent, 
		    mp_size          window_bits,
		    mp_size          num_powers)
{
  mp_int *pa1, *pa2, *ptmp;
  mp_size i;
  mp_size first_window;
  mp_err  res;
  int     expOff;
  mp_int  accum1, accum2, accum[WEAVE_WORD_SIZE];
  mp_int  tmp;
  unsigned char *powersArray = NULL;
  unsigned char *powers = NULL;

  MP_DIGITS(&accum1) = 0;
  MP_DIGITS(&accum2) = 0;
  MP_DIGITS(&accum[0]) = 0;
  MP_DIGITS(&accum[1]) = 0;
  MP_DIGITS(&accum[2]) = 0;
  MP_DIGITS(&accum[3]) = 0;
  MP_DIGITS(&tmp) = 0;

  

  MP_CHECKOK( mpl_get_bits(exponent, 
				bits_in_exponent-window_bits, window_bits) );
  first_window = (mp_size)res;

  MP_CHECKOK( mp_init_size(&accum1, 3 * nLen + 2) );
  MP_CHECKOK( mp_init_size(&accum2, 3 * nLen + 2) );

  
  
  if (num_powers > 2) {
    MP_CHECKOK( mp_init_size(&accum[0], 3 * nLen + 2) );
    MP_CHECKOK( mp_init_size(&accum[1], 3 * nLen + 2) );
    MP_CHECKOK( mp_init_size(&accum[2], 3 * nLen + 2) );
    MP_CHECKOK( mp_init_size(&accum[3], 3 * nLen + 2) );
    mp_set(&accum[0], 1);
    MP_CHECKOK( s_mp_to_mont(&accum[0], mmm, &accum[0]) );
    MP_CHECKOK( mp_copy(montBase, &accum[1]) );
    SQR(montBase, &accum[2]);
    MUL_NOWEAVE(montBase, &accum[2], &accum[3]);
    powersArray = (unsigned char *)malloc(num_powers*(nLen*sizeof(mp_digit)+1));
    if (!powersArray) {
      res = MP_MEM;
      goto CLEANUP;
    }
     \
    powers = (unsigned char *)MP_ALIGN(powersArray,num_powers); \
    MP_CHECKOK( mpi_to_weave(accum, powers, nLen, num_powers) );
    if (first_window < 4) {
      MP_CHECKOK( mp_copy(&accum[first_window], &accum1) );
      first_window = num_powers;
    }
  } else {
      if (first_window == 0) {
        mp_set(&accum1, 1);
        MP_CHECKOK( s_mp_to_mont(&accum1, mmm, &accum1) );
      } else {
        
        MP_CHECKOK( mp_copy(montBase, &accum1) );
      }
  }

  









  for (i = WEAVE_WORD_SIZE; i < num_powers; i++) {
    int acc_index = i & (WEAVE_WORD_SIZE-1); 
    if ( i & 1 ) {
      MUL_NOWEAVE(montBase, &accum[acc_index-1] , &accum[acc_index]);
      
      if (acc_index == (WEAVE_WORD_SIZE-1)) {
        MP_CHECKOK( mpi_to_weave(accum, powers + i - (WEAVE_WORD_SIZE-1),
							 nLen, num_powers) );

        if (first_window <= i) {
          MP_CHECKOK( mp_copy(&accum[first_window & (WEAVE_WORD_SIZE-1)], 
								&accum1) );
          first_window = num_powers;
        }
      }
    } else {
      



      if (i > 2* WEAVE_WORD_SIZE) {
        MP_CHECKOK(weave_to_mpi(&accum2, powers+i/2, nLen, num_powers));
        SQR(&accum2, &accum[acc_index]);
      } else {
	int half_power_index = (i/2) & (WEAVE_WORD_SIZE-1);
	if (half_power_index == acc_index) {
	   
	   MP_CHECKOK(mp_copy(&accum[half_power_index], &accum2));
	   SQR(&accum2,&accum[acc_index]);
	} else {
	   SQR(&accum[half_power_index],&accum[acc_index]);
	}
      }
    }
  }
  


#if MP_ARGCHK == 2
  assert(MP_USED(&accum1) != 0);
#endif

  
  pa1 = &accum1;
  pa2 = &accum2;

  
  if (window_bits != 1) {
    MP_CHECKOK( mp_init_size(&tmp, 3 * nLen + 2) );
  }

  for (expOff = bits_in_exponent - window_bits*2; expOff >= 0; expOff -= window_bits) {
    mp_size smallExp;
    MP_CHECKOK( mpl_get_bits(exponent, expOff, window_bits) );
    smallExp = (mp_size)res;

    
    switch (window_bits) {
    case 1:
	if (!smallExp) {
	    SQR(pa1,pa2); SWAPPA;
	} else if (smallExp & 1) {
	    SQR(pa1,pa2); MUL_NOWEAVE(montBase,pa2,pa1);
	} else {
	    abort();
	}
	break;
    case 6:
	SQR(pa1,pa2); SQR(pa2,pa1); 
	
    case 4:
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1);
	MUL(smallExp, pa1,pa2); SWAPPA;
	break;
    case 5:
	SQR(pa1,pa2); SQR(pa2,pa1); SQR(pa1,pa2); SQR(pa2,pa1); 
	SQR(pa1,pa2); MUL(smallExp,pa2,pa1);
	break;
    default:
	abort(); 
    }
  }

  res = s_mp_redc(pa1, mmm);
  mp_exch(pa1, result);

CLEANUP:
  mp_clear(&accum1);
  mp_clear(&accum2);
  mp_clear(&accum[0]);
  mp_clear(&accum[1]);
  mp_clear(&accum[2]);
  mp_clear(&accum[3]);
  mp_clear(&tmp);
  
  free(powersArray);
  return res;
}
#undef SQR
#undef MUL
#endif

mp_err mp_exptmod(const mp_int *inBase, const mp_int *exponent, 
		  const mp_int *modulus, mp_int *result)
{
  const mp_int *base;
  mp_size bits_in_exponent, i, window_bits, odd_ints;
  mp_err  res;
  int     nLen;
  mp_int  montBase, goodBase;
  mp_mont_modulus mmm;
#ifdef MP_USING_CACHE_SAFE_MOD_EXP
  static unsigned int max_window_bits;
#endif

  
  if (!mp_isodd(modulus))
    return s_mp_exptmod(inBase, exponent, modulus, result);

  MP_DIGITS(&montBase) = 0;
  MP_DIGITS(&goodBase) = 0;

  if (mp_cmp(inBase, modulus) < 0) {
    base = inBase;
  } else {
    MP_CHECKOK( mp_init(&goodBase) );
    base = &goodBase;
    MP_CHECKOK( mp_mod(inBase, modulus, &goodBase) );
  }

  nLen  = MP_USED(modulus);
  MP_CHECKOK( mp_init_size(&montBase, 2 * nLen + 2) );

  mmm.N = *modulus;			

  


  mmm.n0prime = 0 - s_mp_invmod_radix( MP_DIGIT(modulus, 0) );

  MP_CHECKOK( s_mp_to_mont(base, &mmm, &montBase) );

  bits_in_exponent = mpl_significant_bits(exponent);
#ifdef MP_USING_CACHE_SAFE_MOD_EXP
  if (mp_using_cache_safe_exp) {
    if (bits_in_exponent > 780)
	window_bits = 6;
    else if (bits_in_exponent > 256)
	window_bits = 5;
    else if (bits_in_exponent > 20)
	window_bits = 4;
       


    else 
	window_bits = 1;
  } else
#endif
  if (bits_in_exponent > 480)
    window_bits = 6;
  else if (bits_in_exponent > 160)
    window_bits = 5;
  else if (bits_in_exponent > 20)
    window_bits = 4;
  


  else 
    window_bits = 1;

#ifdef MP_USING_CACHE_SAFE_MOD_EXP
  



  if (!max_window_bits) {
    unsigned long cache_size = s_mpi_getProcessorLineSize();
    
    if (cache_size == 0) {
      mp_using_cache_safe_exp = 0;
    } 
    if ((cache_size == 0) || (cache_size >= 64)) {
      max_window_bits = 6;
    } else if (cache_size >= 32) {
      max_window_bits = 5;
    } else if (cache_size >= 16) {
      max_window_bits = 4;
    } else max_window_bits = 1; 
  }

  
  if (mp_using_cache_safe_exp) {
    if (window_bits > max_window_bits) {
      window_bits = max_window_bits;
    }
  }
#endif

  odd_ints = 1 << (window_bits - 1);
  i = bits_in_exponent % window_bits;
  if (i != 0) {
    bits_in_exponent += window_bits - i;
  } 

#ifdef MP_USING_MONT_MULF
  if (mp_using_mont_mulf) {
    MP_CHECKOK( s_mp_pad(&montBase, nLen) );
    res = mp_exptmod_f(&montBase, exponent, modulus, result, &mmm, nLen, 
		     bits_in_exponent, window_bits, odd_ints);
  } else
#endif
#ifdef MP_USING_CACHE_SAFE_MOD_EXP
  if (mp_using_cache_safe_exp) {
    res = mp_exptmod_safe_i(&montBase, exponent, modulus, result, &mmm, nLen, 
		     bits_in_exponent, window_bits, 1 << window_bits);
  } else
#endif
  res = mp_exptmod_i(&montBase, exponent, modulus, result, &mmm, nLen, 
		     bits_in_exponent, window_bits, odd_ints);

CLEANUP:
  mp_clear(&montBase);
  mp_clear(&goodBase);
  


  memset(&mmm, 0, sizeof mmm);
  return res;
}
