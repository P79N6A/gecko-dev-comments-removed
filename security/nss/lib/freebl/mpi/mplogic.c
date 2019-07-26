









#include "mpi-priv.h"
#include "mplogic.h"



static unsigned char bitc[] = {
   0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
   4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};













mp_err mpl_not(mp_int *a, mp_int *b)
{
  mp_err   res;
  unsigned int      ix;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  
  for(ix = 0; ix < USED(b); ix++) 
    DIGIT(b, ix) = ~DIGIT(b, ix);

  s_mp_clamp(b);

  return MP_OKAY;

} 





mp_err mpl_and(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int  *which, *other;
  mp_err   res;
  unsigned int      ix;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(USED(a) <= USED(b)) {
    which = a;
    other = b;
  } else {
    which = b;
    other = a;
  }

  if((res = mp_copy(which, c)) != MP_OKAY)
    return res;

  for(ix = 0; ix < USED(which); ix++)
    DIGIT(c, ix) &= DIGIT(other, ix);

  s_mp_clamp(c);

  return MP_OKAY;

} 





mp_err mpl_or(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int  *which, *other;
  mp_err   res;
  unsigned int      ix;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(USED(a) >= USED(b)) {
    which = a;
    other = b;
  } else {
    which = b;
    other = a;
  }

  if((res = mp_copy(which, c)) != MP_OKAY)
    return res;

  for(ix = 0; ix < USED(which); ix++)
    DIGIT(c, ix) |= DIGIT(other, ix);

  return MP_OKAY;

} 





mp_err mpl_xor(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int  *which, *other;
  mp_err   res;
  unsigned int      ix;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(USED(a) >= USED(b)) {
    which = a;
    other = b;
  } else {
    which = b;
    other = a;
  }

  if((res = mp_copy(which, c)) != MP_OKAY)
    return res;

  for(ix = 0; ix < USED(which); ix++)
    DIGIT(c, ix) ^= DIGIT(other, ix);

  s_mp_clamp(c);

  return MP_OKAY;

} 











mp_err mpl_rsh(const mp_int *a, mp_int *b, mp_digit d)
{
  mp_err   res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  s_mp_div_2d(b, d);

  return MP_OKAY;

} 





mp_err mpl_lsh(const mp_int *a, mp_int *b, mp_digit d)
{
  mp_err   res;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  return s_mp_mul_2d(b, d);

} 

















mp_err mpl_num_set(mp_int *a, int *num)
{
  unsigned int   ix;
  int            db, nset = 0;
  mp_digit       cur;
  unsigned char  reg;

  ARGCHK(a != NULL, MP_BADARG);

  for(ix = 0; ix < USED(a); ix++) {
    cur = DIGIT(a, ix);
    
    for(db = 0; db < sizeof(mp_digit); db++) {
      reg = (unsigned char)(cur >> (CHAR_BIT * db));

      nset += bitc[reg];
    }
  }

  if(num)
    *num = nset;

  return MP_OKAY;

} 





mp_err mpl_num_clear(mp_int *a, int *num)
{
  unsigned int   ix;
  int            db, nset = 0;
  mp_digit       cur;
  unsigned char  reg;

  ARGCHK(a != NULL, MP_BADARG);

  for(ix = 0; ix < USED(a); ix++) {
    cur = DIGIT(a, ix);
    
    for(db = 0; db < sizeof(mp_digit); db++) {
      reg = (unsigned char)(cur >> (CHAR_BIT * db));

      nset += bitc[UCHAR_MAX - reg];
    }
  }

  if(num)
    *num = nset;

  return MP_OKAY;


} 














mp_err mpl_parity(mp_int *a)
{
  unsigned int ix;
  int      par = 0;
  mp_digit cur;

  ARGCHK(a != NULL, MP_BADARG);

  for(ix = 0; ix < USED(a); ix++) {
    int   shft = (sizeof(mp_digit) * CHAR_BIT) / 2;

    cur = DIGIT(a, ix);

    
    while(shft != 0) {
      cur ^= (cur >> shft);
      shft >>= 1;
    }
    cur &= 1;

    
    par ^= cur;
  }

  if(par)
    return MP_ODD;
  else
    return MP_EVEN;

} 









mp_err mpl_set_bit(mp_int *a, mp_size bitNum, mp_size value)
{
  mp_size      ix;
  mp_err       rv;
  mp_digit     mask;

  ARGCHK(a != NULL, MP_BADARG);

  ix = bitNum / MP_DIGIT_BIT;
  if (ix + 1 > MP_USED(a)) {
    rv = s_mp_pad(a, ix + 1);
    if (rv != MP_OKAY)
      return rv;
  }

  bitNum = bitNum % MP_DIGIT_BIT;
  mask = (mp_digit)1 << bitNum;
  if (value)
    MP_DIGIT(a,ix) |= mask;
  else
    MP_DIGIT(a,ix) &= ~mask;
  s_mp_clamp(a);
  return MP_OKAY;
}






mp_err mpl_get_bit(const mp_int *a, mp_size bitNum)
{
  mp_size      bit, ix;
  mp_err       rv;

  ARGCHK(a != NULL, MP_BADARG);

  ix = bitNum / MP_DIGIT_BIT;
  ARGCHK(ix <= MP_USED(a) - 1, MP_RANGE);

  bit   = bitNum % MP_DIGIT_BIT;
  rv = (mp_err)(MP_DIGIT(a, ix) >> bit) & 1;
  return rv;
}











mp_err mpl_get_bits(const mp_int *a, mp_size lsbNum, mp_size numBits) 
{
  mp_size    rshift = (lsbNum % MP_DIGIT_BIT);
  mp_size    lsWndx = (lsbNum / MP_DIGIT_BIT);
  mp_digit * digit  = MP_DIGITS(a) + lsWndx;
  mp_digit   mask   = ((1 << numBits) - 1);

  ARGCHK(numBits < CHAR_BIT * sizeof mask, MP_BADARG);
  ARGCHK(MP_HOWMANY(lsbNum, MP_DIGIT_BIT) <= MP_USED(a), MP_RANGE);

  if ((numBits + lsbNum % MP_DIGIT_BIT <= MP_DIGIT_BIT) ||
      (lsWndx + 1 >= MP_USED(a))) {
    mask &= (digit[0] >> rshift);
  } else {
    mask &= ((digit[0] >> rshift) | (digit[1] << (MP_DIGIT_BIT - rshift)));
  }
  return (mp_err)mask;
}






mp_err mpl_significant_bits(const mp_int *a)
{
  mp_err bits 	= 0;
  int    ix;

  ARGCHK(a != NULL, MP_BADARG);

  ix = MP_USED(a);
  for (ix = MP_USED(a); ix > 0; ) {
    mp_digit d;
    d = MP_DIGIT(a, --ix);
    if (d) {
      while (d) {
	++bits;
	d >>= 1;
      }
      break;
    }
  }
  bits += ix * MP_DIGIT_BIT;
  if (!bits)
    bits = 1;
  return bits;
}



