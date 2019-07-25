











































#include "mpi-priv.h"
#if defined(OSF1)
#include <c_asm.h>
#endif

#if MP_LOGTAB














#include "logtab.h"
#endif




static const char *mp_err_string[] = {
  "unknown result code",     
  "boolean true",            
  "boolean false",           
  "out of memory",           
  "argument out of range",   
  "invalid input parameter", 
  "result is undefined"      
};




static const char *s_dmap_1 = 
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";



unsigned long mp_allocs;
unsigned long mp_frees;
unsigned long mp_copies;




static mp_size s_mp_defprec = MP_DEFPREC;

mp_size mp_get_prec(void)
{
  return s_mp_defprec;

} 

void         mp_set_prec(mp_size prec)
{
  if(prec == 0)
    s_mp_defprec = MP_DEFPREC;
  else
    s_mp_defprec = prec;

} 













mp_err mp_init(mp_int *mp)
{
  return mp_init_size(mp, s_mp_defprec);

} 













mp_err mp_init_size(mp_int *mp, mp_size prec)
{
  ARGCHK(mp != NULL && prec > 0, MP_BADARG);

  prec = MP_ROUNDUP(prec, s_mp_defprec);
  if((DIGITS(mp) = s_mp_alloc(prec, sizeof(mp_digit))) == NULL)
    return MP_MEM;

  SIGN(mp) = ZPOS;
  USED(mp) = 1;
  ALLOC(mp) = prec;

  return MP_OKAY;

} 













mp_err mp_init_copy(mp_int *mp, const mp_int *from)
{
  ARGCHK(mp != NULL && from != NULL, MP_BADARG);

  if(mp == from)
    return MP_OKAY;

  if((DIGITS(mp) = s_mp_alloc(ALLOC(from), sizeof(mp_digit))) == NULL)
    return MP_MEM;

  s_mp_copy(DIGITS(from), DIGITS(mp), USED(from));
  USED(mp) = USED(from);
  ALLOC(mp) = ALLOC(from);
  SIGN(mp) = SIGN(from);

  return MP_OKAY;

} 













mp_err mp_copy(const mp_int *from, mp_int *to)
{
  ARGCHK(from != NULL && to != NULL, MP_BADARG);

  if(from == to)
    return MP_OKAY;

  { 
    mp_digit   *tmp;

    






    if(ALLOC(to) >= USED(from)) {
      s_mp_setz(DIGITS(to) + USED(from), ALLOC(to) - USED(from));
      s_mp_copy(DIGITS(from), DIGITS(to), USED(from));
      
    } else {
      if((tmp = s_mp_alloc(ALLOC(from), sizeof(mp_digit))) == NULL)
	return MP_MEM;

      s_mp_copy(DIGITS(from), tmp, USED(from));

      if(DIGITS(to) != NULL) {
#if MP_CRYPTO
	s_mp_setz(DIGITS(to), ALLOC(to));
#endif
	s_mp_free(DIGITS(to));
      }

      DIGITS(to) = tmp;
      ALLOC(to) = ALLOC(from);
    }

    
    USED(to) = USED(from);
    SIGN(to) = SIGN(from);
  } 

  return MP_OKAY;

} 













void mp_exch(mp_int *mp1, mp_int *mp2)
{
#if MP_ARGCHK == 2
  assert(mp1 != NULL && mp2 != NULL);
#else
  if(mp1 == NULL || mp2 == NULL)
    return;
#endif

  s_mp_exch(mp1, mp2);

} 













void   mp_clear(mp_int *mp)
{
  if(mp == NULL)
    return;

  if(DIGITS(mp) != NULL) {
#if MP_CRYPTO
    s_mp_setz(DIGITS(mp), ALLOC(mp));
#endif
    s_mp_free(DIGITS(mp));
    DIGITS(mp) = NULL;
  }

  USED(mp) = 0;
  ALLOC(mp) = 0;

} 











void   mp_zero(mp_int *mp)
{
  if(mp == NULL)
    return;

  s_mp_setz(DIGITS(mp), ALLOC(mp));
  USED(mp) = 1;
  SIGN(mp) = ZPOS;

} 





void   mp_set(mp_int *mp, mp_digit d)
{
  if(mp == NULL)
    return;

  mp_zero(mp);
  DIGIT(mp, 0) = d;

} 





mp_err mp_set_int(mp_int *mp, long z)
{
  int            ix;
  unsigned long  v = labs(z);
  mp_err         res;

  ARGCHK(mp != NULL, MP_BADARG);

  mp_zero(mp);
  if(z == 0)
    return MP_OKAY;  

  if (sizeof v <= sizeof(mp_digit)) {
    DIGIT(mp,0) = v;
  } else {
    for (ix = sizeof(long) - 1; ix >= 0; ix--) {
      if ((res = s_mp_mul_d(mp, (UCHAR_MAX + 1))) != MP_OKAY)
	return res;

      res = s_mp_add_d(mp, (mp_digit)((v >> (ix * CHAR_BIT)) & UCHAR_MAX));
      if (res != MP_OKAY)
	return res;
    }
  }
  if(z < 0)
    SIGN(mp) = NEG;

  return MP_OKAY;

} 





mp_err mp_set_ulong(mp_int *mp, unsigned long z)
{
  int            ix;
  mp_err         res;

  ARGCHK(mp != NULL, MP_BADARG);

  mp_zero(mp);
  if(z == 0)
    return MP_OKAY;  

  if (sizeof z <= sizeof(mp_digit)) {
    DIGIT(mp,0) = z;
  } else {
    for (ix = sizeof(long) - 1; ix >= 0; ix--) {
      if ((res = s_mp_mul_d(mp, (UCHAR_MAX + 1))) != MP_OKAY)
	return res;

      res = s_mp_add_d(mp, (mp_digit)((z >> (ix * CHAR_BIT)) & UCHAR_MAX));
      if (res != MP_OKAY)
	return res;
    }
  }
  return MP_OKAY;
} 















mp_err mp_add_d(const mp_int *a, mp_digit d, mp_int *b)
{
  mp_int   tmp;
  mp_err   res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if((res = mp_init_copy(&tmp, a)) != MP_OKAY)
    return res;

  if(SIGN(&tmp) == ZPOS) {
    if((res = s_mp_add_d(&tmp, d)) != MP_OKAY)
      goto CLEANUP;
  } else if(s_mp_cmp_d(&tmp, d) >= 0) {
    if((res = s_mp_sub_d(&tmp, d)) != MP_OKAY)
      goto CLEANUP;
  } else {
    mp_neg(&tmp, &tmp);

    DIGIT(&tmp, 0) = d - DIGIT(&tmp, 0);
  }

  if(s_mp_cmp_d(&tmp, 0) == 0)
    SIGN(&tmp) = ZPOS;

  s_mp_exch(&tmp, b);

CLEANUP:
  mp_clear(&tmp);
  return res;

} 












mp_err mp_sub_d(const mp_int *a, mp_digit d, mp_int *b)
{
  mp_int   tmp;
  mp_err   res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if((res = mp_init_copy(&tmp, a)) != MP_OKAY)
    return res;

  if(SIGN(&tmp) == NEG) {
    if((res = s_mp_add_d(&tmp, d)) != MP_OKAY)
      goto CLEANUP;
  } else if(s_mp_cmp_d(&tmp, d) >= 0) {
    if((res = s_mp_sub_d(&tmp, d)) != MP_OKAY)
      goto CLEANUP;
  } else {
    mp_neg(&tmp, &tmp);

    DIGIT(&tmp, 0) = d - DIGIT(&tmp, 0);
    SIGN(&tmp) = NEG;
  }

  if(s_mp_cmp_d(&tmp, 0) == 0)
    SIGN(&tmp) = ZPOS;

  s_mp_exch(&tmp, b);

CLEANUP:
  mp_clear(&tmp);
  return res;

} 












mp_err mp_mul_d(const mp_int *a, mp_digit d, mp_int *b)
{
  mp_err  res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if(d == 0) {
    mp_zero(b);
    return MP_OKAY;
  }

  if((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  res = s_mp_mul_d(b, d);

  return res;

} 





mp_err mp_mul_2(const mp_int *a, mp_int *c)
{
  mp_err  res;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if((res = mp_copy(a, c)) != MP_OKAY)
    return res;

  return s_mp_mul_2(c);

} 













mp_err mp_div_d(const mp_int *a, mp_digit d, mp_int *q, mp_digit *r)
{
  mp_err   res;
  mp_int   qp;
  mp_digit rem;
  int      pow;

  ARGCHK(a != NULL, MP_BADARG);

  if(d == 0)
    return MP_RANGE;

  
  if((pow = s_mp_ispow2d(d)) >= 0) {
    mp_digit  mask;

    mask = ((mp_digit)1 << pow) - 1;
    rem = DIGIT(a, 0) & mask;

    if(q) {
      mp_copy(a, q);
      s_mp_div_2d(q, pow);
    }

    if(r)
      *r = rem;

    return MP_OKAY;
  }

  if((res = mp_init_copy(&qp, a)) != MP_OKAY)
    return res;

  res = s_mp_div_d(&qp, d, &rem);

  if(s_mp_cmp_d(&qp, 0) == 0)
    SIGN(q) = ZPOS;

  if(r)
    *r = rem;

  if(q)
    s_mp_exch(&qp, q);

  mp_clear(&qp);
  return res;

} 











mp_err mp_div_2(const mp_int *a, mp_int *c)
{
  mp_err  res;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if((res = mp_copy(a, c)) != MP_OKAY)
    return res;

  s_mp_div_2(c);

  return MP_OKAY;

} 





mp_err mp_expt_d(const mp_int *a, mp_digit d, mp_int *c)
{
  mp_int   s, x;
  mp_err   res;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if((res = mp_init(&s)) != MP_OKAY)
    return res;
  if((res = mp_init_copy(&x, a)) != MP_OKAY)
    goto X;

  DIGIT(&s, 0) = 1;

  while(d != 0) {
    if(d & 1) {
      if((res = s_mp_mul(&s, &x)) != MP_OKAY)
	goto CLEANUP;
    }

    d /= 2;

    if((res = s_mp_sqr(&x)) != MP_OKAY)
      goto CLEANUP;
  }

  s_mp_exch(&s, c);

CLEANUP:
  mp_clear(&x);
X:
  mp_clear(&s);

  return res;

} 
















mp_err mp_abs(const mp_int *a, mp_int *b)
{
  mp_err   res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  SIGN(b) = ZPOS;

  return MP_OKAY;

} 











mp_err mp_neg(const mp_int *a, mp_int *b)
{
  mp_err   res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  if(s_mp_cmp_d(b, 0) == MP_EQ) 
    SIGN(b) = ZPOS;
  else 
    SIGN(b) = (SIGN(b) == NEG) ? ZPOS : NEG;

  return MP_OKAY;

} 











mp_err mp_add(const mp_int *a, const mp_int *b, mp_int *c)
{
  mp_err  res;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(SIGN(a) == SIGN(b)) { 
    MP_CHECKOK( s_mp_add_3arg(a, b, c) );
  } else if(s_mp_cmp(a, b) >= 0) {  
    MP_CHECKOK( s_mp_sub_3arg(a, b, c) );
  } else {                          
    MP_CHECKOK( s_mp_sub_3arg(b, a, c) );
  }

  if (s_mp_cmp_d(c, 0) == MP_EQ)
    SIGN(c) = ZPOS;

CLEANUP:
  return res;

} 











mp_err mp_sub(const mp_int *a, const mp_int *b, mp_int *c)
{
  mp_err  res;
  int     magDiff;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (a == b) {
    mp_zero(c);
    return MP_OKAY;
  }

  if (MP_SIGN(a) != MP_SIGN(b)) {
    MP_CHECKOK( s_mp_add_3arg(a, b, c) );
  } else if (!(magDiff = s_mp_cmp(a, b))) {
    mp_zero(c);
    res = MP_OKAY;
  } else if (magDiff > 0) {
    MP_CHECKOK( s_mp_sub_3arg(a, b, c) );
  } else {
    MP_CHECKOK( s_mp_sub_3arg(b, a, c) );
    MP_SIGN(c) = !MP_SIGN(a);
  }

  if (s_mp_cmp_d(c, 0) == MP_EQ)
    MP_SIGN(c) = MP_ZPOS;

CLEANUP:
  return res;

} 










mp_err   mp_mul(const mp_int *a, const mp_int *b, mp_int * c)
{
  mp_digit *pb;
  mp_int   tmp;
  mp_err   res;
  mp_size  ib;
  mp_size  useda, usedb;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if (a == c) {
    if ((res = mp_init_copy(&tmp, a)) != MP_OKAY)
      return res;
    if (a == b) 
      b = &tmp;
    a = &tmp;
  } else if (b == c) {
    if ((res = mp_init_copy(&tmp, b)) != MP_OKAY)
      return res;
    b = &tmp;
  } else {
    MP_DIGITS(&tmp) = 0;
  }

  if (MP_USED(a) < MP_USED(b)) {
    const mp_int *xch = b;	
    b = a;
    a = xch;
  }

  MP_USED(c) = 1; MP_DIGIT(c, 0) = 0;
  if((res = s_mp_pad(c, USED(a) + USED(b))) != MP_OKAY)
    goto CLEANUP;

#ifdef NSS_USE_COMBA
  if ((MP_USED(a) == MP_USED(b)) && IS_POWER_OF_2(MP_USED(b))) {
      if (MP_USED(a) == 4) {
          s_mp_mul_comba_4(a, b, c);
          goto CLEANUP;
      }
      if (MP_USED(a) == 8) {
          s_mp_mul_comba_8(a, b, c);
          goto CLEANUP;
      }
      if (MP_USED(a) == 16) {
          s_mp_mul_comba_16(a, b, c);
          goto CLEANUP;
      }
      if (MP_USED(a) == 32) {
          s_mp_mul_comba_32(a, b, c);
          goto CLEANUP;
      } 
  }
#endif

  pb = MP_DIGITS(b);
  s_mpv_mul_d(MP_DIGITS(a), MP_USED(a), *pb++, MP_DIGITS(c));

  
  useda = MP_USED(a);
  usedb = MP_USED(b);
  for (ib = 1; ib < usedb; ib++) {
    mp_digit b_i    = *pb++;

    
    if (b_i)
      s_mpv_mul_d_add(MP_DIGITS(a), useda, b_i, MP_DIGITS(c) + ib);
    else
      MP_DIGIT(c, ib + useda) = b_i;
  }

  s_mp_clamp(c);

  if(SIGN(a) == SIGN(b) || s_mp_cmp_d(c, 0) == MP_EQ)
    SIGN(c) = ZPOS;
  else
    SIGN(c) = NEG;

CLEANUP:
  mp_clear(&tmp);
  return res;
} 





#if MP_SQUARE









mp_err   mp_sqr(const mp_int *a, mp_int *sqr)
{
  mp_digit *pa;
  mp_digit d;
  mp_err   res;
  mp_size  ix;
  mp_int   tmp;
  int      count;

  ARGCHK(a != NULL && sqr != NULL, MP_BADARG);

  if (a == sqr) {
    if((res = mp_init_copy(&tmp, a)) != MP_OKAY)
      return res;
    a = &tmp;
  } else {
    DIGITS(&tmp) = 0;
    res = MP_OKAY;
  }

  ix = 2 * MP_USED(a);
  if (ix > MP_ALLOC(sqr)) {
    MP_USED(sqr) = 1; 
    MP_CHECKOK( s_mp_grow(sqr, ix) );
  } 
  MP_USED(sqr) = ix;
  MP_DIGIT(sqr, 0) = 0;

#ifdef NSS_USE_COMBA
  if (IS_POWER_OF_2(MP_USED(a))) {
      if (MP_USED(a) == 4) {
          s_mp_sqr_comba_4(a, sqr);
          goto CLEANUP;
      }
      if (MP_USED(a) == 8) {
          s_mp_sqr_comba_8(a, sqr);
          goto CLEANUP;
      }
      if (MP_USED(a) == 16) {
          s_mp_sqr_comba_16(a, sqr);
          goto CLEANUP;
      }
      if (MP_USED(a) == 32) {
          s_mp_sqr_comba_32(a, sqr);
          goto CLEANUP;
      } 
  }
#endif

  pa = MP_DIGITS(a);
  count = MP_USED(a) - 1;
  if (count > 0) {
    d = *pa++;
    s_mpv_mul_d(pa, count, d, MP_DIGITS(sqr) + 1);
    for (ix = 3; --count > 0; ix += 2) {
      d = *pa++;
      s_mpv_mul_d_add(pa, count, d, MP_DIGITS(sqr) + ix);
    } 
    MP_DIGIT(sqr, MP_USED(sqr)-1) = 0; 

    
    s_mp_mul_2(sqr);
  } else {
    MP_DIGIT(sqr, 1) = 0;
  }

  
  s_mpv_sqr_add_prop(MP_DIGITS(a), MP_USED(a), MP_DIGITS(sqr));

  SIGN(sqr) = ZPOS;
  s_mp_clamp(sqr);

CLEANUP:
  mp_clear(&tmp);
  return res;

} 
#endif












mp_err mp_div(const mp_int *a, const mp_int *b, mp_int *q, mp_int *r)
{
  mp_err   res;
  mp_int   *pQ, *pR;
  mp_int   qtmp, rtmp, btmp;
  int      cmp;
  mp_sign  signA;
  mp_sign  signB;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);
  
  signA = MP_SIGN(a);
  signB = MP_SIGN(b);

  if(mp_cmp_z(b) == MP_EQ)
    return MP_RANGE;

  DIGITS(&qtmp) = 0;
  DIGITS(&rtmp) = 0;
  DIGITS(&btmp) = 0;

  
  if (!r || r == a || r == b) {
    MP_CHECKOK( mp_init_copy(&rtmp, a) );
    pR = &rtmp;
  } else {
    MP_CHECKOK( mp_copy(a, r) );
    pR = r;
  }

  if (!q || q == a || q == b) {
    MP_CHECKOK( mp_init_size(&qtmp, MP_USED(a)) );
    pQ = &qtmp;
  } else {
    MP_CHECKOK( s_mp_pad(q, MP_USED(a)) );
    pQ = q;
    mp_zero(pQ);
  }

  



  if ((cmp = s_mp_cmp(a, b)) <= 0) {
    if (cmp) {
      
      mp_zero(pQ);
    } else {
      mp_set(pQ, 1);
      mp_zero(pR);
    }
  } else {
    MP_CHECKOK( mp_init_copy(&btmp, b) );
    MP_CHECKOK( s_mp_div(pR, &btmp, pQ) );
  }

  
  MP_SIGN(pR) = signA;   
   
  MP_SIGN(pQ) = (signA == signB) ? ZPOS : NEG;

  if(s_mp_cmp_d(pQ, 0) == MP_EQ)
    SIGN(pQ) = ZPOS;
  if(s_mp_cmp_d(pR, 0) == MP_EQ)
    SIGN(pR) = ZPOS;

  
  if(q && q != pQ) 
    s_mp_exch(pQ, q);

  if(r && r != pR) 
    s_mp_exch(pR, r);

CLEANUP:
  mp_clear(&btmp);
  mp_clear(&rtmp);
  mp_clear(&qtmp);

  return res;

} 





mp_err mp_div_2d(const mp_int *a, mp_digit d, mp_int *q, mp_int *r)
{
  mp_err  res;

  ARGCHK(a != NULL, MP_BADARG);

  if(q) {
    if((res = mp_copy(a, q)) != MP_OKAY)
      return res;
  }
  if(r) {
    if((res = mp_copy(a, r)) != MP_OKAY)
      return res;
  }
  if(q) {
    s_mp_div_2d(q, d);
  }
  if(r) {
    s_mp_mod_2d(r, d);
  }

  return MP_OKAY;

} 












mp_err mp_expt(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int   s, x;
  mp_err   res;
  mp_digit d;
  int      dig, bit;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(mp_cmp_z(b) < 0)
    return MP_RANGE;

  if((res = mp_init(&s)) != MP_OKAY)
    return res;

  mp_set(&s, 1);

  if((res = mp_init_copy(&x, a)) != MP_OKAY)
    goto X;

  
  for(dig = 0; dig < (USED(b) - 1); dig++) {
    d = DIGIT(b, dig);

    
    for(bit = 0; bit < DIGIT_BIT; bit++) {
      if(d & 1) {
	if((res = s_mp_mul(&s, &x)) != MP_OKAY) 
	  goto CLEANUP;
      }

      d >>= 1;
      
      if((res = s_mp_sqr(&x)) != MP_OKAY)
	goto CLEANUP;
    }
  }

  
  d = DIGIT(b, dig);

  while(d) {
    if(d & 1) {
      if((res = s_mp_mul(&s, &x)) != MP_OKAY)
	goto CLEANUP;
    }

    d >>= 1;

    if((res = s_mp_sqr(&x)) != MP_OKAY)
      goto CLEANUP;
  }
  
  if(mp_iseven(b))
    SIGN(&s) = SIGN(a);

  res = mp_copy(&s, c);

CLEANUP:
  mp_clear(&x);
X:
  mp_clear(&s);

  return res;

} 







mp_err mp_2expt(mp_int *a, mp_digit k)
{
  ARGCHK(a != NULL, MP_BADARG);

  return s_mp_2expt(a, k);

} 











mp_err mp_mod(const mp_int *a, const mp_int *m, mp_int *c)
{
  mp_err  res;
  int     mag;

  ARGCHK(a != NULL && m != NULL && c != NULL, MP_BADARG);

  if(SIGN(m) == NEG)
    return MP_RANGE;

  












  if((mag = s_mp_cmp(a, m)) > 0) {
    if((res = mp_div(a, m, NULL, c)) != MP_OKAY)
      return res;
    
    if(SIGN(c) == NEG) {
      if((res = mp_add(c, m, c)) != MP_OKAY)
	return res;
    }

  } else if(mag < 0) {
    if((res = mp_copy(a, c)) != MP_OKAY)
      return res;

    if(mp_cmp_z(a) < 0) {
      if((res = mp_add(c, m, c)) != MP_OKAY)
	return res;

    }
    
  } else {
    mp_zero(c);

  }

  return MP_OKAY;

} 










mp_err mp_mod_d(const mp_int *a, mp_digit d, mp_digit *c)
{
  mp_err   res;
  mp_digit rem;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if(s_mp_cmp_d(a, d) > 0) {
    if((res = mp_div_d(a, d, NULL, &rem)) != MP_OKAY)
      return res;

  } else {
    if(SIGN(a) == NEG)
      rem = d - DIGIT(a, 0);
    else
      rem = DIGIT(a, 0);
  }

  if(c)
    *c = rem;

  return MP_OKAY;

} 


















mp_err mp_sqrt(const mp_int *a, mp_int *b)
{
  mp_int   x, t;
  mp_err   res;
  mp_size  used;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  
  if(SIGN(a) == NEG)
    return MP_RANGE;

  
  if(mp_cmp_d(a, 1) <= 0)
    return mp_copy(a, b);
    
  
  if((res = mp_init_size(&t, USED(a))) != MP_OKAY)
    return res;

  
  if((res = mp_init_copy(&x, a)) != MP_OKAY)
    goto X;

  used = MP_USED(&x);
  if (used > 1) {
    s_mp_rshd(&x, used / 2);
  }

  for(;;) {
    
    mp_copy(&x, &t);      
    if((res = mp_sqr(&t, &t)) != MP_OKAY ||
       (res = mp_sub(&t, a, &t)) != MP_OKAY)
      goto CLEANUP;

    
    s_mp_mul_2(&x);
    if((res = mp_div(&t, &x, &t, NULL)) != MP_OKAY)
      goto CLEANUP;
    s_mp_div_2(&x);

    
    if(mp_cmp_z(&t) == MP_EQ)
      break;

    
    if((res = mp_sub(&x, &t, &x)) != MP_OKAY)
      goto CLEANUP;

  }

  
  mp_sub_d(&x, 1, &x);
  s_mp_exch(&x, b);

 CLEANUP:
  mp_clear(&x);
 X:
  mp_clear(&t); 

  return res;

} 








#if MP_MODARITH








mp_err mp_addmod(const mp_int *a, const mp_int *b, const mp_int *m, mp_int *c)
{
  mp_err  res;

  ARGCHK(a != NULL && b != NULL && m != NULL && c != NULL, MP_BADARG);

  if((res = mp_add(a, b, c)) != MP_OKAY)
    return res;
  if((res = mp_mod(c, m, c)) != MP_OKAY)
    return res;

  return MP_OKAY;

}











mp_err mp_submod(const mp_int *a, const mp_int *b, const mp_int *m, mp_int *c)
{
  mp_err  res;

  ARGCHK(a != NULL && b != NULL && m != NULL && c != NULL, MP_BADARG);

  if((res = mp_sub(a, b, c)) != MP_OKAY)
    return res;
  if((res = mp_mod(c, m, c)) != MP_OKAY)
    return res;

  return MP_OKAY;

}











mp_err mp_mulmod(const mp_int *a, const mp_int *b, const mp_int *m, mp_int *c)
{
  mp_err  res;

  ARGCHK(a != NULL && b != NULL && m != NULL && c != NULL, MP_BADARG);

  if((res = mp_mul(a, b, c)) != MP_OKAY)
    return res;
  if((res = mp_mod(c, m, c)) != MP_OKAY)
    return res;

  return MP_OKAY;

}





#if MP_SQUARE
mp_err mp_sqrmod(const mp_int *a, const mp_int *m, mp_int *c)
{
  mp_err  res;

  ARGCHK(a != NULL && m != NULL && c != NULL, MP_BADARG);

  if((res = mp_sqr(a, c)) != MP_OKAY)
    return res;
  if((res = mp_mod(c, m, c)) != MP_OKAY)
    return res;

  return MP_OKAY;

} 
#endif
















mp_err s_mp_exptmod(const mp_int *a, const mp_int *b, const mp_int *m, mp_int *c)
{
  mp_int   s, x, mu;
  mp_err   res;
  mp_digit d;
  int      dig, bit;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(mp_cmp_z(b) < 0 || mp_cmp_z(m) <= 0)
    return MP_RANGE;

  if((res = mp_init(&s)) != MP_OKAY)
    return res;
  if((res = mp_init_copy(&x, a)) != MP_OKAY ||
     (res = mp_mod(&x, m, &x)) != MP_OKAY)
    goto X;
  if((res = mp_init(&mu)) != MP_OKAY)
    goto MU;

  mp_set(&s, 1);

  
  s_mp_add_d(&mu, 1); 
  s_mp_lshd(&mu, 2 * USED(m));
  if((res = mp_div(&mu, m, &mu, NULL)) != MP_OKAY)
    goto CLEANUP;

  
  for(dig = 0; dig < (USED(b) - 1); dig++) {
    d = DIGIT(b, dig);

    
    for(bit = 0; bit < DIGIT_BIT; bit++) {
      if(d & 1) {
	if((res = s_mp_mul(&s, &x)) != MP_OKAY)
	  goto CLEANUP;
	if((res = s_mp_reduce(&s, m, &mu)) != MP_OKAY)
	  goto CLEANUP;
      }

      d >>= 1;

      if((res = s_mp_sqr(&x)) != MP_OKAY)
	goto CLEANUP;
      if((res = s_mp_reduce(&x, m, &mu)) != MP_OKAY)
	goto CLEANUP;
    }
  }

  
  d = DIGIT(b, dig);

  while(d) {
    if(d & 1) {
      if((res = s_mp_mul(&s, &x)) != MP_OKAY)
	goto CLEANUP;
      if((res = s_mp_reduce(&s, m, &mu)) != MP_OKAY)
	goto CLEANUP;
    }

    d >>= 1;

    if((res = s_mp_sqr(&x)) != MP_OKAY)
      goto CLEANUP;
    if((res = s_mp_reduce(&x, m, &mu)) != MP_OKAY)
      goto CLEANUP;
  }

  s_mp_exch(&s, c);

 CLEANUP:
  mp_clear(&mu);
 MU:
  mp_clear(&x);
 X:
  mp_clear(&s);

  return res;

} 





mp_err mp_exptmod_d(const mp_int *a, mp_digit d, const mp_int *m, mp_int *c)
{
  mp_int   s, x;
  mp_err   res;

  ARGCHK(a != NULL && c != NULL, MP_BADARG);

  if((res = mp_init(&s)) != MP_OKAY)
    return res;
  if((res = mp_init_copy(&x, a)) != MP_OKAY)
    goto X;

  mp_set(&s, 1);

  while(d != 0) {
    if(d & 1) {
      if((res = s_mp_mul(&s, &x)) != MP_OKAY ||
	 (res = mp_mod(&s, m, &s)) != MP_OKAY)
	goto CLEANUP;
    }

    d /= 2;

    if((res = s_mp_sqr(&x)) != MP_OKAY ||
       (res = mp_mod(&x, m, &x)) != MP_OKAY)
      goto CLEANUP;
  }

  s_mp_exch(&s, c);

CLEANUP:
  mp_clear(&x);
X:
  mp_clear(&s);

  return res;

} 


#endif 














int    mp_cmp_z(const mp_int *a)
{
  if(SIGN(a) == NEG)
    return MP_LT;
  else if(USED(a) == 1 && DIGIT(a, 0) == 0)
    return MP_EQ;
  else
    return MP_GT;

} 











int    mp_cmp_d(const mp_int *a, mp_digit d)
{
  ARGCHK(a != NULL, MP_EQ);

  if(SIGN(a) == NEG)
    return MP_LT;

  return s_mp_cmp_d(a, d);

} 





int    mp_cmp(const mp_int *a, const mp_int *b)
{
  ARGCHK(a != NULL && b != NULL, MP_EQ);

  if(SIGN(a) == SIGN(b)) {
    int  mag;

    if((mag = s_mp_cmp(a, b)) == MP_EQ)
      return MP_EQ;

    if(SIGN(a) == ZPOS)
      return mag;
    else
      return -mag;

  } else if(SIGN(a) == ZPOS) {
    return MP_GT;
  } else {
    return MP_LT;
  }

} 











int    mp_cmp_mag(mp_int *a, mp_int *b)
{
  ARGCHK(a != NULL && b != NULL, MP_EQ);

  return s_mp_cmp(a, b);

} 











int    mp_cmp_int(const mp_int *a, long z)
{
  mp_int  tmp;
  int     out;

  ARGCHK(a != NULL, MP_EQ);
  
  mp_init(&tmp); mp_set_int(&tmp, z);
  out = mp_cmp(a, &tmp);
  mp_clear(&tmp);

  return out;

} 










int    mp_isodd(const mp_int *a)
{
  ARGCHK(a != NULL, 0);

  return (int)(DIGIT(a, 0) & 1);

} 





int    mp_iseven(const mp_int *a)
{
  return !mp_isodd(a);

} 








#if MP_NUMTH






mp_err mp_gcd(mp_int *a, mp_int *b, mp_int *c)
{
  mp_err   res;
  mp_int   u, v, t;
  mp_size  k = 0;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(mp_cmp_z(a) == MP_EQ && mp_cmp_z(b) == MP_EQ)
      return MP_RANGE;
  if(mp_cmp_z(a) == MP_EQ) {
    return mp_copy(b, c);
  } else if(mp_cmp_z(b) == MP_EQ) {
    return mp_copy(a, c);
  }

  if((res = mp_init(&t)) != MP_OKAY)
    return res;
  if((res = mp_init_copy(&u, a)) != MP_OKAY)
    goto U;
  if((res = mp_init_copy(&v, b)) != MP_OKAY)
    goto V;

  SIGN(&u) = ZPOS;
  SIGN(&v) = ZPOS;

  
  while(mp_iseven(&u) && mp_iseven(&v)) {
    s_mp_div_2(&u);
    s_mp_div_2(&v);
    ++k;
  }

  
  if(mp_isodd(&u)) {
    if((res = mp_copy(&v, &t)) != MP_OKAY)
      goto CLEANUP;
    
    
    if(SIGN(&v) == ZPOS)
      SIGN(&t) = NEG;
    else
      SIGN(&t) = ZPOS;
    
  } else {
    if((res = mp_copy(&u, &t)) != MP_OKAY)
      goto CLEANUP;

  }

  for(;;) {
    while(mp_iseven(&t)) {
      s_mp_div_2(&t);
    }

    if(mp_cmp_z(&t) == MP_GT) {
      if((res = mp_copy(&t, &u)) != MP_OKAY)
	goto CLEANUP;

    } else {
      if((res = mp_copy(&t, &v)) != MP_OKAY)
	goto CLEANUP;

      
      if(SIGN(&t) == ZPOS)
	SIGN(&v) = NEG;
      else
	SIGN(&v) = ZPOS;
    }

    if((res = mp_sub(&u, &v, &t)) != MP_OKAY)
      goto CLEANUP;

    if(s_mp_cmp_d(&t, 0) == MP_EQ)
      break;
  }

  s_mp_2expt(&v, k);       
  res = mp_mul(&u, &v, c); 

 CLEANUP:
  mp_clear(&v);
 V:
  mp_clear(&u);
 U:
  mp_clear(&t);

  return res;

} 












mp_err mp_lcm(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int  gcd, prod;
  mp_err  res;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  
  if((res = mp_init(&gcd)) != MP_OKAY)
    return res;
  if((res = mp_init(&prod)) != MP_OKAY)
    goto GCD;

  if((res = mp_mul(a, b, &prod)) != MP_OKAY)
    goto CLEANUP;
  if((res = mp_gcd(a, b, &gcd)) != MP_OKAY)
    goto CLEANUP;

  res = mp_div(&prod, &gcd, c, NULL);

 CLEANUP:
  mp_clear(&prod);
 GCD:
  mp_clear(&gcd);

  return res;

} 














mp_err mp_xgcd(const mp_int *a, const mp_int *b, mp_int *g, mp_int *x, mp_int *y)
{
  mp_int   gx, xc, yc, u, v, A, B, C, D;
  mp_int  *clean[9];
  mp_err   res;
  int      last = -1;

  if(mp_cmp_z(b) == 0)
    return MP_RANGE;

  
  MP_CHECKOK( mp_init(&u) );
  clean[++last] = &u;
  MP_CHECKOK( mp_init(&v) );
  clean[++last] = &v;
  MP_CHECKOK( mp_init(&gx) );
  clean[++last] = &gx;
  MP_CHECKOK( mp_init(&A) );
  clean[++last] = &A;
  MP_CHECKOK( mp_init(&B) );
  clean[++last] = &B;
  MP_CHECKOK( mp_init(&C) );
  clean[++last] = &C;
  MP_CHECKOK( mp_init(&D) );
  clean[++last] = &D;
  MP_CHECKOK( mp_init_copy(&xc, a) );
  clean[++last] = &xc;
  mp_abs(&xc, &xc);
  MP_CHECKOK( mp_init_copy(&yc, b) );
  clean[++last] = &yc;
  mp_abs(&yc, &yc);

  mp_set(&gx, 1);

  
  while(mp_iseven(&xc) && mp_iseven(&yc)) {
    mp_size nx = mp_trailing_zeros(&xc);
    mp_size ny = mp_trailing_zeros(&yc);
    mp_size n  = MP_MIN(nx, ny);
    s_mp_div_2d(&xc,n);
    s_mp_div_2d(&yc,n);
    MP_CHECKOK( s_mp_mul_2d(&gx,n) );
  }

  mp_copy(&xc, &u);
  mp_copy(&yc, &v);
  mp_set(&A, 1); mp_set(&D, 1);

  
  do {
    while(mp_iseven(&u)) {
      s_mp_div_2(&u);

      if(mp_iseven(&A) && mp_iseven(&B)) {
	s_mp_div_2(&A); s_mp_div_2(&B);
      } else {
	MP_CHECKOK( mp_add(&A, &yc, &A) );
	s_mp_div_2(&A);
	MP_CHECKOK( mp_sub(&B, &xc, &B) );
	s_mp_div_2(&B);
      }
    }

    while(mp_iseven(&v)) {
      s_mp_div_2(&v);

      if(mp_iseven(&C) && mp_iseven(&D)) {
	s_mp_div_2(&C); s_mp_div_2(&D);
      } else {
	MP_CHECKOK( mp_add(&C, &yc, &C) );
	s_mp_div_2(&C);
	MP_CHECKOK( mp_sub(&D, &xc, &D) );
	s_mp_div_2(&D);
      }
    }

    if(mp_cmp(&u, &v) >= 0) {
      MP_CHECKOK( mp_sub(&u, &v, &u) );
      MP_CHECKOK( mp_sub(&A, &C, &A) );
      MP_CHECKOK( mp_sub(&B, &D, &B) );
    } else {
      MP_CHECKOK( mp_sub(&v, &u, &v) );
      MP_CHECKOK( mp_sub(&C, &A, &C) );
      MP_CHECKOK( mp_sub(&D, &B, &D) );
    }
  } while (mp_cmp_z(&u) != 0);

  
  if(x)
    MP_CHECKOK( mp_copy(&C, x) );

  if(y)
    MP_CHECKOK( mp_copy(&D, y) );
      
  if(g)
    MP_CHECKOK( mp_mul(&gx, &v, g) );

 CLEANUP:
  while(last >= 0)
    mp_clear(clean[last--]);

  return res;

} 



mp_size mp_trailing_zeros(const mp_int *mp)
{
  mp_digit d;
  mp_size  n = 0;
  int      ix;

  if (!mp || !MP_DIGITS(mp) || !mp_cmp_z(mp))
    return n;

  for (ix = 0; !(d = MP_DIGIT(mp,ix)) && (ix < MP_USED(mp)); ++ix)
    n += MP_DIGIT_BIT;
  if (!d)
    return 0;	
#if !defined(MP_USE_UINT_DIGIT)
  if (!(d & 0xffffffffU)) {
    d >>= 32;
    n  += 32;
  }
#endif
  if (!(d & 0xffffU)) {
    d >>= 16;
    n  += 16;
  }
  if (!(d & 0xffU)) {
    d >>= 8;
    n  += 8;
  }
  if (!(d & 0xfU)) {
    d >>= 4;
    n  += 4;
  }
  if (!(d & 0x3U)) {
    d >>= 2;
    n  += 2;
  }
  if (!(d & 0x1U)) {
    d >>= 1;
    n  += 1;
  }
#if MP_ARGCHK == 2
  assert(0 != (d & 1));
#endif
  return n;
}






mp_err s_mp_almost_inverse(const mp_int *a, const mp_int *p, mp_int *c)
{
  mp_err res;
  mp_err k    = 0;
  mp_int d, f, g;

  ARGCHK(a && p && c, MP_BADARG);

  MP_DIGITS(&d) = 0;
  MP_DIGITS(&f) = 0;
  MP_DIGITS(&g) = 0;
  MP_CHECKOK( mp_init(&d) );
  MP_CHECKOK( mp_init_copy(&f, a) );	
  MP_CHECKOK( mp_init_copy(&g, p) );	

  mp_set(c, 1);
  mp_zero(&d);

  if (mp_cmp_z(&f) == 0) {
    res = MP_UNDEF;
  } else 
  for (;;) {
    int diff_sign;
    while (mp_iseven(&f)) {
      mp_size n = mp_trailing_zeros(&f);
      if (!n) {
	res = MP_UNDEF;
	goto CLEANUP;
      }
      s_mp_div_2d(&f, n);
      MP_CHECKOK( s_mp_mul_2d(&d, n) );
      k += n;
    }
    if (mp_cmp_d(&f, 1) == MP_EQ) {	
      res = k;
      break;
    }
    diff_sign = mp_cmp(&f, &g);
    if (diff_sign < 0) {		
      s_mp_exch(&f, &g);
      s_mp_exch(c, &d);
    } else if (diff_sign == 0) {		
      res = MP_UNDEF;		
      break;
    }
    if ((MP_DIGIT(&f,0) % 4) == (MP_DIGIT(&g,0) % 4)) {
      MP_CHECKOK( mp_sub(&f, &g, &f) );	
      MP_CHECKOK( mp_sub(c,  &d,  c) );	
    } else {
      MP_CHECKOK( mp_add(&f, &g, &f) );	
      MP_CHECKOK( mp_add(c,  &d,  c) );	
    }
  }
  if (res >= 0) {
    while (MP_SIGN(c) != MP_ZPOS) {
      MP_CHECKOK( mp_add(c, p, c) );
    }
    res = k;
  }

CLEANUP:
  mp_clear(&d);
  mp_clear(&f);
  mp_clear(&g);
  return res;
}





mp_digit  s_mp_invmod_radix(mp_digit P)
{
  mp_digit T = P;
  T *= 2 - (P * T);
  T *= 2 - (P * T);
  T *= 2 - (P * T);
  T *= 2 - (P * T);
#if !defined(MP_USE_UINT_DIGIT)
  T *= 2 - (P * T);
  T *= 2 - (P * T);
#endif
  return T;
}






mp_err  s_mp_fixup_reciprocal(const mp_int *c, const mp_int *p, int k, mp_int *x)
{
  int      k_orig = k;
  mp_digit r;
  mp_size  ix;
  mp_err   res;

  if (mp_cmp_z(c) < 0) {		
    MP_CHECKOK( mp_add(c, p, x) );	
  } else {
    MP_CHECKOK( mp_copy(c, x) );	
  }

  
  ix = MP_HOWMANY(k, MP_DIGIT_BIT) + MP_USED(p) + 1;
  ix = MP_MAX(ix, MP_USED(x));
  MP_CHECKOK( s_mp_pad(x, ix) );

  r = 0 - s_mp_invmod_radix(MP_DIGIT(p,0));

  for (ix = 0; k > 0; ix++) {
    int      j = MP_MIN(k, MP_DIGIT_BIT);
    mp_digit v = r * MP_DIGIT(x, ix);
    if (j < MP_DIGIT_BIT) {
      v &= ((mp_digit)1 << j) - 1;	
    }
    s_mp_mul_d_add_offset(p, v, x, ix); 
    k -= j;
  }
  s_mp_clamp(x);
  s_mp_div_2d(x, k_orig);
  res = MP_OKAY;

CLEANUP:
  return res;
}


mp_err s_mp_invmod_odd_m(const mp_int *a, const mp_int *m, mp_int *c)
{
  int k;
  mp_err  res;
  mp_int  x;

  ARGCHK(a && m && c, MP_BADARG);

  if(mp_cmp_z(a) == 0 || mp_cmp_z(m) == 0)
    return MP_RANGE;
  if (mp_iseven(m))
    return MP_UNDEF;

  MP_DIGITS(&x) = 0;

  if (a == c) {
    if ((res = mp_init_copy(&x, a)) != MP_OKAY)
      return res;
    if (a == m) 
      m = &x;
    a = &x;
  } else if (m == c) {
    if ((res = mp_init_copy(&x, m)) != MP_OKAY)
      return res;
    m = &x;
  } else {
    MP_DIGITS(&x) = 0;
  }

  MP_CHECKOK( s_mp_almost_inverse(a, m, c) );
  k = res;
  MP_CHECKOK( s_mp_fixup_reciprocal(c, m, k, c) );
CLEANUP:
  mp_clear(&x);
  return res;
}


mp_err mp_invmod_xgcd(const mp_int *a, const mp_int *m, mp_int *c)
{
  mp_int  g, x;
  mp_err  res;

  ARGCHK(a && m && c, MP_BADARG);

  if(mp_cmp_z(a) == 0 || mp_cmp_z(m) == 0)
    return MP_RANGE;

  MP_DIGITS(&g) = 0;
  MP_DIGITS(&x) = 0;
  MP_CHECKOK( mp_init(&x) );
  MP_CHECKOK( mp_init(&g) );

  MP_CHECKOK( mp_xgcd(a, m, &g, &x, NULL) );

  if (mp_cmp_d(&g, 1) != MP_EQ) {
    res = MP_UNDEF;
    goto CLEANUP;
  }

  res = mp_mod(&x, m, c);
  SIGN(c) = SIGN(a);

CLEANUP:
  mp_clear(&x);
  mp_clear(&g);

  return res;
}



mp_err s_mp_invmod_2d(const mp_int *a, mp_size k, mp_int *c)
{
  mp_err res;
  mp_size ix = k + 4;
  mp_int t0, t1, val, tmp, two2k;

  static const mp_digit d2 = 2;
  static const mp_int two = { MP_ZPOS, 1, 1, (mp_digit *)&d2 };

  if (mp_iseven(a))
    return MP_UNDEF;
  if (k <= MP_DIGIT_BIT) {
    mp_digit i = s_mp_invmod_radix(MP_DIGIT(a,0));
    if (k < MP_DIGIT_BIT)
      i &= ((mp_digit)1 << k) - (mp_digit)1;
    mp_set(c, i);
    return MP_OKAY;
  }
  MP_DIGITS(&t0) = 0;
  MP_DIGITS(&t1) = 0;
  MP_DIGITS(&val) = 0;
  MP_DIGITS(&tmp) = 0;
  MP_DIGITS(&two2k) = 0;
  MP_CHECKOK( mp_init_copy(&val, a) );
  s_mp_mod_2d(&val, k);
  MP_CHECKOK( mp_init_copy(&t0, &val) );
  MP_CHECKOK( mp_init_copy(&t1, &t0)  );
  MP_CHECKOK( mp_init(&tmp) );
  MP_CHECKOK( mp_init(&two2k) );
  MP_CHECKOK( s_mp_2expt(&two2k, k) );
  do {
    MP_CHECKOK( mp_mul(&val, &t1, &tmp)  );
    MP_CHECKOK( mp_sub(&two, &tmp, &tmp) );
    MP_CHECKOK( mp_mul(&t1, &tmp, &t1)   );
    s_mp_mod_2d(&t1, k);
    while (MP_SIGN(&t1) != MP_ZPOS) {
      MP_CHECKOK( mp_add(&t1, &two2k, &t1) );
    }
    if (mp_cmp(&t1, &t0) == MP_EQ) 
      break;
    MP_CHECKOK( mp_copy(&t1, &t0) );
  } while (--ix > 0);
  if (!ix) {
    res = MP_UNDEF;
  } else {
    mp_exch(c, &t1);
  }

CLEANUP:
  mp_clear(&t0);
  mp_clear(&t1);
  mp_clear(&val);
  mp_clear(&tmp);
  mp_clear(&two2k);
  return res;
}

mp_err s_mp_invmod_even_m(const mp_int *a, const mp_int *m, mp_int *c)
{
  mp_err res;
  mp_size k;
  mp_int oddFactor, evenFactor;	
  mp_int oddPart, evenPart;	
  mp_int C2, tmp1, tmp2;

  
  

  if ((res = s_mp_ispow2(m)) >= 0) {
    k = res;
    return s_mp_invmod_2d(a, k, c);
  }
  MP_DIGITS(&oddFactor) = 0;
  MP_DIGITS(&evenFactor) = 0;
  MP_DIGITS(&oddPart) = 0;
  MP_DIGITS(&evenPart) = 0;
  MP_DIGITS(&C2)     = 0;
  MP_DIGITS(&tmp1)   = 0;
  MP_DIGITS(&tmp2)   = 0;

  MP_CHECKOK( mp_init_copy(&oddFactor, m) );    
  MP_CHECKOK( mp_init(&evenFactor) );
  MP_CHECKOK( mp_init(&oddPart) );
  MP_CHECKOK( mp_init(&evenPart) );
  MP_CHECKOK( mp_init(&C2)     );
  MP_CHECKOK( mp_init(&tmp1)   );
  MP_CHECKOK( mp_init(&tmp2)   );

  k = mp_trailing_zeros(m);
  s_mp_div_2d(&oddFactor, k);
  MP_CHECKOK( s_mp_2expt(&evenFactor, k) );

  
  MP_CHECKOK( s_mp_invmod_odd_m(a, &oddFactor, &oddPart) );
  
  MP_CHECKOK( s_mp_invmod_2d(   a,       k,    &evenPart) );

  
  



  
  MP_CHECKOK( s_mp_invmod_2d(&oddFactor, k,    &C2) );

  
  MP_CHECKOK( mp_sub(&evenPart, &oddPart,   &tmp1) );
  MP_CHECKOK( mp_mul(&tmp1,     &C2,        &tmp2) );
  s_mp_mod_2d(&tmp2, k);
  while (MP_SIGN(&tmp2) != MP_ZPOS) {
    MP_CHECKOK( mp_add(&tmp2, &evenFactor, &tmp2) );
  }

  
  MP_CHECKOK( mp_mul(&tmp2,     &oddFactor, c) );
  MP_CHECKOK( mp_add(&oddPart,  c,          c) );
  
  MP_CHECKOK( mp_mod(c,         m,          c) );

CLEANUP:
  mp_clear(&oddFactor);
  mp_clear(&evenFactor);
  mp_clear(&oddPart);
  mp_clear(&evenPart);
  mp_clear(&C2);
  mp_clear(&tmp1);
  mp_clear(&tmp2);
  return res;
}












mp_err mp_invmod(const mp_int *a, const mp_int *m, mp_int *c)
{

  ARGCHK(a && m && c, MP_BADARG);

  if(mp_cmp_z(a) == 0 || mp_cmp_z(m) == 0)
    return MP_RANGE;

  if (mp_isodd(m)) {
    return s_mp_invmod_odd_m(a, m, c);
  }
  if (mp_iseven(a))
    return MP_UNDEF;	

  return s_mp_invmod_even_m(a, m, c);

} 


#endif 






#if MP_IOFUNC







void   mp_print(mp_int *mp, FILE *ofp)
{
  int   ix;

  if(mp == NULL || ofp == NULL)
    return;

  fputc((SIGN(mp) == NEG) ? '-' : '+', ofp);

  for(ix = USED(mp) - 1; ix >= 0; ix--) {
    fprintf(ofp, DIGIT_FMT, DIGIT(mp, ix));
  }

} 

#endif 














mp_err  mp_read_raw(mp_int *mp, char *str, int len)
{
  int            ix;
  mp_err         res;
  unsigned char *ustr = (unsigned char *)str;

  ARGCHK(mp != NULL && str != NULL && len > 0, MP_BADARG);

  mp_zero(mp);

  
  if(ustr[0])
    SIGN(mp) = NEG;
  else
    SIGN(mp) = ZPOS;

  
  for(ix = 1; ix < len; ix++) {
    if((res = mp_mul_d(mp, 256, mp)) != MP_OKAY)
      return res;
    if((res = mp_add_d(mp, ustr[ix], mp)) != MP_OKAY)
      return res;
  }

  return MP_OKAY;

} 





int    mp_raw_size(mp_int *mp)
{
  ARGCHK(mp != NULL, 0);

  return (USED(mp) * sizeof(mp_digit)) + 1;

} 





mp_err mp_toraw(mp_int *mp, char *str)
{
  int  ix, jx, pos = 1;

  ARGCHK(mp != NULL && str != NULL, MP_BADARG);

  str[0] = (char)SIGN(mp);

  
  for(ix = USED(mp) - 1; ix >= 0; ix--) {
    mp_digit  d = DIGIT(mp, ix);

    
    for(jx = sizeof(mp_digit) - 1; jx >= 0; jx--) {
      str[pos++] = (char)(d >> (jx * CHAR_BIT));
    }
  }

  return MP_OKAY;

} 














mp_err  mp_read_radix(mp_int *mp, const char *str, int radix)
{
  int     ix = 0, val = 0;
  mp_err  res;
  mp_sign sig = ZPOS;

  ARGCHK(mp != NULL && str != NULL && radix >= 2 && radix <= MAX_RADIX, 
	 MP_BADARG);

  mp_zero(mp);

  
  while(str[ix] && 
	(s_mp_tovalue(str[ix], radix) < 0) && 
	str[ix] != '-' &&
	str[ix] != '+') {
    ++ix;
  }

  if(str[ix] == '-') {
    sig = NEG;
    ++ix;
  } else if(str[ix] == '+') {
    sig = ZPOS; 
    ++ix;
  }

  while((val = s_mp_tovalue(str[ix], radix)) >= 0) {
    if((res = s_mp_mul_d(mp, radix)) != MP_OKAY)
      return res;
    if((res = s_mp_add_d(mp, val)) != MP_OKAY)
      return res;
    ++ix;
  }

  if(s_mp_cmp_d(mp, 0) == MP_EQ)
    SIGN(mp) = ZPOS;
  else
    SIGN(mp) = sig;

  return MP_OKAY;

} 

mp_err mp_read_variable_radix(mp_int *a, const char * str, int default_radix)
{
  int     radix = default_radix;
  int     cx;
  mp_sign sig   = ZPOS;
  mp_err  res;

  
  while ((cx = *str) != 0 && 
	(s_mp_tovalue(cx, radix) < 0) && 
	cx != '-' &&
	cx != '+') {
    ++str;
  }

  if (cx == '-') {
    sig = NEG;
    ++str;
  } else if (cx == '+') {
    sig = ZPOS; 
    ++str;
  }

  if (str[0] == '0') {
    if ((str[1] | 0x20) == 'x') {
      radix = 16;
      str += 2;
    } else {
      radix = 8;
      str++;
    }
  }
  res = mp_read_radix(a, str, radix);
  if (res == MP_OKAY) {
    MP_SIGN(a) = (s_mp_cmp_d(a, 0) == MP_EQ) ? ZPOS : sig;
  }
  return res;
}





int    mp_radix_size(mp_int *mp, int radix)
{
  int  bits;

  if(!mp || radix < 2 || radix > MAX_RADIX)
    return 0;

  bits = USED(mp) * DIGIT_BIT - 1;
 
  return s_mp_outlen(bits, radix);

} 





mp_err mp_toradix(mp_int *mp, char *str, int radix)
{
  int  ix, pos = 0;

  ARGCHK(mp != NULL && str != NULL, MP_BADARG);
  ARGCHK(radix > 1 && radix <= MAX_RADIX, MP_RANGE);

  if(mp_cmp_z(mp) == MP_EQ) {
    str[0] = '0';
    str[1] = '\0';
  } else {
    mp_err   res;
    mp_int   tmp;
    mp_sign  sgn;
    mp_digit rem, rdx = (mp_digit)radix;
    char     ch;

    if((res = mp_init_copy(&tmp, mp)) != MP_OKAY)
      return res;

    
    sgn = SIGN(&tmp); SIGN(&tmp) = ZPOS;

    
    while(mp_cmp_z(&tmp) != 0) {
      if((res = mp_div_d(&tmp, rdx, &tmp, &rem)) != MP_OKAY) {
	mp_clear(&tmp);
	return res;
      }

      
      ch = s_mp_todigit(rem, radix, 0);

      str[pos++] = ch;
    }

    
    if(sgn == NEG)
      str[pos++] = '-';

    
    str[pos--] = '\0';

    
    ix = 0;
    while(ix < pos) {
      char tmp = str[ix];

      str[ix] = str[pos];
      str[pos] = tmp;
      ++ix;
      --pos;
    }
    
    mp_clear(&tmp);
  }

  return MP_OKAY;

} 





int    mp_tovalue(char ch, int r)
{
  return s_mp_tovalue(ch, r);

} 















const char  *mp_strerror(mp_err ec)
{
  int   aec = (ec < 0) ? -ec : ec;

  

  if(ec < MP_LAST_CODE || ec > MP_OKAY) {
    return mp_err_string[0];  
  } else {
    return mp_err_string[aec + 1];
  }

} 












mp_err   s_mp_grow(mp_int *mp, mp_size min)
{
  if(min > ALLOC(mp)) {
    mp_digit   *tmp;

    
    min = MP_ROUNDUP(min, s_mp_defprec);

    if((tmp = s_mp_alloc(min, sizeof(mp_digit))) == NULL)
      return MP_MEM;

    s_mp_copy(DIGITS(mp), tmp, USED(mp));

#if MP_CRYPTO
    s_mp_setz(DIGITS(mp), ALLOC(mp));
#endif
    s_mp_free(DIGITS(mp));
    DIGITS(mp) = tmp;
    ALLOC(mp) = min;
  }

  return MP_OKAY;

} 






mp_err   s_mp_pad(mp_int *mp, mp_size min)
{
  if(min > USED(mp)) {
    mp_err  res;

    
    if (min > ALLOC(mp)) {
      if ((res = s_mp_grow(mp, min)) != MP_OKAY)
	return res;
    } else {
      s_mp_setz(DIGITS(mp) + USED(mp), min - USED(mp));
    }

    
    USED(mp) = min;
  }

  return MP_OKAY;

} 





#if MP_MACRO == 0

void s_mp_setz(mp_digit *dp, mp_size count)
{
#if MP_MEMSET == 0
  int  ix;

  for(ix = 0; ix < count; ix++)
    dp[ix] = 0;
#else
  memset(dp, 0, count * sizeof(mp_digit));
#endif

} 
#endif





#if MP_MACRO == 0

void s_mp_copy(const mp_digit *sp, mp_digit *dp, mp_size count)
{
#if MP_MEMCPY == 0
  int  ix;

  for(ix = 0; ix < count; ix++)
    dp[ix] = sp[ix];
#else
  memcpy(dp, sp, count * sizeof(mp_digit));
#endif
  ++mp_copies;

} 
#endif





#if MP_MACRO == 0

void    *s_mp_alloc(size_t nb, size_t ni)
{
  ++mp_allocs;
  return calloc(nb, ni);

} 
#endif





#if MP_MACRO == 0

void     s_mp_free(void *ptr)
{
  if(ptr) {
    ++mp_frees;
    free(ptr);
  }
} 
#endif





#if MP_MACRO == 0

void     s_mp_clamp(mp_int *mp)
{
  mp_size used = MP_USED(mp);
  while (used > 1 && DIGIT(mp, used - 1) == 0)
    --used;
  MP_USED(mp) = used;
} 
#endif






void     s_mp_exch(mp_int *a, mp_int *b)
{
  mp_int   tmp;

  tmp = *a;
  *a = *b;
  *b = tmp;

} 















   

mp_err   s_mp_lshd(mp_int *mp, mp_size p)
{
  mp_err  res;
  mp_size pos;
  int     ix;

  if(p == 0)
    return MP_OKAY;

  if (MP_USED(mp) == 1 && MP_DIGIT(mp, 0) == 0)
    return MP_OKAY;

  if((res = s_mp_pad(mp, USED(mp) + p)) != MP_OKAY)
    return res;

  pos = USED(mp) - 1;

  
  for(ix = pos - p; ix >= 0; ix--) 
    DIGIT(mp, ix + p) = DIGIT(mp, ix);

  
  for(ix = 0; ix < p; ix++)
    DIGIT(mp, ix) = 0;

  return MP_OKAY;

} 









mp_err   s_mp_mul_2d(mp_int *mp, mp_digit d)
{
  mp_err   res;
  mp_digit dshift, bshift;
  mp_digit mask;

  ARGCHK(mp != NULL,  MP_BADARG);

  dshift = d / MP_DIGIT_BIT;
  bshift = d % MP_DIGIT_BIT;
  
  mask   = ((mp_digit)~0 << (MP_DIGIT_BIT - bshift)); 
  mask  &= MP_DIGIT(mp, MP_USED(mp) - 1);

  if (MP_OKAY != (res = s_mp_pad(mp, MP_USED(mp) + dshift + (mask != 0) )))
    return res;

  if (dshift && MP_OKAY != (res = s_mp_lshd(mp, dshift)))
    return res;

  if (bshift) { 
    mp_digit *pa = MP_DIGITS(mp);
    mp_digit *alim = pa + MP_USED(mp);
    mp_digit  prev = 0;

    for (pa += dshift; pa < alim; ) {
      mp_digit x = *pa;
      *pa++ = (x << bshift) | prev;
      prev = x >> (DIGIT_BIT - bshift);
    }
  }

  s_mp_clamp(mp);
  return MP_OKAY;
} 









void     s_mp_rshd(mp_int *mp, mp_size p)
{
  mp_size  ix;
  mp_digit *src, *dst;

  if(p == 0)
    return;

  
  if(p >= USED(mp)) {
    s_mp_setz(DIGITS(mp), ALLOC(mp));
    USED(mp) = 1;
    SIGN(mp) = ZPOS;
    return;
  }

  
  dst = MP_DIGITS(mp);
  src = dst + p;
  for (ix = USED(mp) - p; ix > 0; ix--)
    *dst++ = *src++;

  MP_USED(mp) -= p;
  
  while (p-- > 0)
    *dst++ = 0;

#if 0
  
  s_mp_clamp(mp);
#endif

} 






void     s_mp_div_2(mp_int *mp)
{
  s_mp_div_2d(mp, 1);

} 





mp_err s_mp_mul_2(mp_int *mp)
{
  mp_digit *pd;
  int      ix, used;
  mp_digit kin = 0;

  
  used = MP_USED(mp);
  pd = MP_DIGITS(mp);
  for (ix = 0; ix < used; ix++) {
    mp_digit d = *pd;
    *pd++ = (d << 1) | kin;
    kin = (d >> (DIGIT_BIT - 1));
  }

  
  if (kin) {
    if (ix >= ALLOC(mp)) {
      mp_err res;
      if((res = s_mp_grow(mp, ALLOC(mp) + 1)) != MP_OKAY)
	return res;
    }

    DIGIT(mp, ix) = kin;
    USED(mp) += 1;
  }

  return MP_OKAY;

} 










void     s_mp_mod_2d(mp_int *mp, mp_digit d)
{
  mp_size  ndig = (d / DIGIT_BIT), nbit = (d % DIGIT_BIT);
  mp_size  ix;
  mp_digit dmask;

  if(ndig >= USED(mp))
    return;

  
  dmask = ((mp_digit)1 << nbit) - 1;
  DIGIT(mp, ndig) &= dmask;

  
  for(ix = ndig + 1; ix < USED(mp); ix++)
    DIGIT(mp, ix) = 0;

  s_mp_clamp(mp);

} 










void     s_mp_div_2d(mp_int *mp, mp_digit d)
{
  int       ix;
  mp_digit  save, next, mask;

  s_mp_rshd(mp, d / DIGIT_BIT);
  d %= DIGIT_BIT;
  if (d) {
    mask = ((mp_digit)1 << d) - 1;
    save = 0;
    for(ix = USED(mp) - 1; ix >= 0; ix--) {
      next = DIGIT(mp, ix) & mask;
      DIGIT(mp, ix) = (DIGIT(mp, ix) >> d) | (save << (DIGIT_BIT - d));
      save = next;
    }
  }
  s_mp_clamp(mp);

} 
















mp_err   s_mp_norm(mp_int *a, mp_int *b, mp_digit *pd)
{
  mp_digit  d;
  mp_digit  mask;
  mp_digit  b_msd;
  mp_err    res    = MP_OKAY;

  d = 0;
  mask  = DIGIT_MAX & ~(DIGIT_MAX >> 1);	
  b_msd = DIGIT(b, USED(b) - 1);
  while (!(b_msd & mask)) {
    b_msd <<= 1;
    ++d;
  }

  if (d) {
    MP_CHECKOK( s_mp_mul_2d(a, d) );
    MP_CHECKOK( s_mp_mul_2d(b, d) );
  }

  *pd = d;
CLEANUP:
  return res;

} 










mp_err   s_mp_add_d(mp_int *mp, mp_digit d)    
{
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
  mp_word   w, k = 0;
  mp_size   ix = 1;

  w = (mp_word)DIGIT(mp, 0) + d;
  DIGIT(mp, 0) = ACCUM(w);
  k = CARRYOUT(w);

  while(ix < USED(mp) && k) {
    w = (mp_word)DIGIT(mp, ix) + k;
    DIGIT(mp, ix) = ACCUM(w);
    k = CARRYOUT(w);
    ++ix;
  }

  if(k != 0) {
    mp_err  res;

    if((res = s_mp_pad(mp, USED(mp) + 1)) != MP_OKAY)
      return res;

    DIGIT(mp, ix) = (mp_digit)k;
  }

  return MP_OKAY;
#else
  mp_digit * pmp = MP_DIGITS(mp);
  mp_digit sum, mp_i, carry = 0;
  mp_err   res = MP_OKAY;
  int used = (int)MP_USED(mp);

  mp_i = *pmp;
  *pmp++ = sum = d + mp_i;
  carry = (sum < d);
  while (carry && --used > 0) {
    mp_i = *pmp;
    *pmp++ = sum = carry + mp_i;
    carry = !sum;
  }
  if (carry && !used) {
    
    used = MP_USED(mp);
    MP_CHECKOK( s_mp_pad(mp, used + 1) );
    MP_DIGIT(mp, used) = carry;
  }
CLEANUP:
  return res;
#endif
} 






mp_err   s_mp_sub_d(mp_int *mp, mp_digit d)    
{
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_SUB_WORD)
  mp_word   w, b = 0;
  mp_size   ix = 1;

  
  w = (RADIX + (mp_word)DIGIT(mp, 0)) - d;
  b = CARRYOUT(w) ? 0 : 1;
  DIGIT(mp, 0) = ACCUM(w);

  
  while(b && ix < USED(mp)) {
    w = (RADIX + (mp_word)DIGIT(mp, ix)) - b;
    b = CARRYOUT(w) ? 0 : 1;
    DIGIT(mp, ix) = ACCUM(w);
    ++ix;
  }

  
  s_mp_clamp(mp);

  
  if(b)
    return MP_RANGE;
  else
    return MP_OKAY;
#else
  mp_digit *pmp = MP_DIGITS(mp);
  mp_digit mp_i, diff, borrow;
  mp_size  used = MP_USED(mp);

  mp_i = *pmp;
  *pmp++ = diff = mp_i - d;
  borrow = (diff > mp_i);
  while (borrow && --used) {
    mp_i = *pmp;
    *pmp++ = diff = mp_i - borrow;
    borrow = (diff > mp_i);
  }
  s_mp_clamp(mp);
  return (borrow && !used) ? MP_RANGE : MP_OKAY;
#endif
} 






mp_err   s_mp_mul_d(mp_int *a, mp_digit d)
{
  mp_err  res;
  mp_size used;
  int     pow;

  if (!d) {
    mp_zero(a);
    return MP_OKAY;
  }
  if (d == 1)
    return MP_OKAY;
  if (0 <= (pow = s_mp_ispow2d(d))) {
    return s_mp_mul_2d(a, (mp_digit)pow);
  }

  used = MP_USED(a);
  MP_CHECKOK( s_mp_pad(a, used + 1) );

  s_mpv_mul_d(MP_DIGITS(a), used, d, MP_DIGITS(a));

  s_mp_clamp(a);

CLEANUP:
  return res;
  
} 












mp_err   s_mp_div_d(mp_int *mp, mp_digit d, mp_digit *r)
{
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_DIV_WORD)
  mp_word   w = 0, q;
#else
  mp_digit  w, q;
#endif
  int       ix;
  mp_err    res;
  mp_int    quot;
  mp_int    rem;

  if(d == 0)
    return MP_RANGE;
  if (d == 1) {
    if (r)
      *r = 0;
    return MP_OKAY;
  }
  
  if (MP_USED(mp) == 1) {
    mp_digit n   = MP_DIGIT(mp,0);
    mp_digit rem;

    q   = n / d;
    rem = n % d;
    MP_DIGIT(mp,0) = q;
    if (r)
      *r = rem;
    return MP_OKAY;
  }

  MP_DIGITS(&rem)  = 0;
  MP_DIGITS(&quot) = 0;
  
  MP_CHECKOK( mp_init_size(&quot, USED(mp)) );

#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_DIV_WORD)
  for(ix = USED(mp) - 1; ix >= 0; ix--) {
    w = (w << DIGIT_BIT) | DIGIT(mp, ix);

    if(w >= d) {
      q = w / d;
      w = w % d;
    } else {
      q = 0;
    }

    s_mp_lshd(&quot, 1);
    DIGIT(&quot, 0) = (mp_digit)q;
  }
#else
  {
    mp_digit p;
#if !defined(MP_ASSEMBLY_DIV_2DX1D)
    mp_digit norm;
#endif

    MP_CHECKOK( mp_init_copy(&rem, mp) );

#if !defined(MP_ASSEMBLY_DIV_2DX1D)
    MP_DIGIT(&quot, 0) = d;
    MP_CHECKOK( s_mp_norm(&rem, &quot, &norm) );
    if (norm)
      d <<= norm;
    MP_DIGIT(&quot, 0) = 0;
#endif

    p = 0;
    for (ix = USED(&rem) - 1; ix >= 0; ix--) {
      w = DIGIT(&rem, ix);

      if (p) {
        MP_CHECKOK( s_mpv_div_2dx1d(p, w, d, &q, &w) );
      } else if (w >= d) {
	q = w / d;
	w = w % d;
      } else {
	q = 0;
      }

      MP_CHECKOK( s_mp_lshd(&quot, 1) );
      DIGIT(&quot, 0) = q;
      p = w;
    }
#if !defined(MP_ASSEMBLY_DIV_2DX1D)
    if (norm)
      w >>= norm;
#endif
  }
#endif

  
  if(r)
    *r = (mp_digit)w;

  s_mp_clamp(&quot);
  mp_exch(&quot, mp);
CLEANUP:
  mp_clear(&quot);
  mp_clear(&rem);

  return res;
} 











mp_err   s_mp_add(mp_int *a, const mp_int *b)  
{
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
  mp_word   w = 0;
#else
  mp_digit  d, sum, carry = 0;
#endif
  mp_digit *pa, *pb;
  mp_size   ix;
  mp_size   used;
  mp_err    res;

  
  if((USED(b) > USED(a)) && (res = s_mp_pad(a, USED(b))) != MP_OKAY)
    return res;

  






  pa = MP_DIGITS(a);
  pb = MP_DIGITS(b);
  used = MP_USED(b);
  for(ix = 0; ix < used; ix++) {
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
    w = w + *pa + *pb++;
    *pa++ = ACCUM(w);
    w = CARRYOUT(w);
#else
    d = *pa;
    sum = d + *pb++;
    d = (sum < d);			
    *pa++ = sum += carry;
    carry = d + (sum < carry);		
#endif
  }

  


  used = MP_USED(a);
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
  while (w && ix < used) {
    w = w + *pa;
    *pa++ = ACCUM(w);
    w = CARRYOUT(w);
    ++ix;
  }
#else
  while (carry && ix < used) {
    sum = carry + *pa;
    *pa++ = sum;
    carry = !sum;
    ++ix;
  }
#endif

  



#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
  if (w) {
    if((res = s_mp_pad(a, used + 1)) != MP_OKAY)
      return res;

    DIGIT(a, ix) = (mp_digit)w;
  }
#else
  if (carry) {
    if((res = s_mp_pad(a, used + 1)) != MP_OKAY)
      return res;

    DIGIT(a, used) = carry;
  }
#endif

  return MP_OKAY;
} 



 
mp_err   s_mp_add_3arg(const mp_int *a, const mp_int *b, mp_int *c)  
{
  mp_digit *pa, *pb, *pc;
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
  mp_word   w = 0;
#else
  mp_digit  sum, carry = 0, d;
#endif
  mp_size   ix;
  mp_size   used;
  mp_err    res;

  MP_SIGN(c) = MP_SIGN(a);
  if (MP_USED(a) < MP_USED(b)) {
    const mp_int *xch = a;
    a = b;
    b = xch;
  }

  
  if (MP_OKAY != (res = s_mp_pad(c, MP_USED(a))))
    return res;

  






  pa = MP_DIGITS(a);
  pb = MP_DIGITS(b);
  pc = MP_DIGITS(c);
  used = MP_USED(b);
  for (ix = 0; ix < used; ix++) {
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
    w = w + *pa++ + *pb++;
    *pc++ = ACCUM(w);
    w = CARRYOUT(w);
#else
    d = *pa++;
    sum = d + *pb++;
    d = (sum < d);			
    *pc++ = sum += carry;
    carry = d + (sum < carry);		
#endif
  }

  


  for (used = MP_USED(a); ix < used; ++ix) {
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
    w = w + *pa++;
    *pc++ = ACCUM(w);
    w = CARRYOUT(w);
#else
    *pc++ = sum = carry + *pa++;
    carry = (sum < carry);
#endif
  }

  



#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
  if (w) {
    if((res = s_mp_pad(c, used + 1)) != MP_OKAY)
      return res;

    DIGIT(c, used) = (mp_digit)w;
    ++used;
  }
#else
  if (carry) {
    if((res = s_mp_pad(c, used + 1)) != MP_OKAY)
      return res;

    DIGIT(c, used) = carry;
    ++used;
  }
#endif
  MP_USED(c) = used;
  return MP_OKAY;
}



mp_err   s_mp_add_offset(mp_int *a, mp_int *b, mp_size offset)   
{
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
  mp_word   w, k = 0;
#else
  mp_digit  d, sum, carry = 0;
#endif
  mp_size   ib;
  mp_size   ia;
  mp_size   lim;
  mp_err    res;

  
  lim = MP_USED(b) + offset;
  if((lim > USED(a)) && (res = s_mp_pad(a, lim)) != MP_OKAY)
    return res;

  






  lim = USED(b);
  for(ib = 0, ia = offset; ib < lim; ib++, ia++) {
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
    w = (mp_word)DIGIT(a, ia) + DIGIT(b, ib) + k;
    DIGIT(a, ia) = ACCUM(w);
    k = CARRYOUT(w);
#else
    d = MP_DIGIT(a, ia);
    sum = d + MP_DIGIT(b, ib);
    d = (sum < d);
    MP_DIGIT(a,ia) = sum += carry;
    carry = d + (sum < carry);
#endif
  }

  


#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
  for (lim = MP_USED(a); k && (ia < lim); ++ia) {
    w = (mp_word)DIGIT(a, ia) + k;
    DIGIT(a, ia) = ACCUM(w);
    k = CARRYOUT(w);
  }
#else
  for (lim = MP_USED(a); carry && (ia < lim); ++ia) {
    d = MP_DIGIT(a, ia);
    MP_DIGIT(a,ia) = sum = d + carry;
    carry = (sum < d);
  }
#endif

  



#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_ADD_WORD)
  if(k) {
    if((res = s_mp_pad(a, USED(a) + 1)) != MP_OKAY)
      return res;

    DIGIT(a, ia) = (mp_digit)k;
  }
#else
  if (carry) {
    if((res = s_mp_pad(a, lim + 1)) != MP_OKAY)
      return res;

    DIGIT(a, lim) = carry;
  }
#endif
  s_mp_clamp(a);

  return MP_OKAY;

} 






mp_err   s_mp_sub(mp_int *a, const mp_int *b)  
{
  mp_digit *pa, *pb, *limit;
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_SUB_WORD)
  mp_sword  w = 0;
#else
  mp_digit  d, diff, borrow = 0;
#endif

  





  pa = MP_DIGITS(a);
  pb = MP_DIGITS(b);
  limit = pb + MP_USED(b);
  while (pb < limit) {
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_SUB_WORD)
    w = w + *pa - *pb++;
    *pa++ = ACCUM(w);
    w >>= MP_DIGIT_BIT;
#else
    d = *pa;
    diff = d - *pb++;
    d = (diff > d);				
    if (borrow && --diff == MP_DIGIT_MAX)
      ++d;
    *pa++ = diff;
    borrow = d;	
#endif
  }
  limit = MP_DIGITS(a) + MP_USED(a);
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_SUB_WORD)
  while (w && pa < limit) {
    w = w + *pa;
    *pa++ = ACCUM(w);
    w >>= MP_DIGIT_BIT;
  }
#else
  while (borrow && pa < limit) {
    d = *pa;
    *pa++ = diff = d - borrow;
    borrow = (diff > d);
  }
#endif

  
  s_mp_clamp(a);

  




#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_SUB_WORD)
  return w ? MP_RANGE : MP_OKAY;
#else
  return borrow ? MP_RANGE : MP_OKAY;
#endif
} 



 
mp_err   s_mp_sub_3arg(const mp_int *a, const mp_int *b, mp_int *c)  
{
  mp_digit *pa, *pb, *pc;
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_SUB_WORD)
  mp_sword  w = 0;
#else
  mp_digit  d, diff, borrow = 0;
#endif
  int       ix, limit;
  mp_err    res;

  MP_SIGN(c) = MP_SIGN(a);

  
  if (MP_OKAY != (res = s_mp_pad(c, MP_USED(a))))
    return res;

  





  pa = MP_DIGITS(a);
  pb = MP_DIGITS(b);
  pc = MP_DIGITS(c);
  limit = MP_USED(b);
  for (ix = 0; ix < limit; ++ix) {
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_SUB_WORD)
    w = w + *pa++ - *pb++;
    *pc++ = ACCUM(w);
    w >>= MP_DIGIT_BIT;
#else
    d = *pa++;
    diff = d - *pb++;
    d = (diff > d);
    if (borrow && --diff == MP_DIGIT_MAX)
      ++d;
    *pc++ = diff;
    borrow = d;
#endif
  }
  for (limit = MP_USED(a); ix < limit; ++ix) {
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_SUB_WORD)
    w = w + *pa++;
    *pc++ = ACCUM(w);
    w >>= MP_DIGIT_BIT;
#else
    d = *pa++;
    *pc++ = diff = d - borrow;
    borrow = (diff > d);
#endif
  }

  
  MP_USED(c) = ix;
  s_mp_clamp(c);

  




#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_SUB_WORD)
  return w ? MP_RANGE : MP_OKAY;
#else
  return borrow ? MP_RANGE : MP_OKAY;
#endif
}



mp_err   s_mp_mul(mp_int *a, const mp_int *b)
{
  return mp_mul(a, b, a);
} 



#if defined(MP_USE_UINT_DIGIT) && defined(MP_USE_LONG_LONG_MULTIPLY)

#define MP_MUL_DxD(a, b, Phi, Plo) \
  { unsigned long long product = (unsigned long long)a * b; \
    Plo = (mp_digit)product; \
    Phi = (mp_digit)(product >> MP_DIGIT_BIT); }
#elif defined(OSF1)
#define MP_MUL_DxD(a, b, Phi, Plo) \
  { Plo = asm ("mulq %a0, %a1, %v0", a, b);\
    Phi = asm ("umulh %a0, %a1, %v0", a, b); }
#else
#define MP_MUL_DxD(a, b, Phi, Plo) \
  { mp_digit a0b1, a1b0; \
    Plo = (a & MP_HALF_DIGIT_MAX) * (b & MP_HALF_DIGIT_MAX); \
    Phi = (a >> MP_HALF_DIGIT_BIT) * (b >> MP_HALF_DIGIT_BIT); \
    a0b1 = (a & MP_HALF_DIGIT_MAX) * (b >> MP_HALF_DIGIT_BIT); \
    a1b0 = (a >> MP_HALF_DIGIT_BIT) * (b & MP_HALF_DIGIT_MAX); \
    a1b0 += a0b1; \
    Phi += a1b0 >> MP_HALF_DIGIT_BIT; \
    if (a1b0 < a0b1)  \
      Phi += MP_HALF_RADIX; \
    a1b0 <<= MP_HALF_DIGIT_BIT; \
    Plo += a1b0; \
    if (Plo < a1b0) \
      ++Phi; \
  }
#endif

#if !defined(MP_ASSEMBLY_MULTIPLY)

void s_mpv_mul_d(const mp_digit *a, mp_size a_len, mp_digit b, mp_digit *c)
{
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_MUL_WORD)
  mp_digit   d = 0;

  
  while (a_len--) {
    mp_word w = ((mp_word)b * *a++) + d;
    *c++ = ACCUM(w);
    d = CARRYOUT(w);
  }
  *c = d;
#else
  mp_digit carry = 0;
  while (a_len--) {
    mp_digit a_i = *a++;
    mp_digit a0b0, a1b1;

    MP_MUL_DxD(a_i, b, a1b1, a0b0);

    a0b0 += carry;
    if (a0b0 < carry)
      ++a1b1;
    *c++ = a0b0;
    carry = a1b1;
  }
  *c = carry;
#endif
}


void s_mpv_mul_d_add(const mp_digit *a, mp_size a_len, mp_digit b, 
			      mp_digit *c)
{
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_MUL_WORD)
  mp_digit   d = 0;

  
  while (a_len--) {
    mp_word w = ((mp_word)b * *a++) + *c + d;
    *c++ = ACCUM(w);
    d = CARRYOUT(w);
  }
  *c = d;
#else
  mp_digit carry = 0;
  while (a_len--) {
    mp_digit a_i = *a++;
    mp_digit a0b0, a1b1;

    MP_MUL_DxD(a_i, b, a1b1, a0b0);

    a0b0 += carry;
    if (a0b0 < carry)
      ++a1b1;
    a0b0 += a_i = *c;
    if (a0b0 < a_i)
      ++a1b1;
    *c++ = a0b0;
    carry = a1b1;
  }
  *c = carry;
#endif
}



void s_mpv_mul_d_add_prop(const mp_digit *a, mp_size a_len, mp_digit b, mp_digit *c)
{
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_MUL_WORD)
  mp_digit   d = 0;

  
  while (a_len--) {
    mp_word w = ((mp_word)b * *a++) + *c + d;
    *c++ = ACCUM(w);
    d = CARRYOUT(w);
  }

  while (d) {
    mp_word w = (mp_word)*c + d;
    *c++ = ACCUM(w);
    d = CARRYOUT(w);
  }
#else
  mp_digit carry = 0;
  while (a_len--) {
    mp_digit a_i = *a++;
    mp_digit a0b0, a1b1;

    MP_MUL_DxD(a_i, b, a1b1, a0b0);

    a0b0 += carry;
    if (a0b0 < carry)
      ++a1b1;

    a0b0 += a_i = *c;
    if (a0b0 < a_i)
      ++a1b1;

    *c++ = a0b0;
    carry = a1b1;
  }
  while (carry) {
    mp_digit c_i = *c;
    carry += c_i;
    *c++ = carry;
    carry = carry < c_i;
  }
#endif
}
#endif

#if defined(MP_USE_UINT_DIGIT) && defined(MP_USE_LONG_LONG_MULTIPLY)

#define MP_SQR_D(a, Phi, Plo) \
  { unsigned long long square = (unsigned long long)a * a; \
    Plo = (mp_digit)square; \
    Phi = (mp_digit)(square >> MP_DIGIT_BIT); }
#elif defined(OSF1)
#define MP_SQR_D(a, Phi, Plo) \
  { Plo = asm ("mulq  %a0, %a0, %v0", a);\
    Phi = asm ("umulh %a0, %a0, %v0", a); }
#else
#define MP_SQR_D(a, Phi, Plo) \
  { mp_digit Pmid; \
    Plo  = (a  & MP_HALF_DIGIT_MAX) * (a  & MP_HALF_DIGIT_MAX); \
    Phi  = (a >> MP_HALF_DIGIT_BIT) * (a >> MP_HALF_DIGIT_BIT); \
    Pmid = (a  & MP_HALF_DIGIT_MAX) * (a >> MP_HALF_DIGIT_BIT); \
    Phi += Pmid >> (MP_HALF_DIGIT_BIT - 1);  \
    Pmid <<= (MP_HALF_DIGIT_BIT + 1);  \
    Plo += Pmid;  \
    if (Plo < Pmid)  \
      ++Phi;  \
  }
#endif

#if !defined(MP_ASSEMBLY_SQUARE)

void s_mpv_sqr_add_prop(const mp_digit *pa, mp_size a_len, mp_digit *ps)
{
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_MUL_WORD)
  mp_word  w;
  mp_digit d;
  mp_size  ix;

  w  = 0;
#define ADD_SQUARE(n) \
    d = pa[n]; \
    w += (d * (mp_word)d) + ps[2*n]; \
    ps[2*n] = ACCUM(w); \
    w = (w >> DIGIT_BIT) + ps[2*n+1]; \
    ps[2*n+1] = ACCUM(w); \
    w = (w >> DIGIT_BIT)

  for (ix = a_len; ix >= 4; ix -= 4) {
    ADD_SQUARE(0);
    ADD_SQUARE(1);
    ADD_SQUARE(2);
    ADD_SQUARE(3);
    pa += 4;
    ps += 8;
  }
  if (ix) {
    ps += 2*ix;
    pa += ix;
    switch (ix) {
    case 3: ADD_SQUARE(-3); 
    case 2: ADD_SQUARE(-2); 
    case 1: ADD_SQUARE(-1); 
    case 0: break;
    }
  }
  while (w) {
    w += *ps;
    *ps++ = ACCUM(w);
    w = (w >> DIGIT_BIT);
  }
#else
  mp_digit carry = 0;
  while (a_len--) {
    mp_digit a_i = *pa++;
    mp_digit a0a0, a1a1;

    MP_SQR_D(a_i, a1a1, a0a0);

    
    a0a0 += carry;
    if (a0a0 < carry)
      ++a1a1;

    
    a0a0 += a_i = *ps;
    if (a0a0 < a_i)
      ++a1a1;
    *ps++ = a0a0;
    a1a1 += a_i = *ps;
    carry = (a1a1 < a_i);
    *ps++ = a1a1;
  }
  while (carry) {
    mp_digit s_i = *ps;
    carry += s_i;
    *ps++ = carry;
    carry = carry < s_i;
  }
#endif
}
#endif

#if (defined(MP_NO_MP_WORD) || defined(MP_NO_DIV_WORD)) \
&& !defined(MP_ASSEMBLY_DIV_2DX1D)




mp_err s_mpv_div_2dx1d(mp_digit Nhi, mp_digit Nlo, mp_digit divisor, 
		       mp_digit *qp, mp_digit *rp)
{
    mp_digit d1, d0, q1, q0;
    mp_digit r1, r0, m;

    d1 = divisor >> MP_HALF_DIGIT_BIT;
    d0 = divisor & MP_HALF_DIGIT_MAX;
    r1 = Nhi % d1;
    q1 = Nhi / d1;
    m = q1 * d0;
    r1 = (r1 << MP_HALF_DIGIT_BIT) | (Nlo >> MP_HALF_DIGIT_BIT);
    if (r1 < m) {
        q1--, r1 += divisor;
        if (r1 >= divisor && r1 < m) {
	    q1--, r1 += divisor;
	}
    }
    r1 -= m;
    r0 = r1 % d1;
    q0 = r1 / d1;
    m = q0 * d0;
    r0 = (r0 << MP_HALF_DIGIT_BIT) | (Nlo & MP_HALF_DIGIT_MAX);
    if (r0 < m) {
        q0--, r0 += divisor;
        if (r0 >= divisor && r0 < m) {
	    q0--, r0 += divisor;
	}
    }
    if (qp)
	*qp = (q1 << MP_HALF_DIGIT_BIT) | q0;
    if (rp)
	*rp = r0 - m;
    return MP_OKAY;
}
#endif

#if MP_SQUARE


mp_err   s_mp_sqr(mp_int *a)
{
  mp_err   res;
  mp_int   tmp;

  if((res = mp_init_size(&tmp, 2 * USED(a))) != MP_OKAY)
    return res;
  res = mp_sqr(a, &tmp);
  if (res == MP_OKAY) {
    s_mp_exch(&tmp, a);
  }
  mp_clear(&tmp);
  return res;
}


#endif









mp_err   s_mp_div(mp_int *rem, 	
                  mp_int *div, 	
		  mp_int *quot)	
{
  mp_int   part, t;
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_DIV_WORD)
  mp_word  q_msd;
#else
  mp_digit q_msd;
#endif
  mp_err   res;
  mp_digit d;
  mp_digit div_msd;
  int      ix;

  if(mp_cmp_z(div) == 0)
    return MP_RANGE;

  
  if((ix = s_mp_ispow2(div)) >= 0) {
    MP_CHECKOK( mp_copy(rem, quot) );
    s_mp_div_2d(quot, (mp_digit)ix);
    s_mp_mod_2d(rem,  (mp_digit)ix);

    return MP_OKAY;
  }

  DIGITS(&t) = 0;
  MP_SIGN(rem) = ZPOS;
  MP_SIGN(div) = ZPOS;

  
  MP_CHECKOK( mp_init_size(&t, MP_ALLOC(rem)));

  
  MP_CHECKOK( s_mp_norm(rem, div, &d) );

  part = *rem;

  
  MP_USED(quot) = MP_ALLOC(quot);

  
  
  while (MP_USED(rem) > MP_USED(div) || s_mp_cmp(rem, div) >= 0) {
    int i;
    int unusedRem;

    unusedRem = MP_USED(rem) - MP_USED(div);
    MP_DIGITS(&part) = MP_DIGITS(rem) + unusedRem;
    MP_ALLOC(&part)  = MP_ALLOC(rem)  - unusedRem;
    MP_USED(&part)   = MP_USED(div);
    if (s_mp_cmp(&part, div) < 0) {
      -- unusedRem;
#if MP_ARGCHK == 2
      assert(unusedRem >= 0);
#endif
      -- MP_DIGITS(&part);
      ++ MP_USED(&part);
      ++ MP_ALLOC(&part);
    }

    
    q_msd = MP_DIGIT(&part, MP_USED(&part) - 1);
    div_msd = MP_DIGIT(div, MP_USED(div) - 1);
    if (q_msd >= div_msd) {
      q_msd = 1;
    } else if (MP_USED(&part) > 1) {
#if !defined(MP_NO_MP_WORD) && !defined(MP_NO_DIV_WORD)
      q_msd = (q_msd << MP_DIGIT_BIT) | MP_DIGIT(&part, MP_USED(&part) - 2);
      q_msd /= div_msd;
      if (q_msd == RADIX)
        --q_msd;
#else
      mp_digit r;
      MP_CHECKOK( s_mpv_div_2dx1d(q_msd, MP_DIGIT(&part, MP_USED(&part) - 2), 
				  div_msd, &q_msd, &r) );
#endif
    } else {
      q_msd = 0;
    }
#if MP_ARGCHK == 2
    assert(q_msd > 0); 
#endif
    if (q_msd <= 0)
      break;

    
    mp_copy(div, &t);
    MP_CHECKOK( s_mp_mul_d(&t, (mp_digit)q_msd) );

    






    for (i = 4; s_mp_cmp(&t, &part) > 0 && i > 0; --i) {
      --q_msd;
      s_mp_sub(&t, div);	
    }
    if (i < 0) {
      res = MP_RANGE;
      goto CLEANUP;
    }

    
    MP_CHECKOK( s_mp_sub(&part, &t) );	
    s_mp_clamp(rem);

    




    MP_DIGIT(quot, unusedRem) = (mp_digit)q_msd;
  }

  
  if (d) {
    s_mp_div_2d(rem, d);
  }

  s_mp_clamp(quot);

CLEANUP:
  mp_clear(&t);

  return res;

} 






mp_err   s_mp_2expt(mp_int *a, mp_digit k)
{
  mp_err    res;
  mp_size   dig, bit;

  dig = k / DIGIT_BIT;
  bit = k % DIGIT_BIT;

  mp_zero(a);
  if((res = s_mp_pad(a, dig + 1)) != MP_OKAY)
    return res;
  
  DIGIT(a, dig) |= ((mp_digit)1 << bit);

  return MP_OKAY;

} 


















mp_err   s_mp_reduce(mp_int *x, const mp_int *m, const mp_int *mu)
{
  mp_int   q;
  mp_err   res;

  if((res = mp_init_copy(&q, x)) != MP_OKAY)
    return res;

  s_mp_rshd(&q, USED(m) - 1);  
  s_mp_mul(&q, mu);            
  s_mp_rshd(&q, USED(m) + 1);  

  
  s_mp_mod_2d(x, DIGIT_BIT * (USED(m) + 1));

  
  s_mp_mul(&q, m);
  s_mp_mod_2d(&q, DIGIT_BIT * (USED(m) + 1));

  
  if((res = mp_sub(x, &q, x)) != MP_OKAY)
    goto CLEANUP;

  
  if(mp_cmp_z(x) < 0) {
    mp_set(&q, 1);
    if((res = s_mp_lshd(&q, USED(m) + 1)) != MP_OKAY)
      goto CLEANUP;
    if((res = mp_add(x, &q, x)) != MP_OKAY)
      goto CLEANUP;
  }

  
  while(mp_cmp(x, m) >= 0) {
    if((res = s_mp_sub(x, m)) != MP_OKAY)
      break;
  }

 CLEANUP:
  mp_clear(&q);

  return res;

} 










int      s_mp_cmp(const mp_int *a, const mp_int *b)
{
  mp_size used_a = MP_USED(a);
  {
    mp_size used_b = MP_USED(b);

    if (used_a > used_b)
      goto IS_GT;
    if (used_a < used_b)
      goto IS_LT;
  }
  {
    mp_digit *pa, *pb;
    mp_digit da = 0, db = 0;

#define CMP_AB(n) if ((da = pa[n]) != (db = pb[n])) goto done

    pa = MP_DIGITS(a) + used_a;
    pb = MP_DIGITS(b) + used_a;
    while (used_a >= 4) {
      pa     -= 4;
      pb     -= 4;
      used_a -= 4;
      CMP_AB(3);
      CMP_AB(2);
      CMP_AB(1);
      CMP_AB(0);
    }
    while (used_a-- > 0 && ((da = *--pa) == (db = *--pb))) 
      ;
done:
    if (da > db)
      goto IS_GT;
    if (da < db) 
      goto IS_LT;
  }
  return MP_EQ;
IS_LT:
  return MP_LT;
IS_GT:
  return MP_GT;
} 






int      s_mp_cmp_d(const mp_int *a, mp_digit d)
{
  if(USED(a) > 1)
    return MP_GT;

  if(DIGIT(a, 0) < d)
    return MP_LT;
  else if(DIGIT(a, 0) > d)
    return MP_GT;
  else
    return MP_EQ;

} 









int      s_mp_ispow2(const mp_int *v)
{
  mp_digit d;
  int      extra = 0, ix;

  ix = MP_USED(v) - 1;
  d = MP_DIGIT(v, ix); 

  extra = s_mp_ispow2d(d);
  if (extra < 0 || ix == 0)
    return extra;

  while (--ix >= 0) {
    if (DIGIT(v, ix) != 0)
      return -1; 
    extra += MP_DIGIT_BIT;
  }

  return extra;

} 





int      s_mp_ispow2d(mp_digit d)
{
  if ((d != 0) && ((d & (d-1)) == 0)) { 
    int pow = 0;
#if defined (MP_USE_UINT_DIGIT)
    if (d & 0xffff0000U)
      pow += 16;
    if (d & 0xff00ff00U)
      pow += 8;
    if (d & 0xf0f0f0f0U)
      pow += 4;
    if (d & 0xccccccccU)
      pow += 2;
    if (d & 0xaaaaaaaaU)
      pow += 1;
#elif defined(MP_USE_LONG_LONG_DIGIT)
    if (d & 0xffffffff00000000ULL)
      pow += 32;
    if (d & 0xffff0000ffff0000ULL)
      pow += 16;
    if (d & 0xff00ff00ff00ff00ULL)
      pow += 8;
    if (d & 0xf0f0f0f0f0f0f0f0ULL)
      pow += 4;
    if (d & 0xccccccccccccccccULL)
      pow += 2;
    if (d & 0xaaaaaaaaaaaaaaaaULL)
      pow += 1;
#elif defined(MP_USE_LONG_DIGIT)
    if (d & 0xffffffff00000000UL)
      pow += 32;
    if (d & 0xffff0000ffff0000UL)
      pow += 16;
    if (d & 0xff00ff00ff00ff00UL)
      pow += 8;
    if (d & 0xf0f0f0f0f0f0f0f0UL)
      pow += 4;
    if (d & 0xccccccccccccccccUL)
      pow += 2;
    if (d & 0xaaaaaaaaaaaaaaaaUL)
      pow += 1;
#else
#error "unknown type for mp_digit"
#endif
    return pow;
  }
  return -1;

} 

















int      s_mp_tovalue(char ch, int r)
{
  int    val, xch;
  
  if(r > 36)
    xch = ch;
  else
    xch = toupper(ch);

  if(isdigit(xch))
    val = xch - '0';
  else if(isupper(xch))
    val = xch - 'A' + 10;
  else if(islower(xch))
    val = xch - 'a' + 36;
  else if(xch == '+')
    val = 62;
  else if(xch == '/')
    val = 63;
  else 
    return -1;

  if(val < 0 || val >= r)
    return -1;

  return val;

} 













  
char     s_mp_todigit(mp_digit val, int r, int low)
{
  char   ch;

  if(val >= r)
    return 0;

  ch = s_dmap_1[val];

  if(r <= 36 && low)
    ch = tolower(ch);

  return ch;

} 










int      s_mp_outlen(int bits, int r)
{
  return (int)((double)bits * LOG_V_2(r) + 1.5) + 1;

} 











mp_err  
mp_read_unsigned_octets(mp_int *mp, const unsigned char *str, mp_size len)
{
  int            count;
  mp_err         res;
  mp_digit       d;

  ARGCHK(mp != NULL && str != NULL && len > 0, MP_BADARG);

  mp_zero(mp);

  count = len % sizeof(mp_digit);
  if (count) {
    for (d = 0; count-- > 0; --len) {
      d = (d << 8) | *str++;
    }
    MP_DIGIT(mp, 0) = d;
  }

  
  for(; len > 0; len -= sizeof(mp_digit)) {
    for (d = 0, count = sizeof(mp_digit); count > 0; --count) {
      d = (d << 8) | *str++;
    }
    if (MP_EQ == mp_cmp_z(mp)) {
      if (!d)
	continue;
    } else {
      if((res = s_mp_lshd(mp, 1)) != MP_OKAY)
	return res;
    }
    MP_DIGIT(mp, 0) = d;
  }
  return MP_OKAY;
} 



int    
mp_unsigned_octet_size(const mp_int *mp)
{
  int  bytes;
  int  ix;
  mp_digit  d = 0;

  ARGCHK(mp != NULL, MP_BADARG);
  ARGCHK(MP_ZPOS == SIGN(mp), MP_BADARG);

  bytes = (USED(mp) * sizeof(mp_digit));

  
  
  for(ix = USED(mp) - 1; ix >= 0; ix--) {
    d = DIGIT(mp, ix);
    if (d) 
	break;
    bytes -= sizeof(d);
  }
  if (!bytes)
    return 1;

  
  for(ix = sizeof(mp_digit) - 1; ix >= 0; ix--) {
    unsigned char x = (unsigned char)(d >> (ix * CHAR_BIT));
    if (x) 
	break;
    --bytes;
  }
  return bytes;
} 




mp_err 
mp_to_unsigned_octets(const mp_int *mp, unsigned char *str, mp_size maxlen)
{
  int  ix, pos = 0;
  int  bytes;

  ARGCHK(mp != NULL && str != NULL && !SIGN(mp), MP_BADARG);

  bytes = mp_unsigned_octet_size(mp);
  ARGCHK(bytes <= maxlen, MP_BADARG);

  
  for(ix = USED(mp) - 1; ix >= 0; ix--) {
    mp_digit  d = DIGIT(mp, ix);
    int       jx;

    
    for(jx = sizeof(mp_digit) - 1; jx >= 0; jx--) {
      unsigned char x = (unsigned char)(d >> (jx * CHAR_BIT));
      if (!pos && !x)	
	continue;
      str[pos++] = x;
    }
  }
  if (!pos)
    str[pos++] = 0;
  return pos;
} 




mp_err 
mp_to_signed_octets(const mp_int *mp, unsigned char *str, mp_size maxlen)
{
  int  ix, pos = 0;
  int  bytes;

  ARGCHK(mp != NULL && str != NULL && !SIGN(mp), MP_BADARG);

  bytes = mp_unsigned_octet_size(mp);
  ARGCHK(bytes <= maxlen, MP_BADARG);

  
  for(ix = USED(mp) - 1; ix >= 0; ix--) {
    mp_digit  d = DIGIT(mp, ix);
    int       jx;

    
    for(jx = sizeof(mp_digit) - 1; jx >= 0; jx--) {
      unsigned char x = (unsigned char)(d >> (jx * CHAR_BIT));
      if (!pos) {
	if (!x)		
	  continue;
	if (x & 0x80) { 
	  ARGCHK(bytes + 1 <= maxlen, MP_BADARG);
	  if (bytes + 1 > maxlen)
	    return MP_BADARG;
	  str[pos++] = 0;
	}
      }
      str[pos++] = x;
    }
  }
  if (!pos)
    str[pos++] = 0;
  return pos;
} 




mp_err 
mp_to_fixlen_octets(const mp_int *mp, unsigned char *str, mp_size length)
{
  int  ix, pos = 0;
  int  bytes;

  ARGCHK(mp != NULL && str != NULL && !SIGN(mp), MP_BADARG);

  bytes = mp_unsigned_octet_size(mp);
  ARGCHK(bytes <= length, MP_BADARG);

  
  for (;length > bytes; --length) {
	*str++ = 0;
  }

  
  for(ix = USED(mp) - 1; ix >= 0; ix--) {
    mp_digit  d = DIGIT(mp, ix);
    int       jx;

    
    for(jx = sizeof(mp_digit) - 1; jx >= 0; jx--) {
      unsigned char x = (unsigned char)(d >> (jx * CHAR_BIT));
      if (!pos && !x)	
	continue;
      str[pos++] = x;
    }
  }
  if (!pos)
    str[pos++] = 0;
  return MP_OKAY;
} 





