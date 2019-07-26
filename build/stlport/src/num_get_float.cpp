

















#include "stlport_prefix.h"

#include <limits>
#include <locale>
#include <istream>

#if (defined (__GNUC__) && !defined (__sun) && !defined (__hpux)) || \
    defined (__DMC__)
#  include <stdint.h>
#endif

#if defined (__linux__) || defined (__MINGW32__) || defined (__CYGWIN__) || \
    defined (__BORLANDC__) || defined (__DMC__) || defined (__HP_aCC)

#  if defined (__BORLANDC__)
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#  endif

union _ll {
  uint64_t i64;
  struct {
#  if defined (_STLP_BIG_ENDIAN)
    uint32_t hi;
    uint32_t lo;
#  elif defined (_STLP_LITTLE_ENDIAN)
    uint32_t lo;
    uint32_t hi;
#  else
#    error Unknown endianess
#  endif
  } i32;
};

#  if defined (__linux__) && !defined (__ANDROID__)
#    include <ieee754.h>
#  else
union ieee854_long_double {
  long double d;

  
  struct {
    unsigned int mantissa1:32;
    unsigned int mantissa0:32;
    unsigned int exponent:15;
    unsigned int negative:1;
    unsigned int empty:16;
  } ieee;
};

#    define IEEE854_LONG_DOUBLE_BIAS 0x3fff
#  endif
#endif

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE






#if !defined (_STLP_NO_WCHAR_T)
void  _STLP_CALL
_Initialize_get_float( const ctype<wchar_t>& ct,
                       wchar_t& Plus, wchar_t& Minus,
                       wchar_t& pow_e, wchar_t& pow_E,
                       wchar_t* digits) {
  char ndigits[11] = "0123456789";
  Plus  = ct.widen('+');
  Minus = ct.widen('-');
  pow_e = ct.widen('e');
  pow_E = ct.widen('E');
  ct.widen(ndigits + 0, ndigits + 10, digits);
}
#endif 







#if defined (_STLP_MSVC) || defined (__BORLANDC__) || defined (__ICL)
typedef unsigned long uint32;
typedef unsigned __int64 uint64;
#  define ULL(x) x##Ui64
#elif defined (__unix) || defined (__MINGW32__) || \
      (defined (__DMC__) && (__LONGLONG)) || defined (__WATCOMC__) || \
      defined (__ANDROID__)
typedef uint32_t uint32;
typedef uint64_t uint64;
#  define ULL(x) x##ULL
#else
#  error There should be some unsigned 64-bit integer on the system!
#endif




static void _Stl_mult64(const uint64 u, const uint64 v,
                        uint64& high, uint64& low) {
  const uint64 low_mask = ULL(0xffffffff);
  const uint64 u0 = u & low_mask;
  const uint64 u1 = u >> 32;
  const uint64 v0 = v & low_mask;
  const uint64 v1 = v >> 32;

  uint64 t = u0 * v0;
  low = t & low_mask;

  t = u1 * v0 + (t >> 32);
  uint64 w1 = t & low_mask;
  uint64 w2 = t >> 32;

  uint64 x = u0 * v1 + w1;
  low += (x & low_mask) << 32;
  high = u1 * v1 + w2 + (x >> 32);
}

#if !defined (__linux__) || defined (__ANDROID__)

#  define bit11 ULL(0x7ff)
#  define exponent_mask (bit11 << 52)

#  if !defined (__GNUC__) || (__GNUC__ != 3) || (__GNUC_MINOR__ != 4) || \
      (!defined (__CYGWIN__) && !defined (__MINGW32__))

inline
#  endif
void _Stl_set_exponent(uint64 &val, uint64 exp)
{ val = (val & ~exponent_mask) | ((exp & bit11) << 52); }

#endif 







static const uint64 _Stl_tenpow[80] = {
ULL(0xa000000000000000), 
ULL(0xc800000000000000), 
ULL(0xfa00000000000000), 
ULL(0x9c40000000000000), 
ULL(0xc350000000000000), 
ULL(0xf424000000000000), 
ULL(0x9896800000000000), 
ULL(0xbebc200000000000), 
ULL(0xee6b280000000000), 
ULL(0x9502f90000000000), 
ULL(0xba43b74000000000), 
ULL(0xe8d4a51000000000), 
ULL(0x9184e72a00000000), 
ULL(0xb5e620f480000000), 
ULL(0xe35fa931a0000000), 
ULL(0x8e1bc9bf04000000), 
ULL(0xb1a2bc2ec5000000), 
ULL(0xde0b6b3a76400000), 
ULL(0x8ac7230489e80000), 
ULL(0xad78ebc5ac620000), 
ULL(0xd8d726b7177a8000), 
ULL(0x878678326eac9000), 
ULL(0xa968163f0a57b400), 
ULL(0xd3c21bcecceda100), 
ULL(0x84595161401484a0), 
ULL(0xa56fa5b99019a5c8), 
ULL(0xcecb8f27f4200f3a), 

ULL(0xd0cf4b50cfe20766), 
ULL(0xd2d80db02aabd62c), 
ULL(0xd4e5e2cdc1d1ea96), 
ULL(0xd6f8d7509292d603), 
ULL(0xd910f7ff28069da4), 
ULL(0xdb2e51bfe9d0696a), 
ULL(0xdd50f1996b947519), 
ULL(0xdf78e4b2bd342cf7), 
ULL(0xe1a63853bbd26451), 
ULL(0xe3d8f9e563a198e5), 




ULL(0xfd87b5f28300ca0e), 
ULL(0xfb158592be068d2f), 
ULL(0xf8a95fcf88747d94), 
ULL(0xf64335bcf065d37d), 
ULL(0xf3e2f893dec3f126), 
ULL(0xf18899b1bc3f8ca2), 
ULL(0xef340a98172aace5), 
ULL(0xece53cec4a314ebe), 
ULL(0xea9c227723ee8bcb), 
ULL(0xe858ad248f5c22ca), 
ULL(0xe61acf033d1a45df), 
ULL(0xe3e27a444d8d98b8), 
ULL(0xe1afa13afbd14d6e)  
};

static const short _Stl_twoexp[80] = {
4,7,10,14,17,20,24,27,30,34,37,40,44,47,50,54,57,60,64,67,70,74,77,80,84,87,90,
183,276,369,462,555,648,741,834,927,1020,
-93,-186,-279,-372,-465,-558,-651,-744,-837,-930,-1023,-1116,-1209
};

#define  TEN_1  0           /* offset to 10 **   1 */
#define  TEN_27   26        /* offset to 10 **  27 */
#define  TEN_M28  37        /* offset to 10 ** -28 */
#define  NUM_HI_P 11
#define  NUM_HI_N 13

#define _Stl_HIBITULL (ULL(1) << 63)

static void _Stl_norm_and_round(uint64& p, int& norm, uint64 prodhi, uint64 prodlo) {
  norm = 0;
  if ((prodhi & _Stl_HIBITULL) == 0) {
                                


    if ((prodhi == ~_Stl_HIBITULL) &&
        ((prodlo >> 62) == 0x3)) {  



      p = _Stl_HIBITULL;
      return;
    }
    p = (prodhi << 1) | (prodlo >> 63); 
    norm = 1;
    prodlo <<= 1;
  }
  else {
    p = prodhi;
  }

  if ((prodlo & _Stl_HIBITULL) != 0) {     
    if (((p & 0x1) != 0) ||
        prodlo != _Stl_HIBITULL ) {    
      
      ++p;
      if (p == 0)
        ++p;
    }
  }
}





static void _Stl_tenscale(uint64& p, int exp, int& bexp) {
  bexp = 0;

  if ( exp == 0 ) {              
    return;
  }

  int exp_hi = 0, exp_lo = exp; 
  int tlo = TEN_1, thi;         
  int num_hi;                   

  if (exp > 0) {                
    if (exp_lo > 27) {
      exp_lo++;
      while (exp_lo > 27) {
        exp_hi++;
        exp_lo -= 28;
      }
    }
    thi = TEN_27;
    num_hi = NUM_HI_P;
  } else { 
    while (exp_lo < 0) {
      exp_hi++;
      exp_lo += 28;
    }
    thi = TEN_M28;
    num_hi = NUM_HI_N;
  }

  uint64 prodhi, prodlo;        
  int norm;                     

  int hi, lo;                   
  while (exp_hi) {              
    hi = (min) (exp_hi, num_hi);    
    exp_hi -= hi;               
    hi += thi-1;
    _Stl_mult64(p, _Stl_tenpow[hi], prodhi, prodlo);
    _Stl_norm_and_round(p, norm, prodhi, prodlo);
    bexp += _Stl_twoexp[hi] - norm;
  }

  if (exp_lo) {
    lo = tlo + exp_lo -1;
    _Stl_mult64(p, _Stl_tenpow[lo], prodhi, prodlo);
    _Stl_norm_and_round(p, norm, prodhi, prodlo);
    bexp += _Stl_twoexp[lo] - norm;
  }

  return;
}






#if !defined (__linux__) || defined (__ANDROID__)

union _Double_rep {
  uint64 ival;
  double val;
};

static double _Stl_atod(char *buffer, ptrdiff_t ndigit, int dexp) {
  typedef numeric_limits<double> limits;
  _Double_rep drep;
  uint64 &value = drep.ival;  






  uint32 guard;         
  uint64 rest;          

  int bexp;             
  int nzero;            
  int sexp;             

  char *bufferend;              

  
  bufferend = buffer + ndigit;
  value = 0;

  while (buffer < bufferend) {
    value *= 10;
    value += *buffer++;
  }

  
  if (value == 0) {
    return 0.0;
  }

  
  bexp = 64;                    

  
  nzero = 0;
  if ((value >> 32) != 0) { nzero  = 32; }    
  if ((value >> (16 + nzero)) != 0) { nzero += 16; }
  if ((value >> ( 8 + nzero)) != 0) { nzero +=  8; }
  if ((value >> ( 4 + nzero)) != 0) { nzero +=  4; }
  if ((value >> ( 2 + nzero)) != 0) { nzero +=  2; }
  if ((value >> ( 1 + nzero)) != 0) { nzero +=  1; }
  if ((value >> (     nzero)) != 0) { nzero +=  1; }

  
  value <<=  (64 - nzero);    
  bexp -= 64 - nzero;

  



  
  _Stl_tenscale(value, dexp, sexp);
  bexp += sexp;

  if (bexp <= -1022) {          
    bexp += 1022;
    if (bexp < -53) {          
      value = 0;
    }
    else {                      
      int lead0 = 12 - bexp;          

      
      if (lead0 > 64) {
        rest = value;
        guard = 0;
        value = 0;
      }
      else if (lead0 == 64) {
        rest = value & ((ULL(1)<< 63)-1);
        guard = (uint32) ((value>> 63) & 1 );
        value = 0;
      }
      else {
        rest = value & (((ULL(1) << lead0)-1)-1);
        guard = (uint32) (((value>> lead0)-1) & 1);
        value >>=  lead0; 
      }

      
      if (guard && ((value & 1) || rest) ) {
        ++value;
        if (value == (ULL(1) << (limits::digits - 1))) { 
          value = 0;
          _Stl_set_exponent(value, 1);
        }
      }
    }
  }
  else {                        
    
    rest = value & ((1 << 10) - 1);
    value >>= 10;
    guard = (uint32) value & 1;
    value >>= 1;

    






    if (guard) {
      if (((value&1)!=0) || (rest!=0)) {
        ++value;                        
        if ((value >> 53) != 0) {       
          value >>= 1;          
          ++bexp;
        }
      }
    }
    













    if (bexp > limits::max_exponent) {          
      return limits::infinity();
    }
    else {                      
      value &= ~(ULL(1) << (limits::digits - 1));   
      _Stl_set_exponent(value, bexp + 1022); 
    }
  }

  _STLP_STATIC_ASSERT(sizeof(uint64) >= sizeof(double))
  return drep.val;
}

#endif

#if defined (__linux__) || defined (__MINGW32__) || defined (__CYGWIN__) || \
    defined (__BORLANDC__) || defined (__DMC__) || defined (__HP_aCC)

template <class D, class IEEE, int M, int BIAS>
D _Stl_atodT(char *buffer, ptrdiff_t ndigit, int dexp)
{
  typedef numeric_limits<D> limits;

  
  char *bufferend = buffer + ndigit; 
  _ll vv;
  vv.i64 = 0L;

  while ( buffer < bufferend ) {
    vv.i64 *= 10;
    vv.i64 += *buffer++;
  }

  if ( vv.i64 == ULL(0) ) { 
    return D(0.0);
  }

  

  int bexp = 64; 

  
  int nzero = 0;
  if ((vv.i64 >> 32) != 0) { nzero = 32; }
  if ((vv.i64 >> (16 + nzero)) != 0) { nzero += 16; }
  if ((vv.i64 >> ( 8 + nzero)) != 0) { nzero +=  8; }
  if ((vv.i64 >> ( 4 + nzero)) != 0) { nzero +=  4; }
  if ((vv.i64 >> ( 2 + nzero)) != 0) { nzero +=  2; }
  if ((vv.i64 >> ( 1 + nzero)) != 0) { nzero +=  1; }
  if ((vv.i64 >> (     nzero)) != 0) { nzero +=  1; }

  
  nzero = 64 - nzero;
  vv.i64 <<= nzero;    
  bexp -= nzero;

  



  
  int sexp;
  _Stl_tenscale(vv.i64, dexp, sexp);
  bexp += sexp;

  if ( bexp >= limits::min_exponent ) { 
    if ( limits::digits < 64 ) {
      
      uint64_t rest = vv.i64 & ((~ULL(0) / ULL(2)) >> (limits::digits - 1));
      vv.i64 >>= M - 2;
      uint32_t guard = (uint32) vv.i64 & 1;
      vv.i64 >>= 1;

      







      if (guard) {
        if ( ((vv.i64 & 1) != 0) || (rest != 0) ) {
          vv.i64++;       
          if ( (vv.i64 >> (limits::digits < 64 ? limits::digits : 0)) != 0 ) { 
            vv.i64 >>= 1; 
            ++bexp;
          }
        }
      }

      vv.i64 &= ~(ULL(1) << (limits::digits - 1)); 
    }
    













    if (bexp > limits::max_exponent) { 
      return limits::infinity();
    }

    

    IEEE v;

    v.ieee.mantissa0 = vv.i32.hi;
    v.ieee.mantissa1 = vv.i32.lo;
    v.ieee.negative = 0;
    v.ieee.exponent = bexp + BIAS - 1;

    return v.d;
  }

  
  bexp += BIAS - 1;
  if (bexp < -limits::digits) { 
    vv.i64 = 0;
  } else {  

    




    int lead0 = M - bexp; 
    uint64_t rest;
    uint32_t guard;

    

    if (lead0 > 64) {
      rest = vv.i64;
      guard = 0;
      vv.i64 = 0;
    } else if (lead0 == 64) {
      rest = vv.i64 & ((ULL(1) << 63)-1);
      guard = (uint32) ((vv.i64 >> 63) & 1 );
      vv.i64 = 0;
    } else {
      rest = vv.i64 & (((ULL(1) << lead0)-1)-1);
      guard = (uint32) (((vv.i64 >> lead0)-1) & 1);
      vv.i64 >>=  lead0; 
    }

    
    if (guard && ( (vv.i64 & 1) || rest)) {
      vv.i64++;
      if (vv.i64 == (ULL(1) << (limits::digits - 1))) { 
        IEEE v;

        v.ieee.mantissa0 = 0;
        v.ieee.mantissa1 = 0;
        v.ieee.negative = 0;
        v.ieee.exponent = 1;
        return v.d;
      }
    }
  }

  IEEE v;

  v.ieee.mantissa0 = vv.i32.hi;
  v.ieee.mantissa1 = vv.i32.lo;
  v.ieee.negative = 0;
  v.ieee.exponent = 0;

  return v.d;
}
#endif 

#if !defined (__linux__) || defined (__ANDROID__)
static double _Stl_string_to_double(const char *s) {
  typedef numeric_limits<double> limits;
  const int max_digits = limits::digits10 + 2;
  unsigned c;
  unsigned Negate, decimal_point;
  char *d;
  int exp;
  int dpchar;
  char digits[max_digits];

  c = *s++;

  
  Negate = 0;
  if (c == '+') {
    c = *s++;
  } else if (c == '-') {
    Negate = 1;
    c = *s++;
  }

  d = digits;
  dpchar = '.' - '0';
  decimal_point = 0;
  exp = 0;

  for (;;) {
    c -= '0';
    if (c < 10) {
      if (d == digits + max_digits) {
        
        exp += (decimal_point ^ 1);
      } else {
        if (c == 0 && d == digits) {
          
        } else {
          *d++ = (char) c;
        }
        exp -= decimal_point;
      }
    } else if (c == (unsigned int) dpchar && !decimal_point) { 
      decimal_point = 1;
    } else {
      break;
    }
    c = *s++;
  }

  
  if (d == digits) {
    return 0.0;
  }

  if (c == 'e' - '0' || c == 'E' - '0') {
    register unsigned negate_exp = 0;
    register int e = 0;
    c = *s++;
    if (c == '+' || c == ' ') {
      c = *s++;
    } else if (c == '-') {
      negate_exp = 1;
      c = *s++;
    }
    if (c -= '0', c < 10) {
      do {
        e = e * 10 + (int)c;
        c = *s++;
      } while (c -= '0', c < 10);

      if (negate_exp) {
        e = -e;
      }
      exp += e;
    }
  }

  double x;
  ptrdiff_t n = d - digits;
  if ((exp + n - 1) < limits::min_exponent10) {
    x = 0;
  }
  else if ((exp + n - 1) > limits::max_exponent10) {
    x = limits::infinity();
  }
  else {
    


    x = _Stl_atod(digits, n, exp);
  }

  if (Negate) {
    x = -x;
  }

  return x;
}

#endif

#if defined (__linux__) || defined (__MINGW32__) || defined (__CYGWIN__) || \
    defined (__BORLANDC__) || defined (__DMC__) || defined (__HP_aCC)

template <class D, class IEEE, int M, int BIAS>
D _Stl_string_to_doubleT(const char *s)
{
  typedef numeric_limits<D> limits;
  const int max_digits = limits::digits10; ;
  unsigned c;
  unsigned decimal_point;
  char *d;
  int exp;
  D x;
  int dpchar;
  char digits[max_digits];

  c = *s++;

  
  bool Negate = false;
  if (c == '+') {
    c = *s++;
  } else if (c == '-') {
    Negate = true;
    c = *s++;
  }

  d = digits;
  dpchar = '.' - '0';
  decimal_point = 0;
  exp = 0;

  for (;;) {
    c -= '0';
    if (c < 10) {
      if (d == digits + max_digits) {
        
        exp += (decimal_point ^ 1);
      } else {
        if (c == 0 && d == digits) {
          
        } else {
          *d++ = (char) c;
        }
        exp -= decimal_point;
      }
    } else if (c == (unsigned int) dpchar && !decimal_point) {    
      decimal_point = 1;
    } else {
      break;
    }
    c = *s++;
  }
  
  if (d == digits) {
    return D(0.0);
  }

  if (c == 'e'-'0' || c == 'E'-'0') {
    bool negate_exp = false;
    register int e = 0;
    c = *s++;
    if (c == '+' || c == ' ') {
      c = *s++;
    } else if (c == '-') {
      negate_exp = true;
      c = *s++;
    }
    if (c -= '0', c < 10) {
      do {
        e = e * 10 + (int)c;
        c = *s++;
      } while (c -= '0', c < 10);

      if (negate_exp) {
        e = -e;
      }
      exp += e;
    }
  }

  ptrdiff_t n = d - digits;
  if ((exp + n - 1) < limits::min_exponent10) {
    return D(0.0); 
  } else if ((exp + n - 1) > limits::max_exponent10 ) {
    
    x = limits::infinity();
  } else {
    
    


    x = _Stl_atodT<D,IEEE,M,BIAS>(digits, n, exp);
  }

  return Negate ? -x : x;
}

#endif 

void _STLP_CALL
__string_to_float(const __iostring& v, float& val)
{
#if !defined (__linux__) || defined (__ANDROID__)
  val = (float)_Stl_string_to_double(v.c_str());
#else
  val = (float)_Stl_string_to_doubleT<double,ieee754_double,12,IEEE754_DOUBLE_BIAS>(v.c_str());
#endif
}

void _STLP_CALL
__string_to_float(const __iostring& v, double& val)
{
#if !defined (__linux__) || defined (__ANDROID__)
  val = _Stl_string_to_double(v.c_str());
#else
  val = _Stl_string_to_doubleT<double,ieee754_double,12,IEEE754_DOUBLE_BIAS>(v.c_str());
#endif
}

#if !defined (_STLP_NO_LONG_DOUBLE)
void _STLP_CALL
__string_to_float(const __iostring& v, long double& val) {
#if !defined (__linux__) && !defined (__MINGW32__) && !defined (__CYGWIN__) && \
    !defined (__BORLANDC__) && !defined (__DMC__) && !defined (__HP_aCC)
  
  _STLP_STATIC_ASSERT( sizeof(long double) <= sizeof(double) )
  val = _Stl_string_to_double(v.c_str());
#else
  val = _Stl_string_to_doubleT<long double,ieee854_long_double,16,IEEE854_LONG_DOUBLE_BIAS>(v.c_str());
#endif
}
#endif

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE




