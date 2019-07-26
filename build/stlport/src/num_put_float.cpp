

















#include "stlport_prefix.h"

#include <cmath>
#include <ios>
#include <locale>

#if defined (__DECCXX)
#  define NDIG 400
#else
#  define NDIG 82
#endif

#define todigit(x) ((x)+'0')

#if defined (_STLP_UNIX)

#  if defined (__sun)
#    include <floatingpoint.h>
#  endif

#  if defined (__sun) || defined (__digital__) || defined (__sgi) || defined (_STLP_SCO_OPENSERVER) || defined (__NCR_SVR)

#    include <values.h>
#    include <nan.h>
#  endif

#  if defined (__QNXNTO__) || ( defined(__GNUC__) && defined(__APPLE__) ) || defined(_STLP_USE_UCLIBC)  || \
      defined(__FreeBSD__)
#    define USE_SPRINTF_INSTEAD
#  endif

#  if defined (_AIX) 
#    include <math.h>
#    include <float.h>
#  endif

#  include <math.h>
#endif

#include <cstdio>
#include <cstdlib>

#if defined (_STLP_MSVC_LIB) || defined (__MINGW32__) || defined (__BORLANDC__) || defined (__DJGPP) || \
    defined (_STLP_SCO_OPENSERVER) || defined (__NCR_SVR)
#  include <float.h>
#endif

#if defined (__MRC__) || defined (__SC__)  || defined (_CRAY)  
#  include <fp.h>
#endif

#if defined (__CYGWIN__)
#  include <ieeefp.h>
#endif

#if defined (__MSL__)
#  include <cstdlib>  
#  include <cstdio>  
#  include <algorithm>
#  include <cassert>
#endif

#if defined (__ISCPP__)
#  include <cfloat>
#endif

#include <algorithm>

#if defined (__DMC__)
#  define snprintf _snprintf
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

#if defined (__MWERKS__) || defined(__BEOS__)
#  define USE_SPRINTF_INSTEAD
#endif

template <int N>
struct _Dig
{
    enum { dig = _Dig<N/10>::dig + 1 };
};

_STLP_TEMPLATE_NULL
struct _Dig<0>
{
    enum { dig = 0 };
};

#ifdef _STLP_NO_LONG_DOUBLE
# define MAXEDIGITS int(_Dig<DBL_MAX_10_EXP>::dig)
# define MAXFSIG DBL_DIG
# define MAXFCVT (DBL_DIG + 1)
#else
# define MAXEDIGITS int(_Dig<LDBL_MAX_10_EXP>::dig)
# define MAXFSIG LDBL_DIG
# define MAXFCVT (LDBL_DIG + 1)
#endif



#if !defined (USE_SPRINTF_INSTEAD)
#  if defined (__hpux) && defined (__GNUC__)
#    define _STLP_USE_SIGN_HELPER
#  elif defined (__DJGPP) || (defined (_STLP_USE_GLIBC) && ! defined (__MSL__)) || \
      defined (__CYGWIN__) || \
      defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || \
      defined (__HP_aCC)
static inline bool _Stl_is_nan_or_inf(double x)
#    if defined (isfinite)
{ return !isfinite(x); }
#    else
{ return !finite(x); }
#    endif
static inline bool _Stl_is_neg_nan(double x)    { return isnan(x) && ( copysign(1., x) < 0 ); }
static inline bool _Stl_is_inf(double x)        { return isinf(x); }

static inline bool _Stl_is_neg_inf(double x)    { return isinf(x) && x < 0; }
#  elif (defined (__unix) || defined (__unix__)) && \
         !defined (__APPLE__) && !defined (__DJGPP) && !defined(__osf__) && \
         !defined (_CRAY) && !defined (__ANDROID__)
static inline bool _Stl_is_nan_or_inf(double x) { return IsNANorINF(x); }
static inline bool _Stl_is_inf(double x)        { return IsNANorINF(x) && IsINF(x); }
static inline bool _Stl_is_neg_inf(double x)    { return (IsINF(x)) && (x < 0.0); }
static inline bool _Stl_is_neg_nan(double x)    { return IsNegNAN(x); }
#  elif defined (_STLP_MSVC_LIB) || defined (__MINGW32__) || defined (__BORLANDC__)
static inline bool _Stl_is_nan_or_inf(double x) { return !_finite(x); }
#    if !defined (__BORLANDC__)
static inline bool _Stl_is_inf(double x)        {
  int fclass = _fpclass(x);
  return fclass == _FPCLASS_NINF || fclass == _FPCLASS_PINF;
}
static inline bool _Stl_is_neg_inf(double x)    { return _fpclass(x) == _FPCLASS_NINF; }
#    else
static inline bool _Stl_is_inf(double x)        {  return _Stl_is_nan_or_inf(x) && !_isnan(x);}
static inline bool _Stl_is_neg_inf(double x)    {  return _Stl_is_inf(x) && x < 0 ; }
#    endif
static inline bool _Stl_is_neg_nan(double x)    { return _isnan(x) && _copysign(1., x) < 0 ; }
#    if defined (__BORLANDC__)
static inline bool _Stl_is_nan_or_inf(long double x) { return !_finitel(x); }
static inline bool _Stl_is_inf(long double x)        {  return _Stl_is_nan_or_inf(x) && !_isnanl(x);}
static inline bool _Stl_is_neg_inf(long double x)    {  return _Stl_is_inf(x) && x < 0 ; }
static inline bool _Stl_is_neg_nan(long double x)    { return _isnanl(x) && _copysignl(1.l, x) < 0 ; }
#    elif !defined (_STLP_NO_LONG_DOUBLE)

static inline bool _Stl_is_nan_or_inf(long double x) { return _Stl_is_nan_or_inf(__STATIC_CAST(double, x)); }
static inline bool _Stl_is_inf(long double x)        {  return _Stl_is_inf(__STATIC_CAST(double, x));}
static inline bool _Stl_is_neg_inf(long double x)    {  return _Stl_is_neg_inf(__STATIC_CAST(double, x)); }
static inline bool _Stl_is_neg_nan(long double x)    { return _Stl_is_neg_nan(__STATIC_CAST(double, x)); }
#    endif
#  elif defined (__MRC__) || defined (__SC__) || defined (__DMC__)
static bool _Stl_is_nan_or_inf(double x) { return isnan(x) || !isfinite(x); }
static bool _Stl_is_inf(double x)        { return !isfinite(x); }
static bool _Stl_is_neg_inf(double x)    { return !isfinite(x) && signbit(x); }
static bool _Stl_is_neg_nan(double x)    { return isnan(x) && signbit(x); }
#  elif  (defined(__GNUC__) && defined(__APPLE__))
static inline bool _Stl_is_nan_or_inf(double x) { return !finite(x); }
static inline bool _Stl_is_inf(double x)        {   return _Stl_is_nan_or_inf(x) && ! isnan(x); }
static inline bool _Stl_is_neg_inf(double x)    {   return _Stl_is_inf(x) && x < 0 ; }
static inline bool _Stl_is_neg_nan(double x)    { return isnan(x) && copysign(1., x) < 0 ; }
#  elif defined( _AIX ) 
static bool _Stl_is_nan_or_inf(double x) { return isnan(x) || !finite(x); }
static bool _Stl_is_inf(double x)        { return !finite(x); }

static bool _Stl_is_neg_inf(double x)    { return _Stl_is_inf(x) && ( copysign(1., x) < 0 );  }
static bool _Stl_is_neg_nan(double x)    { return isnan(x) && ( copysign(1., x) < 0 );  }
#  elif defined (__ISCPP__)
static inline bool _Stl_is_nan_or_inf  (double x) { return _fp_isINF(x) || _fp_isNAN(x); }
static inline bool _Stl_is_inf         (double x) { return _fp_isINF(x); }
static inline bool _Stl_is_neg_inf     (double x) { return _fp_isINF(x) && x < 0; }
static inline bool _Stl_is_neg_nan     (double x) { return _fp_isNAN(x) && x < 0; }
#  elif defined (_CRAY)
#    if defined (_CRAYIEEE)
static inline bool _Stl_is_nan_or_inf(double x) { return isnan(x) || isinf(x); }
static inline bool _Stl_is_inf(double x)        { return isinf(x); }
static inline bool _Stl_is_neg_inf(double x)    { return isinf(x) && signbit(x); }
static inline bool _Stl_is_neg_nan(double x)    { return isnan(x) && signbit(x); }
#    else
static inline bool _Stl_is_nan_or_inf(double x) { return false; }
static inline bool _Stl_is_inf(double x)        { return false; }
static inline bool _Stl_is_neg_inf(double x)    { return false; }
static inline bool _Stl_is_neg_nan(double x)    { return false; }
#    endif
#  else 
#    define USE_SPRINTF_INSTEAD
#  endif
#endif 

#if !defined (USE_SPRINTF_INSTEAD)




#  if defined (__CYGWIN__) || defined(__DJGPP)
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign, char* buf)
{ return ecvtbuf(x, n, pt, sign, buf); }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign, char* buf)
{ return fcvtbuf(x, n, pt, sign, buf); }
#    if !defined (_STLP_NO_LONG_DOUBLE)
#      if defined (__CYGWIN__)
#        define _STLP_EMULATE_LONG_DOUBLE_CVT
#      else
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign, char* buf)
{ return ecvtbuf(x, n, pt, sign, buf); }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign, char* buf)
{ return fcvtbuf(x, n, pt, sign, buf); }
#      endif
#    endif
#  elif defined (_STLP_USE_GLIBC)
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign, char* buf, size_t bsize)
{ return ecvt_r(x, n, pt, sign, buf, bsize) == 0 ? buf : 0; }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign, char* buf, size_t bsize)
{ return fcvt_r(x, n, pt, sign, buf, bsize) == 0 ? buf : 0; }
#    ifndef _STLP_NO_LONG_DOUBLE
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign, char* buf, size_t bsize)
{ return qecvt_r(x, n, pt, sign, buf, bsize) == 0 ? buf : 0; }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign, char* buf, size_t bsize)
{ return qfcvt_r(x, n, pt, sign, buf, bsize) == 0 ? buf : 0; }
#    endif
#    define _STLP_NEED_CVT_BUFFER_SIZE
#  elif defined (__sun)
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign, char* buf)
{ return econvert(x, n, pt, sign, buf); }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign, char* buf)
{ return fconvert(x, n, pt, sign, buf); }
#    ifndef _STLP_NO_LONG_DOUBLE
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign, char* buf)
{ return qeconvert(&x, n, pt, sign, buf); }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign, char* buf)
{ return qfconvert(&x, n, pt, sign, buf); }
#    endif
#  elif defined (__DECCXX)
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign, char* buf, size_t bsize)
{ return (ecvt_r(x, n, pt, sign, buf, bsize) == 0 ? buf : 0); }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign, char* buf, size_t bsize)
{ return (fcvt_r(x, n, pt, sign, buf, bsize) == 0 ? buf : 0); }
#    ifndef _STLP_NO_LONG_DOUBLE

static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign, char* buf, size_t bsize)
{ return (ecvt_r((double)x, n, pt, sign, buf, bsize) == 0 ? buf : 0) ; }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign, char* buf, size_t bsize)
{ return (fcvt_r((double)x, n, pt, sign, buf, bsize) == 0 ? buf : 0); }
#    endif
#    define _STLP_NEED_CVT_BUFFER_SIZE
#  elif defined (__hpux)
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign)
{ return ecvt(x, n, pt, sign); }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign)
{ return fcvt(x, n, pt, sign); }
#    if !defined (_STLP_NO_LONG_DOUBLE)
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign)
{ return _ldecvt(*(long_double*)&x, n, pt, sign); }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign)
{ return _ldfcvt(*(long_double*)&x, n, pt, sign); }
#    endif
#    define _STLP_CVT_NEED_SYNCHRONIZATION
#  elif defined (__unix) && !defined (__APPLE__) && !defined (_CRAY) && \
        !defined (__ANDROID__)
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign, char* buf)
{ return ecvt_r(x, n, pt, sign, buf); }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign, char* buf)
{ return fcvt_r(x, n, pt, sign, buf); }
#    if !defined (_STLP_NO_LONG_DOUBLE)
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign, char* buf)
{ return qecvt_r(x, n, pt, sign, buf); }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign, char* buf)
{ return qfcvt_r(x, n, pt, sign, buf); }
#    endif
#  elif defined (_STLP_MSVC_LIB) || defined (__MINGW32__) || defined (__BORLANDC__)
#    if defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
#      define _STLP_APPEND(a, b) a##b
#      define _STLP_BUF_PARAMS , char* buf, size_t bsize
#      define _STLP_SECURE_FUN(F, X, N, PT, SIGN) _STLP_APPEND(F, _s)(buf, bsize, X, N, PT, SIGN); return buf
#    else
#      define _STLP_BUF_PARAMS
#      define _STLP_SECURE_FUN(F, X, N, PT, SIGN) return F(X, N, PT, SIGN)
#      define _STLP_CVT_NEED_SYNCHRONIZATION
#    endif
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign _STLP_BUF_PARAMS)
{ _STLP_SECURE_FUN(_ecvt, x, n, pt, sign); }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign _STLP_BUF_PARAMS)
{ _STLP_SECURE_FUN(_fcvt, x, n, pt, sign); }
#    if !defined (_STLP_NO_LONG_DOUBLE)
#      if defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
#        define _STLP_PARAMS , buf, bsize
#      else
#        define _STLP_PARAMS
#      endif
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign _STLP_BUF_PARAMS)
{ return _Stl_ecvtR(__STATIC_CAST(double, x), n, pt, sign _STLP_PARAMS); }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign _STLP_BUF_PARAMS)
{ return _Stl_fcvtR(__STATIC_CAST(double, x), n, pt, sign _STLP_PARAMS); }
#      undef _STLP_PARAMS
#    endif
#    undef _STLP_SECURE_FUN
#    undef _STLP_BUF_PARAMS
#    undef _STLP_APPEND
#    if defined (__BORLANDC__) 
#      define _STLP_EMULATE_LONG_DOUBLE_CVT
#    endif
#  elif defined (__ISCPP__)
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign, char* buf)
{ return _fp_ecvt( x, n, pt, sign, buf); }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign, char* buf)
{ return _fp_fcvt(x, n, pt, sign, buf); }
#    if !defined (_STLP_NO_LONG_DOUBLE)
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign, char* buf)
{ return _fp_ecvt( x, n, pt, sign, buf); }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign, char* buf)
{ return _fp_fcvt(x, n, pt, sign, buf); }
#    endif
#  elif defined (_AIX) || defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || \
        defined (__MRC__) || defined (__SC__) || defined (_CRAY) || \
        defined (_STLP_SCO_OPENSERVER) || defined (__NCR_SVR) || \
        defined (__DMC__)
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign)
{ return ecvt(x, n, pt, sign ); }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign)
{ return fcvt(x, n, pt, sign); }
#    if !defined (_STLP_NO_LONG_DOUBLE)
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign)
{ return ecvt(x, n, pt, sign ); }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign)
{ return fcvt(x, n, pt, sign); }
#    endif
#    define _STLP_CVT_NEED_SYNCHRONIZATION
#  else
#    error Missing _Stl_ecvtR and _Stl_fcvtR implementations.
#  endif

#if defined (_STLP_CVT_NEED_SYNCHRONIZATION)


#  if defined (_STLP_THREADS)
static _STLP_STATIC_MUTEX& put_float_mutex() {
  static _STLP_STATIC_MUTEX __put_float_mutex _STLP_MUTEX_INITIALIZER;
  return __put_float_mutex;
}
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign, char* buf) {
  _STLP_auto_lock lock(put_float_mutex());
  strcpy(buf, _Stl_ecvtR(x, n, pt, sign)); return buf;
}
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign, char* buf) {
  _STLP_auto_lock lock(put_float_mutex());
  strcpy(buf, _Stl_fcvtR(x, n, pt, sign)); return buf;
}
#    if !defined (_STLP_NO_LONG_DOUBLE) && !defined (_STLP_EMULATE_LONG_DOUBLE_CVT)
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign, char* buf) {
  _STLP_auto_lock lock(put_float_mutex());
  strcpy(buf, _Stl_ecvtR(x, n, pt, sign)); return buf;
}
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign, char* buf) {
  _STLP_auto_lock lock(put_float_mutex());
  strcpy(buf, _Stl_fcvtR(x, n, pt, sign)); return buf;
}
#    endif
#  else
static inline char* _Stl_ecvtR(double x, int n, int* pt, int* sign, char*)
{ return _Stl_ecvtR(x, n, pt, sign); }
static inline char* _Stl_fcvtR(double x, int n, int* pt, int* sign, char*)
{ return _Stl_fcvtR(x, n, pt, sign); }
#    if !defined (_STLP_NO_LONG_DOUBLE) && !defined (_STLP_EMULATE_LONG_DOUBLE_CVT)
static inline char* _Stl_ecvtR(long double x, int n, int* pt, int* sign, char*)
{ return _Stl_ecvtR(x, n, pt, sign); }
static inline char* _Stl_fcvtR(long double x, int n, int* pt, int* sign, char*)
{ return _Stl_fcvtR(x, n, pt, sign); }
#    endif
#  endif
#endif

#  if !defined (_STLP_USE_SAFE_STRING_FUNCTIONS) && !defined (_STLP_NEED_CVT_BUFFER_SIZE)
#    define _STLP_CVT_BUFFER(B) B
#  else
#    define _STLP_CVT_BUFFER(B) _STLP_ARRAY_AND_SIZE(B)
#  endif

#  if defined (_STLP_EMULATE_LONG_DOUBLE_CVT)
static void __fill_fmtbuf(char* fmtbuf, ios_base::fmtflags flags, char long_modifier);


static char* _Stl_ecvtR(long double x, int n, int* pt, int* sign, char* buf) {
  
  
  double y = __STATIC_CAST(double, x); 
  if (x == y)
    return _Stl_ecvtR(y, n, pt, sign, buf);

  char fmtbuf[32];
  __fill_fmtbuf(fmtbuf, 0, 'L');
  sprintf(buf, fmtbuf, n, x < 0.0l ? -x : x);
  
  *pt = 0;
  *sign = 0;
  int i = -1;
  int offset = 0;
  while (buf[++i] != 0 && n != 0) {
    if (buf[i] >= '0' && buf[i] <= '9') {
      --n;
      if (offset != 0)
        buf[i - offset] = buf[i];
    }
    else {
      if (offset != 0) break;
      ++offset;
      *pt = i;
    }
  }
  if (offset != 0)
    buf[i - offset] = 0;
  
  int e = 0;
  while (buf[++i] != 0) {
    if (buf[i] >= '0' && buf[i] <= '9') {
      e = e * 10 + (buf[i] - '0');
    }
  }
  *pt += e;
  return buf;
}

static char* _Stl_fcvtR(long double x, int n, int* pt, int* sign, char* buf) {
  
  
  double y = __STATIC_CAST(double, x);
  if (x == y)
    return _Stl_fcvtR(y, n, pt, sign, buf);

  char fmtbuf[32];
  __fill_fmtbuf(fmtbuf, ios_base::fixed, 'L');
  sprintf(buf, fmtbuf, n, x < 0.0l ? -x : x);
  *pt = 0;
  *sign = 0;
  int i = -1;
  int offset = 0;
  while (buf[++i] != 0 && (offset == 0 || n != 0)) {
    if (buf[i] >= '0' && buf[i] <= '9') {
      if (offset != 0) {
        --n;
        buf[i - offset] = buf[i];
      }
    }
    else {
      ++offset;
      *pt = i;
    }
  }
  if (offset != 0)
    buf[i - offset] = 0;
  else
    *pt = i;
  return buf;
}
#endif












static size_t __format_float_scientific( __iostring& buf, const char *bp,
                                         int decpt, int sign, bool is_zero,
                                         ios_base::fmtflags flags,
                                         int precision) {
  
  if (sign)
    buf += '-';
  else if (flags & ios_base::showpos)
    buf += '+';

  
  buf += *bp++;

  
  
  
  size_t __group_pos = buf.size();

  
  if (precision != 0 || flags & ios_base::showpoint) {
    buf += '.';
  }

  
  while (*bp != 0 && precision--)
    buf += *bp++;

  
  if (precision > 0)
    buf.append(precision, '0');

  
  char expbuf[MAXEDIGITS + 3];
  
  char *suffix = expbuf + MAXEDIGITS + 2;
  *suffix = 0;
  if (!is_zero) {
    int nn = decpt - 1;
    if (nn < 0)
      nn = -nn;
    for (; nn > 9; nn /= 10)
      *--suffix = (char) todigit(nn % 10);
    *--suffix = (char) todigit(nn);
  }

  
  
  
  
  while (suffix > &expbuf[MAXEDIGITS])
    *--suffix = '0';

  
  *--suffix = (char) ((decpt > 0 || is_zero ) ? '+' : '-');

  
  *--suffix = flags & ios_base::uppercase ? 'E' : 'e';

  
  buf += suffix;
  return __group_pos;
}

static size_t __format_float_fixed( __iostring &buf, const char *bp,
                                    int decpt, int sign,
                                    ios_base::fmtflags flags,
                                    int precision) {
  if ( sign && (decpt > -precision) && (*bp != 0) )
    buf += '-';
  else if ( flags & ios_base::showpos )
    buf += '+';

  
  int nnn = decpt;
  do {
    buf += (nnn <= 0 || *bp == 0) ? '0' : *bp++;
  } while ( --nnn > 0 );

  
  size_t __group_pos = buf.size();

  
  if ( flags & ios_base::showpoint || precision > 0 ) {
    buf += '.';
  }

  
  while ( *bp != 0 && --precision >= 0 ) {
    buf += (++decpt <= 0) ? '0' : *bp++;
  }

  
  if (precision > 0)
    buf.append(precision, '0');

  return __group_pos;
}

#if defined (_STLP_USE_SIGN_HELPER)
template<class _FloatT>
struct float_sign_helper {
  float_sign_helper(_FloatT __x)
  { _M_number._num = __x; }

  bool is_negative() const {
    const unsigned short sign_mask(1 << (sizeof(unsigned short) * CHAR_BIT - 1));
    return (get_sign_word() & sign_mask) != 0;
  }
private:
  union {
    unsigned short _Words[8];
    _FloatT _num;
  } _M_number;

  unsigned short get_word_higher() const _STLP_NOTHROW
  { return _M_number._Words[0]; }
  unsigned short get_word_lower() const _STLP_NOTHROW
  { return _M_number._Words[(sizeof(_FloatT) >= 12 ? 10 : sizeof(_FloatT)) / sizeof(unsigned short) - 1]; }
  unsigned short get_sign_word() const _STLP_NOTHROW
#  if defined (_STLP_BIG_ENDIAN)
  { return get_word_higher(); }
#  else 
  { return get_word_lower(); }
#  endif
};
#endif

template <class _FloatT>
static size_t __format_nan_or_inf(__iostring& buf, _FloatT x, ios_base::fmtflags flags) {
  static const char* inf[2] = { "inf", "Inf" };
  static const char* nan[2] = { "nan", "NaN" };
  const char** inf_or_nan;
#if !defined (_STLP_USE_SIGN_HELPER)
  if (_Stl_is_inf(x)) {            
    inf_or_nan = inf;
    if (_Stl_is_neg_inf(x))
      buf += '-';
    else if (flags & ios_base::showpos)
      buf += '+';
  } else {                      
    inf_or_nan = nan;
    if (_Stl_is_neg_nan(x))
      buf += '-';
    else if (flags & ios_base::showpos)
      buf += '+';
  }
#else
  typedef numeric_limits<_FloatT> limits;
  if (x == limits::infinity() || x == -limits::infinity()) {
    inf_or_nan = inf;
  } else {                    
    inf_or_nan = nan;
  }
  float_sign_helper<_FloatT> helper(x);
  if (helper.is_negative())
    buf += '-';
  else if (flags & ios_base::showpos)
    buf += '+';
#endif
  size_t ret = buf.size();
  buf += inf_or_nan[flags & ios_base::uppercase ? 1 : 0];
  return ret;
}

static inline size_t __format_float(__iostring &buf, const char * bp,
                                    int decpt, int sign, bool is_zero,
                                    ios_base::fmtflags flags,
                                    int precision) {
  size_t __group_pos = 0;
  switch (flags & ios_base::floatfield) {
    case ios_base::scientific:
      __group_pos = __format_float_scientific( buf, bp, decpt, sign, is_zero,
                                               flags, precision);
      break;
    case ios_base::fixed:
      __group_pos = __format_float_fixed( buf, bp, decpt, sign,
                                          flags, precision);
      break;
    default: 
      
      if (flags & ios_base::showpoint || precision > 0) {
        if (precision == 0) precision = 1;
      } else
        precision = 6;

      
      if (is_zero)
        decpt = 1;

      int kk = precision;
      if (!(flags & ios_base::showpoint)) {
        size_t n = strlen(bp);
        if (n < (size_t)kk)
          kk = (int)n;
        while (kk >= 1 && bp[kk-1] == '0')
          --kk;
      }

      if (decpt < -3 || decpt > precision) {
        precision = kk - 1;
        __group_pos = __format_float_scientific( buf, bp, decpt, sign, is_zero,
                                                 flags, precision);
      } else {
        precision = kk - decpt;
        __group_pos = __format_float_fixed( buf, bp, decpt, sign,
                                            flags, precision);
      }
      break;
  } 
  return __group_pos;
}

#endif

#if defined (USE_SPRINTF_INSTEAD) || defined (_STLP_EMULATE_LONG_DOUBLE_CVT)
struct GroupPos {
  bool operator () (char __c) const {
    return __c == '.' ||
           __c == 'e' || __c == 'E';
  }
};


static void __fill_fmtbuf(char* fmtbuf, ios_base::fmtflags flags, char long_modifier) {
  fmtbuf[0] = '%';
  int i = 1;

  if (flags & ios_base::showpos)
    fmtbuf[i++] = '+';

  if (flags & ios_base::showpoint)
    fmtbuf[i++] = '#';

  fmtbuf[i++] = '.';
  fmtbuf[i++] = '*';

  if (long_modifier)
    fmtbuf[i++] = long_modifier;

  switch (flags & ios_base::floatfield)
    {
    case ios_base::scientific:
      fmtbuf[i++] = (flags & ios_base::uppercase) ?  'E' : 'e';
      break;
    case ios_base::fixed:
#  if defined (__FreeBSD__)
      fmtbuf[i++] = 'f';
#  else
      fmtbuf[i++] = (flags & ios_base::uppercase) ? 'F' : 'f';
#  endif
      break;
    default:
      fmtbuf[i++] = (flags & ios_base::uppercase) ?  'G' : 'g';
      break;
    }

  fmtbuf[i] = 0;
}

#endif  

template <class _FloatT>
static size_t  __write_floatT(__iostring &buf, ios_base::fmtflags flags, int precision,
                              _FloatT x
#if defined (USE_SPRINTF_INSTEAD)
                              , char modifier) {
  



  typedef numeric_limits<_FloatT> limits;
  char static_buf[limits::max_exponent10 + 6]; 
  char fmtbuf[32];
  __fill_fmtbuf(fmtbuf, flags, modifier);
  snprintf(_STLP_ARRAY_AND_SIZE(static_buf), fmtbuf, precision, x);
  buf = static_buf;
  return find_if(buf.begin(), buf.end(), GroupPos()) - buf.begin();
#else
                              ) {
  typedef numeric_limits<_FloatT> limits;
  
  if (limits::has_infinity && limits::has_quiet_NaN) {
    if (!(x == x) || 
        (x == limits::infinity() || x == -limits::infinity())) {
      return __format_nan_or_inf(buf, x, flags);
    }
  }
  
  
  else if (_Stl_is_nan_or_inf(x)) {
    return __format_nan_or_inf(buf, x, flags);
  }
#  if defined (__MINGW32__)
  
  if (x > numeric_limits<double>::max() ||
      x < -numeric_limits<double>::max()) {
    return __format_nan_or_inf(buf, x, flags);
  }
#  endif

  




  char cvtbuf[limits::max_exponent10 + limits::digits10 + 2 + 1];
  char *bp;
  int decpt, sign;

  switch (flags & ios_base::floatfield) {
  case ios_base::fixed:
    {
      




      int digits10 = (x > -1.0 && x < 1.0 ? -limits::min_exponent10 + limits::digits10 + 2
                                          : limits::digits10 + 2);
      bp = _Stl_fcvtR(x, (min) (precision, digits10), &decpt, &sign, _STLP_CVT_BUFFER(cvtbuf) );
    }
    break;
  case ios_base::scientific:
  default:
    
    {
      int digits10 = limits::digits10 + 2;
      bp = _Stl_ecvtR(x, (min) (precision, digits10), &decpt, &sign, _STLP_CVT_BUFFER(cvtbuf) );
    }
    break;
  }
  return __format_float(buf, bp, decpt, sign, x == 0.0, flags, precision);
#endif
}

size_t  _STLP_CALL
__write_float(__iostring &buf, ios_base::fmtflags flags, int precision,
              double x) {
  return __write_floatT(buf, flags, precision, x
#if defined (USE_SPRINTF_INSTEAD)
                                               , 0
#endif
                                                  );
}

#if !defined (_STLP_NO_LONG_DOUBLE)
size_t _STLP_CALL
__write_float(__iostring &buf, ios_base::fmtflags flags, int precision,
              long double x) {
  return __write_floatT(buf, flags, precision, x
#if defined (USE_SPRINTF_INSTEAD)
                                               , 'L'
#endif
                                                    );
}
#endif

void _STLP_CALL __get_floor_digits(__iostring &out, _STLP_LONGEST_FLOAT_TYPE __x) {
  typedef numeric_limits<_STLP_LONGEST_FLOAT_TYPE> limits;
#if defined (USE_SPRINTF_INSTEAD)
  char cvtbuf[limits::max_exponent10 + 6];
#  if !defined (_STLP_NO_LONG_DOUBLE)
  snprintf(_STLP_ARRAY_AND_SIZE(cvtbuf), "%Lf", __x); 
#  else
  snprintf(_STLP_ARRAY_AND_SIZE(cvtbuf), "%f", __x);  
#  endif
  char *p = strchr( cvtbuf, '.' );
  if ( p == 0 ) {
    out.append( cvtbuf );
  } else {
    out.append( cvtbuf, p );
  }
#else
  char cvtbuf[limits::max_exponent10 + 1];
  char * bp;
  int decpt, sign;
  bp = _Stl_fcvtR(__x, 0, &decpt, &sign, _STLP_CVT_BUFFER(cvtbuf));

  if (sign) {
    out += '-';
  }
  out.append(bp, bp + decpt);
#endif
}


#if !defined (_STLP_NO_WCHAR_T)
void _STLP_CALL __convert_float_buffer( __iostring const& str, __iowstring &out,
                                        const ctype<wchar_t>& ct, wchar_t dot, bool __check_dot) {
  string::const_iterator str_ite(str.begin()), str_end(str.end());

  
  if (__check_dot) {
    while (str_ite != str_end) {
      if (*str_ite != '.') {
        out += ct.widen(*str_ite++);
      } else {
        out += dot;
        break;
      }
    }
  } else {
    if (str_ite != str_end) {
      out += ct.widen(*str_ite);
    }
  }

  if (str_ite != str_end) {
    
    while (++str_ite != str_end) {
      out += ct.widen(*str_ite);
    }
  }
}

#endif

void _STLP_CALL
__adjust_float_buffer(__iostring &str, char dot) {
  if ('.' != dot) {
    size_t __dot_pos = str.find('.');
    if (__dot_pos != string::npos) {
      str[__dot_pos] = dot;
    }
  }
}

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE




