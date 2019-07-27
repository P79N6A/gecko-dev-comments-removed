






















#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "prdtoa.h"
#include "prlog.h"
#include "prprf.h"
#include "prmem.h"
#include "nsCRTGlue.h"
#include "nsTextFormatter.h"
#include "nsMemory.h"






#ifdef HAVE_VA_COPY
#define VARARGS_ASSIGN(foo, bar)        VA_COPY(foo,bar)
#elif defined(HAVE_VA_LIST_AS_ARRAY)
#define VARARGS_ASSIGN(foo, bar)	foo[0] = bar[0]
#else
#define VARARGS_ASSIGN(foo, bar)	(foo) = (bar)
#endif

typedef struct SprintfStateStr SprintfState;

struct SprintfStateStr
{
  int (*stuff)(SprintfState* aState, const char16_t* aStr, uint32_t aLen);

  char16_t* base;
  char16_t* cur;
  uint32_t maxlen;

  void* stuffclosure;
};




struct NumArgState
{
  int type;    
  va_list ap;  

  enum Type
  {
    INT16,
    UINT16,
    INTN,
    UINTN,
    INT32,
    UINT32,
    INT64,
    UINT64,
    STRING,
    DOUBLE,
    INTSTR,
    UNISTRING,
    UNKNOWN
  };
};

#define NAS_DEFAULT_NUM 20  /* default number of NumberedArgumentState array */

#define _LEFT		0x1
#define _SIGNED		0x2
#define _SPACED		0x4
#define _ZEROS		0x8
#define _NEG		0x10

#define ELEMENTS_OF(array_) (sizeof(array_) / sizeof(array_[0]))




static int
fill2(SprintfState* aState, const char16_t* aSrc, int aSrcLen, int aWidth,
      int aFlags)
{
  char16_t space = ' ';
  int rv;

  aWidth -= aSrcLen;
  
  if ((aWidth > 0) && ((aFlags & _LEFT) == 0)) {
    if (aFlags & _ZEROS) {
      space = '0';
    }
    while (--aWidth >= 0) {
      rv = (*aState->stuff)(aState, &space, 1);
      if (rv < 0) {
        return rv;
      }
    }
  }

  
  rv = (*aState->stuff)(aState, aSrc, aSrcLen);
  if (rv < 0) {
    return rv;
  }

  
  if ((aWidth > 0) && ((aFlags & _LEFT) != 0)) {
    while (--aWidth >= 0) {
      rv = (*aState->stuff)(aState, &space, 1);
      if (rv < 0) {
        return rv;
      }
    }
  }
  return 0;
}




static int
fill_n(SprintfState* aState, const char16_t* aSrc, int aSrcLen, int aWidth,
       int aPrec, int aType, int aFlags)
{
  int zerowidth   = 0;
  int precwidth   = 0;
  int signwidth   = 0;
  int leftspaces  = 0;
  int rightspaces = 0;
  int cvtwidth;
  int rv;
  char16_t sign;
  char16_t space = ' ';
  char16_t zero = '0';

  if ((aType & 1) == 0) {
    if (aFlags & _NEG) {
      sign = '-';
      signwidth = 1;
    } else if (aFlags & _SIGNED) {
      sign = '+';
      signwidth = 1;
    } else if (aFlags & _SPACED) {
      sign = ' ';
      signwidth = 1;
    }
  }
  cvtwidth = signwidth + aSrcLen;

  if (aPrec > 0) {
    if (aPrec > aSrcLen) {
      
      precwidth = aPrec - aSrcLen;
      cvtwidth += precwidth;
    }
  }

  if ((aFlags & _ZEROS) && (aPrec < 0)) {
    if (aWidth > cvtwidth) {
      
      zerowidth = aWidth - cvtwidth;
      cvtwidth += zerowidth;
    }
  }

  if (aFlags & _LEFT) {
    if (aWidth > cvtwidth) {
      
      rightspaces = aWidth - cvtwidth;
    }
  } else {
    if (aWidth > cvtwidth) {
      
      leftspaces = aWidth - cvtwidth;
    }
  }
  while (--leftspaces >= 0) {
    rv = (*aState->stuff)(aState, &space, 1);
    if (rv < 0) {
      return rv;
    }
  }
  if (signwidth) {
    rv = (*aState->stuff)(aState, &sign, 1);
    if (rv < 0) {
      return rv;
    }
  }
  while (--precwidth >= 0) {
    rv = (*aState->stuff)(aState,  &space, 1);
    if (rv < 0) {
      return rv;
    }
  }
  while (--zerowidth >= 0) {
    rv = (*aState->stuff)(aState,  &zero, 1);
    if (rv < 0) {
      return rv;
    }
  }
  rv = (*aState->stuff)(aState, aSrc, aSrcLen);
  if (rv < 0) {
    return rv;
  }
  while (--rightspaces >= 0) {
    rv = (*aState->stuff)(aState,  &space, 1);
    if (rv < 0) {
      return rv;
    }
  }
  return 0;
}




static int
cvt_l(SprintfState* aState, long aNum, int aWidth, int aPrec, int aRadix,
      int aType, int aFlags, const char16_t* aHexStr)
{
  char16_t cvtbuf[100];
  char16_t* cvt;
  int digits;

  
  if ((aPrec == 0) && (aNum == 0)) {
    return 0;
  }

  




  cvt = &cvtbuf[0] + ELEMENTS_OF(cvtbuf);
  digits = 0;
  while (aNum) {
    int digit = (((unsigned long)aNum) % aRadix) & 0xF;
    *--cvt = aHexStr[digit];
    digits++;
    aNum = (long)(((unsigned long)aNum) / aRadix);
  }
  if (digits == 0) {
    *--cvt = '0';
    digits++;
  }

  



  return fill_n(aState, cvt, digits, aWidth, aPrec, aType, aFlags);
}




static int
cvt_ll(SprintfState* aState, int64_t aNum, int aWidth, int aPrec, int aRadix,
       int aType, int aFlags, const char16_t* aHexStr)
{
  char16_t cvtbuf[100];
  char16_t* cvt;
  int digits;
  int64_t rad;

  
  if (aPrec == 0 && aNum == 0) {
    return 0;
  }

  




  rad = aRadix;
  cvt = &cvtbuf[0] + ELEMENTS_OF(cvtbuf);
  digits = 0;
  while (aNum != 0) {
    *--cvt = aHexStr[int32_t(aNum % rad) & 0xf];
    digits++;
    aNum /= rad;
  }
  if (digits == 0) {
    *--cvt = '0';
    digits++;
  }

  



  return fill_n(aState, cvt, digits, aWidth, aPrec, aType, aFlags);
}





static int
cvt_f(SprintfState* aState, double aDouble, int aWidth, int aPrec,
      const char16_t aType, int aFlags)
{
  int    mode = 2;
  int    decpt;
  int    sign;
  char   buf[256];
  char*  bufp = buf;
  int    bufsz = 256;
  char   num[256];
  char*  nump;
  char*  endnum;
  int    numdigits = 0;
  char   exp = 'e';

  if (aPrec == -1) {
    aPrec = 6;
  } else if (aPrec > 50) {
    
    
    aPrec = 50;
  }

  switch (aType) {
    case 'f':
      numdigits = aPrec;
      mode = 3;
      break;
    case 'E':
      exp = 'E';
    
    case 'e':
      numdigits = aPrec + 1;
      mode = 2;
      break;
    case 'G':
      exp = 'E';
    
    case 'g':
      if (aPrec == 0) {
        aPrec = 1;
      }
      numdigits = aPrec;
      mode = 2;
      break;
    default:
      NS_ERROR("invalid aType passed to cvt_f");
  }

  if (PR_dtoa(aDouble, mode, numdigits, &decpt, &sign,
              &endnum, num, bufsz) == PR_FAILURE) {
    buf[0] = '\0';
    return -1;
  }
  numdigits = endnum - num;
  nump = num;

  if (sign) {
    *bufp++ = '-';
  } else if (aFlags & _SIGNED) {
    *bufp++ = '+';
  }

  if (decpt == 9999) {
    while ((*bufp++ = *nump++)) {
    }
  } else {

    switch (aType) {

      case 'E':
      case 'e':

        *bufp++ = *nump++;
        if (aPrec > 0) {
          *bufp++ = '.';
          while (*nump) {
            *bufp++ = *nump++;
            aPrec--;
          }
          while (aPrec-- > 0) {
            *bufp++ = '0';
          }
        }
        *bufp++ = exp;
        PR_snprintf(bufp, bufsz - (bufp - buf), "%+03d", decpt - 1);
        break;

      case 'f':

        if (decpt < 1) {
          *bufp++ = '0';
          if (aPrec > 0) {
            *bufp++ = '.';
            while (decpt++ && aPrec-- > 0) {
              *bufp++ = '0';
            }
            while (*nump && aPrec-- > 0) {
              *bufp++ = *nump++;
            }
            while (aPrec-- > 0) {
              *bufp++ = '0';
            }
          }
        } else {
          while (*nump && decpt-- > 0) {
            *bufp++ = *nump++;
          }
          while (decpt-- > 0) {
            *bufp++ = '0';
          }
          if (aPrec > 0) {
            *bufp++ = '.';
            while (*nump && aPrec-- > 0) {
              *bufp++ = *nump++;
            }
            while (aPrec-- > 0) {
              *bufp++ = '0';
            }
          }
        }
        *bufp = '\0';
        break;

      case 'G':
      case 'g':

        if ((decpt < -3) || ((decpt - 1) >= aPrec)) {
          *bufp++ = *nump++;
          numdigits--;
          if (numdigits > 0) {
            *bufp++ = '.';
            while (*nump) {
              *bufp++ = *nump++;
            }
          }
          *bufp++ = exp;
          PR_snprintf(bufp, bufsz - (bufp - buf), "%+03d", decpt - 1);
        } else {
          if (decpt < 1) {
            *bufp++ = '0';
            if (aPrec > 0) {
              *bufp++ = '.';
              while (decpt++) {
                *bufp++ = '0';
              }
              while (*nump) {
                *bufp++ = *nump++;
              }
            }
          } else {
            while (*nump && decpt-- > 0) {
              *bufp++ = *nump++;
              numdigits--;
            }
            while (decpt-- > 0) {
              *bufp++ = '0';
            }
            if (numdigits > 0) {
              *bufp++ = '.';
              while (*nump) {
                *bufp++ = *nump++;
              }
            }
          }
          *bufp = '\0';
        }
    }
  }

  char16_t rbuf[256];
  char16_t* rbufp = rbuf;
  bufp = buf;
  
  while ((*rbufp++ = *bufp++)) {
  }
  *rbufp = '\0';

  return fill2(aState, rbuf, NS_strlen(rbuf), aWidth, aFlags);
}






static int
cvt_S(SprintfState* aState, const char16_t* aStr, int aWidth, int aPrec,
      int aFlags)
{
  int slen;

  if (aPrec == 0) {
    return 0;
  }

  
  slen = aStr ? NS_strlen(aStr) : 6;
  if (aPrec > 0) {
    if (aPrec < slen) {
      slen = aPrec;
    }
  }

  
  return fill2(aState, aStr ? aStr : MOZ_UTF16("(null)"), slen, aWidth, aFlags);
}






static int
cvt_s(SprintfState* aState, const char* aStr, int aWidth, int aPrec, int aFlags)
{
  NS_ConvertUTF8toUTF16 utf16Val(aStr);
  return cvt_S(aState, utf16Val.get(), aWidth, aPrec, aFlags);
}








static struct NumArgState*
BuildArgArray(const char16_t* aFmt, va_list aAp, int* aRv,
              struct NumArgState* aNasArray)
{
  int number = 0, cn = 0, i;
  const char16_t* p;
  char16_t  c;
  struct NumArgState* nas;

  



  p = aFmt;
  *aRv = 0;
  i = 0;
  while ((c = *p++) != 0) {
    if (c != '%') {
      continue;
    }
    
    if ((c = *p++) == '%') {
      continue;
    }

    while (c != 0) {
      if (c > '9' || c < '0') {
        
        if (c == '$') {
          if (i > 0) {
            *aRv = -1;
            return nullptr;
          }
          number++;
          break;

        } else {
          
          if (number > 0) {
            *aRv = -1;
            return nullptr;
          }
          i = 1;
          break;
        }
      }
      c = *p++;
    }
  }

  if (number == 0) {
    return nullptr;
  }

  if (number > NAS_DEFAULT_NUM) {
    nas = (struct NumArgState*)moz_xmalloc(number * sizeof(struct NumArgState));
    if (!nas) {
      *aRv = -1;
      return nullptr;
    }
  } else {
    nas = aNasArray;
  }

  for (i = 0; i < number; i++) {
    nas[i].type = NumArgState::UNKNOWN;
  }

  



  p = aFmt;
  while ((c = *p++) != 0) {
    if (c != '%') {
      continue;
    }
    c = *p++;
    if (c == '%') {
      continue;
    }
    cn = 0;
    
    while (c && c != '$') {
      cn = cn * 10 + c - '0';
      c = *p++;
    }

    if (!c || cn < 1 || cn > number) {
      *aRv = -1;
      break;
    }

    

    cn--;
    if (nas[cn].type != NumArgState::UNKNOWN) {
      continue;
    }

    c = *p++;

    
    if (c == '*') {
      
      *aRv = -1;
      break;
    } else {
      while ((c >= '0') && (c <= '9')) {
        c = *p++;
      }
    }

    
    if (c == '.') {
      c = *p++;
      if (c == '*') {
        
        *aRv = -1;
        break;
      } else {
        while ((c >= '0') && (c <= '9')) {
          c = *p++;
        }
      }
    }

    
    nas[cn].type = NumArgState::INTN;
    if (c == 'h') {
      nas[cn].type = NumArgState::INT16;
      c = *p++;
    } else if (c == 'L') {
      
      nas[cn].type = NumArgState::INT64;
      c = *p++;
    } else if (c == 'l') {
      nas[cn].type = NumArgState::INT32;
      c = *p++;
      if (c == 'l') {
        nas[cn].type = NumArgState::INT64;
        c = *p++;
      }
    }

    
    switch (c) {
      case 'd':
      case 'c':
      case 'i':
      case 'o':
      case 'u':
      case 'x':
      case 'X':
        break;

      case 'e':
      case 'f':
      case 'g':
        nas[cn].type = NumArgState::DOUBLE;
        break;

      case 'p':
        
        if (sizeof(void*) == sizeof(int32_t)) {
          nas[cn].type = NumArgState::UINT32;
        } else if (sizeof(void*) == sizeof(int64_t)) {
          nas[cn].type = NumArgState::UINT64;
        } else if (sizeof(void*) == sizeof(int)) {
          nas[cn].type = NumArgState::UINTN;
        } else {
          nas[cn].type = NumArgState::UNKNOWN;
        }
        break;

      case 'C':
        
        PR_ASSERT(0);
        nas[cn].type = NumArgState::UNKNOWN;
        break;

      case 'S':
        nas[cn].type = NumArgState::UNISTRING;
        break;

      case 's':
        nas[cn].type = NumArgState::STRING;
        break;

      case 'n':
        nas[cn].type = NumArgState::INTSTR;
        break;

      default:
        PR_ASSERT(0);
        nas[cn].type = NumArgState::UNKNOWN;
        break;
    }

    
    if (nas[cn].type == NumArgState::UNKNOWN) {
      *aRv = -1;
      break;
    }
  }


  



  if (*aRv < 0) {
    if (nas != aNasArray) {
      PR_DELETE(nas);
    }
    return nullptr;
  }

  cn = 0;
  while (cn < number) {
    if (nas[cn].type == NumArgState::UNKNOWN) {
      cn++;
      continue;
    }

    VARARGS_ASSIGN(nas[cn].ap, aAp);

    switch (nas[cn].type) {
      case NumArgState::INT16:
      case NumArgState::UINT16:
      case NumArgState::INTN:
      case NumArgState::UINTN:     (void)va_arg(aAp, int);         break;

      case NumArgState::INT32:     (void)va_arg(aAp, int32_t);     break;

      case NumArgState::UINT32:    (void)va_arg(aAp, uint32_t);    break;

      case NumArgState::INT64:     (void)va_arg(aAp, int64_t);     break;

      case NumArgState::UINT64:    (void)va_arg(aAp, uint64_t);    break;

      case NumArgState::STRING:    (void)va_arg(aAp, char*);       break;

      case NumArgState::INTSTR:    (void)va_arg(aAp, int*);        break;

      case NumArgState::DOUBLE:    (void)va_arg(aAp, double);      break;

      case NumArgState::UNISTRING: (void)va_arg(aAp, char16_t*);   break;

      default:
        if (nas != aNasArray) {
          PR_DELETE(nas);
        }
        *aRv = -1;
        return nullptr;
    }
    cn++;
  }
  return nas;
}




static int
dosprintf(SprintfState* aState, const char16_t* aFmt, va_list aAp)
{
  char16_t c;
  int flags, width, prec, radix, type;
  union
  {
    char16_t ch;
    int i;
    long l;
    int64_t ll;
    double d;
    const char* s;
    const char16_t* S;
    int* ip;
  } u;
  char16_t space = ' ';

  nsAutoString hex;
  hex.AssignLiteral("0123456789abcdef");

  nsAutoString HEX;
  HEX.AssignLiteral("0123456789ABCDEF");

  const char16_t* hexp;
  int rv, i;
  struct NumArgState* nas = nullptr;
  struct NumArgState  nasArray[NAS_DEFAULT_NUM];


  



  nas = BuildArgArray(aFmt, aAp, &rv, nasArray);
  if (rv < 0) {
    
    PR_ASSERT(0);
    return rv;
  }

  while ((c = *aFmt++) != 0) {
    if (c != '%') {
      rv = (*aState->stuff)(aState, aFmt - 1, 1);
      if (rv < 0) {
        return rv;
      }
      continue;
    }

    



    flags = 0;
    c = *aFmt++;
    if (c == '%') {
      
      rv = (*aState->stuff)(aState, aFmt - 1, 1);
      if (rv < 0) {
        return rv;
      }
      continue;
    }

    if (nas) {
      
      i = 0;
      
      while (c && c != '$') {
        i = (i * 10) + (c - '0');
        c = *aFmt++;
      }

      if (nas[i - 1].type == NumArgState::UNKNOWN) {
        if (nas && (nas != nasArray)) {
          PR_DELETE(nas);
        }
        return -1;
      }

      VARARGS_ASSIGN(aAp, nas[i - 1].ap);
      c = *aFmt++;
    }

    






    while ((c == '-') || (c == '+') || (c == ' ') || (c == '0')) {
      if (c == '-') {
        flags |= _LEFT;
      }
      if (c == '+') {
        flags |= _SIGNED;
      }
      if (c == ' ') {
        flags |= _SPACED;
      }
      if (c == '0') {
        flags |= _ZEROS;
      }
      c = *aFmt++;
    }
    if (flags & _SIGNED) {
      flags &= ~_SPACED;
    }
    if (flags & _LEFT) {
      flags &= ~_ZEROS;
    }

    
    if (c == '*') {
      c = *aFmt++;
      width = va_arg(aAp, int);
    } else {
      width = 0;
      while ((c >= '0') && (c <= '9')) {
        width = (width * 10) + (c - '0');
        c = *aFmt++;
      }
    }

    
    prec = -1;
    if (c == '.') {
      c = *aFmt++;
      if (c == '*') {
        c = *aFmt++;
        prec = va_arg(aAp, int);
      } else {
        prec = 0;
        while ((c >= '0') && (c <= '9')) {
          prec = (prec * 10) + (c - '0');
          c = *aFmt++;
        }
      }
    }

    
    type = NumArgState::INTN;
    if (c == 'h') {
      type = NumArgState::INT16;
      c = *aFmt++;
    } else if (c == 'L') {
      
      type = NumArgState::INT64;
      c = *aFmt++;
    } else if (c == 'l') {
      type = NumArgState::INT32;
      c = *aFmt++;
      if (c == 'l') {
        type = NumArgState::INT64;
        c = *aFmt++;
      }
    }

    
    hexp = hex.get();
    switch (c) {
      case 'd':
      case 'i':                               
        radix = 10;
        goto fetch_and_convert;

      case 'o':                               
        radix = 8;
        type |= 1;
        goto fetch_and_convert;

      case 'u':                               
        radix = 10;
        type |= 1;
        goto fetch_and_convert;

      case 'x':                               
        radix = 16;
        type |= 1;
        goto fetch_and_convert;

      case 'X':                               
        radix = 16;
        hexp = HEX.get();
        type |= 1;
        goto fetch_and_convert;

        fetch_and_convert:
        switch (type) {
          case NumArgState::INT16:
            u.l = va_arg(aAp, int);
            if (u.l < 0) {
              u.l = -u.l;
              flags |= _NEG;
            }
            goto do_long;
          case NumArgState::UINT16:
            u.l = va_arg(aAp, int) & 0xffff;
            goto do_long;
          case NumArgState::INTN:
            u.l = va_arg(aAp, int);
            if (u.l < 0) {
              u.l = -u.l;
              flags |= _NEG;
            }
            goto do_long;
          case NumArgState::UINTN:
            u.l = (long)va_arg(aAp, unsigned int);
            goto do_long;

          case NumArgState::INT32:
            u.l = va_arg(aAp, int32_t);
            if (u.l < 0) {
              u.l = -u.l;
              flags |= _NEG;
            }
            goto do_long;
          case NumArgState::UINT32:
            u.l = (long)va_arg(aAp, uint32_t);
          do_long:
            rv = cvt_l(aState, u.l, width, prec, radix, type, flags, hexp);
            if (rv < 0) {
              return rv;
            }
            break;

          case NumArgState::INT64:
            u.ll = va_arg(aAp, int64_t);
            if (u.ll < 0) {
              u.ll = -u.ll;
              flags |= _NEG;
            }
            goto do_longlong;
          case NumArgState::UINT64:
            u.ll = va_arg(aAp, uint64_t);
          do_longlong:
            rv = cvt_ll(aState, u.ll, width, prec, radix, type, flags, hexp);
            if (rv < 0) {
              return rv;
            }
            break;
        }
        break;

      case 'e':
      case 'E':
      case 'f':
      case 'g':
      case 'G':
        u.d = va_arg(aAp, double);
        rv = cvt_f(aState, u.d, width, prec, c, flags);
        if (rv < 0) {
          return rv;
        }
        break;

      case 'c':
        u.ch = va_arg(aAp, int);
        if ((flags & _LEFT) == 0) {
          while (width-- > 1) {
            rv = (*aState->stuff)(aState, &space, 1);
            if (rv < 0) {
              return rv;
            }
          }
        }
        rv = (*aState->stuff)(aState, &u.ch, 1);
        if (rv < 0) {
          return rv;
        }
        if (flags & _LEFT) {
          while (width-- > 1) {
            rv = (*aState->stuff)(aState, &space, 1);
            if (rv < 0) {
              return rv;
            }
          }
        }
        break;

      case 'p':
        if (sizeof(void*) == sizeof(int32_t)) {
          type = NumArgState::UINT32;
        } else if (sizeof(void*) == sizeof(int64_t)) {
          type = NumArgState::UINT64;
        } else if (sizeof(void*) == sizeof(int)) {
          type = NumArgState::UINTN;
        } else {
          PR_ASSERT(0);
          break;
        }
        radix = 16;
        goto fetch_and_convert;

#if 0
      case 'C':
        
        PR_ASSERT(0);
        break;
#endif

      case 'S':
        u.S = va_arg(aAp, const char16_t*);
        rv = cvt_S(aState, u.S, width, prec, flags);
        if (rv < 0) {
          return rv;
        }
        break;

      case 's':
        u.s = va_arg(aAp, const char*);
        rv = cvt_s(aState, u.s, width, prec, flags);
        if (rv < 0) {
          return rv;
        }
        break;

      case 'n':
        u.ip = va_arg(aAp, int*);
        if (u.ip) {
          *u.ip = aState->cur - aState->base;
        }
        break;

      default:
        
#if 0
        PR_ASSERT(0);
#endif
        char16_t perct = '%';
        rv = (*aState->stuff)(aState, &perct, 1);
        if (rv < 0) {
          return rv;
        }
        rv = (*aState->stuff)(aState, aFmt - 1, 1);
        if (rv < 0) {
          return rv;
        }
    }
  }

  
  char16_t null = '\0';

  rv = (*aState->stuff)(aState, &null, 1);

  if (nas && (nas != nasArray)) {
    PR_DELETE(nas);
  }

  return rv;
}



static int
StringStuff(SprintfState* aState, const char16_t* aStr, uint32_t aLen)
{
  if (*aStr == '\0') {
    return 0;
  }

  ptrdiff_t off = aState->cur - aState->base;

  nsAString* str = static_cast<nsAString*>(aState->stuffclosure);
  str->Append(aStr, aLen);

  aState->base = str->BeginWriting();
  aState->cur = aState->base + off;

  return 0;
}





static int
GrowStuff(SprintfState* aState, const char16_t* aStr, uint32_t aLen)
{
  ptrdiff_t off;
  char16_t* newbase;
  uint32_t newlen;

  off = aState->cur - aState->base;
  if (off + aLen >= aState->maxlen) {
    
    newlen = aState->maxlen + ((aLen > 32) ? aLen : 32);
    if (aState->base) {
      newbase = (char16_t*)moz_xrealloc(aState->base,
                                        newlen * sizeof(char16_t));
    } else {
      newbase = (char16_t*)moz_xmalloc(newlen * sizeof(char16_t));
    }
    if (!newbase) {
      
      return -1;
    }
    aState->base = newbase;
    aState->maxlen = newlen;
    aState->cur = aState->base + off;
  }

  
  while (aLen) {
    --aLen;
    *aState->cur++ = *aStr++;
  }
  PR_ASSERT((uint32_t)(aState->cur - aState->base) <= aState->maxlen);
  return 0;
}




char16_t*
nsTextFormatter::smprintf(const char16_t* aFmt, ...)
{
  va_list ap;
  char16_t* rv;

  va_start(ap, aFmt);
  rv = nsTextFormatter::vsmprintf(aFmt, ap);
  va_end(ap);
  return rv;
}

uint32_t
nsTextFormatter::ssprintf(nsAString& aOut, const char16_t* aFmt, ...)
{
  va_list ap;
  uint32_t rv;

  va_start(ap, aFmt);
  rv = nsTextFormatter::vssprintf(aOut, aFmt, ap);
  va_end(ap);
  return rv;
}

uint32_t
nsTextFormatter::vssprintf(nsAString& aOut, const char16_t* aFmt, va_list aAp)
{
  SprintfState ss;
  ss.stuff = StringStuff;
  ss.base = 0;
  ss.cur = 0;
  ss.maxlen = 0;
  ss.stuffclosure = &aOut;

  aOut.Truncate();
  int n = dosprintf(&ss, aFmt, aAp);
  return n ? n - 1 : n;
}

char16_t*
nsTextFormatter::vsmprintf(const char16_t* aFmt, va_list aAp)
{
  SprintfState ss;
  int rv;

  ss.stuff = GrowStuff;
  ss.base = 0;
  ss.cur = 0;
  ss.maxlen = 0;
  rv = dosprintf(&ss, aFmt, aAp);
  if (rv < 0) {
    if (ss.base) {
      PR_DELETE(ss.base);
    }
    return 0;
  }
  return ss.base;
}




static int
LimitStuff(SprintfState* aState, const char16_t* aStr, uint32_t aLen)
{
  uint32_t limit = aState->maxlen - (aState->cur - aState->base);

  if (aLen > limit) {
    aLen = limit;
  }
  while (aLen) {
    --aLen;
    *aState->cur++ = *aStr++;
  }
  return 0;
}





uint32_t
nsTextFormatter::snprintf(char16_t* aOut, uint32_t aOutLen,
                          const char16_t* aFmt, ...)
{
  va_list ap;
  uint32_t rv;

  PR_ASSERT((int32_t)aOutLen > 0);
  if ((int32_t)aOutLen <= 0) {
    return 0;
  }

  va_start(ap, aFmt);
  rv = nsTextFormatter::vsnprintf(aOut, aOutLen, aFmt, ap);
  va_end(ap);
  return rv;
}

uint32_t
nsTextFormatter::vsnprintf(char16_t* aOut, uint32_t aOutLen,
                           const char16_t* aFmt, va_list aAp)
{
  SprintfState ss;
  uint32_t n;

  PR_ASSERT((int32_t)aOutLen > 0);
  if ((int32_t)aOutLen <= 0) {
    return 0;
  }

  ss.stuff = LimitStuff;
  ss.base = aOut;
  ss.cur = aOut;
  ss.maxlen = aOutLen;
  (void) dosprintf(&ss, aFmt, aAp);

  
  if ((ss.cur != ss.base) && (*(ss.cur - 1) != '\0')) {
    *(--ss.cur) = '\0';
  }

  n = ss.cur - ss.base;
  return n ? n - 1 : n;
}




void
nsTextFormatter::smprintf_free(char16_t* aMem)
{
  free(aMem);
}

