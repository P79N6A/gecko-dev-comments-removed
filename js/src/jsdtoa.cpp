









































#include "jslibmath.h"
#include "jstypes.h"
#include "jsstdint.h"
#include "jsdtoa.h"
#include "jsprf.h"
#include "jsutil.h" 
#include "jspubtd.h"
#include "jsnum.h"
#include "jsbit.h"

#ifdef JS_THREADSAFE
#include "jslock.h"
#endif

#ifdef IS_LITTLE_ENDIAN
#define IEEE_8087
#else
#define IEEE_MC68k
#endif

#ifndef Long
#define Long int32
#endif

#ifndef ULong
#define ULong uint32
#endif











#ifdef JS_THREADSAFE
static PRLock *dtoalock;
static JSBool _dtoainited = JS_FALSE;

#define LOCK_DTOA() PR_Lock(dtoalock);
#define UNLOCK_DTOA() PR_Unlock(dtoalock)
#else
#define LOCK_DTOA()
#define UNLOCK_DTOA()
#endif
#include "dtoa.c"

JS_FRIEND_API(JSBool)
js_InitDtoa()
{
#ifdef JS_THREADSAFE
    if (!_dtoainited) {
        dtoalock = PR_NewLock();
        JS_ASSERT(dtoalock);
        _dtoainited = JS_TRUE;
    }

    return (dtoalock != 0);
#else
    return JS_TRUE;
#endif
}

JS_FRIEND_API(void)
js_FinishDtoa()
{
#ifdef JS_THREADSAFE
    if (_dtoainited) {
        PR_DestroyLock(dtoalock);
        dtoalock = NULL;
        _dtoainited = JS_FALSE;
    }
#endif
}


static const uint8 dtoaModes[] = {
    0,   
    0,   
    3,   
    2,   
    2};  

JS_FRIEND_API(double)
JS_strtod(const char *s00, char **se, int *err)
{
    double retval;
    if (err)
        *err = 0;
    LOCK_DTOA();
    retval = _strtod(s00, se);
    UNLOCK_DTOA();
    return retval;
}

JS_FRIEND_API(char *)
JS_dtostr(char *buffer, size_t bufferSize, JSDToStrMode mode, int precision, double dinput)
{
    U d;
    int decPt;        
    int sign;         
    int nDigits;      
    char *numBegin;   
    char *numEnd = 0; 

    JS_ASSERT(bufferSize >= (size_t)(mode <= DTOSTR_STANDARD_EXPONENTIAL
                                    ? DTOSTR_STANDARD_BUFFER_SIZE
                                    : DTOSTR_VARIABLE_BUFFER_SIZE(precision)));

    



    if (mode == DTOSTR_FIXED && (dinput >= 1e21 || dinput <= -1e21))
        mode = DTOSTR_STANDARD;

    LOCK_DTOA();
    dval(d) = dinput;
    numBegin = dtoa(d, dtoaModes[mode], precision, &decPt, &sign, &numEnd);
    if (!numBegin) {
        UNLOCK_DTOA();
        return NULL;
    }

    nDigits = numEnd - numBegin;
    JS_ASSERT((size_t) nDigits <= bufferSize - 2);
    if ((size_t) nDigits > bufferSize - 2) {
        UNLOCK_DTOA();
        return NULL;
    }

    memcpy(buffer + 2, numBegin, nDigits);
    freedtoa(numBegin);
    UNLOCK_DTOA();
    numBegin = buffer + 2; 
    numEnd = numBegin + nDigits;
    *numEnd = '\0';

    
    if (decPt != 9999) {
        JSBool exponentialNotation = JS_FALSE;
        int minNDigits = 0;  
        char *p;
        char *q;

        switch (mode) {
            case DTOSTR_STANDARD:
                if (decPt < -5 || decPt > 21)
                    exponentialNotation = JS_TRUE;
                else
                    minNDigits = decPt;
                break;

            case DTOSTR_FIXED:
                if (precision >= 0)
                    minNDigits = decPt + precision;
                else
                    minNDigits = decPt;
                break;

            case DTOSTR_EXPONENTIAL:
                JS_ASSERT(precision > 0);
                minNDigits = precision;
                
            case DTOSTR_STANDARD_EXPONENTIAL:
                exponentialNotation = JS_TRUE;
                break;

            case DTOSTR_PRECISION:
                JS_ASSERT(precision > 0);
                minNDigits = precision;
                if (decPt < -5 || decPt > precision)
                    exponentialNotation = JS_TRUE;
                break;
        }

        
        if (nDigits < minNDigits) {
            p = numBegin + minNDigits;
            nDigits = minNDigits;
            do {
                *numEnd++ = '0';
            } while (numEnd != p);
            *numEnd = '\0';
        }

        if (exponentialNotation) {
            
            if (nDigits != 1) {
                numBegin--;
                numBegin[0] = numBegin[1];
                numBegin[1] = '.';
            }
            JS_snprintf(numEnd, bufferSize - (numEnd - buffer), "e%+d", decPt-1);
        } else if (decPt != nDigits) {
            
            JS_ASSERT(decPt <= nDigits);
            if (decPt > 0) {
                
                p = --numBegin;
                do {
                    *p = p[1];
                    p++;
                } while (--decPt);
                *p = '.';
            } else {
                
                p = numEnd;
                numEnd += 1 - decPt;
                q = numEnd;
                JS_ASSERT(numEnd < buffer + bufferSize);
                *numEnd = '\0';
                while (p != numBegin)
                    *--q = *--p;
                for (p = numBegin + 1; p != q; p++)
                    *p = '0';
                *numBegin = '.';
                *--numBegin = '0';
            }
        }
    }

    
    if (sign &&
            !(word0(d) == Sign_bit && word1(d) == 0) &&
            !((word0(d) & Exp_mask) == Exp_mask &&
              (word1(d) || (word0(d) & Frac_mask)))) {
        *--numBegin = '-';
    }
    return numBegin;
}





static uint32
divrem(Bigint *b, uint32 divisor)
{
    int32 n = b->wds;
    uint32 remainder = 0;
    ULong *bx;
    ULong *bp;

    JS_ASSERT(divisor > 0 && divisor <= 65536);

    if (!n)
        return 0; 
    bx = b->x;
    bp = bx + n;
    do {
        ULong a = *--bp;
        ULong dividend = remainder << 16 | a >> 16;
        ULong quotientHi = dividend / divisor;
        ULong quotientLo;

        remainder = dividend - quotientHi*divisor;
        JS_ASSERT(quotientHi <= 0xFFFF && remainder < divisor);
        dividend = remainder << 16 | (a & 0xFFFF);
        quotientLo = dividend / divisor;
        remainder = dividend - quotientLo*divisor;
        JS_ASSERT(quotientLo <= 0xFFFF && remainder < divisor);
        *bp = quotientHi << 16 | quotientLo;
    } while (bp != bx);
    
    if (bx[n-1] == 0)
        b->wds--;
    return remainder;
}


static uint32 quorem2(Bigint *b, int32 k)
{
    ULong mask;
    ULong result;
    ULong *bx, *bxe;
    int32 w;
    int32 n = k >> 5;
    k &= 0x1F;
    mask = (1<<k) - 1;

    w = b->wds - n;
    if (w <= 0)
        return 0;
    JS_ASSERT(w <= 2);
    bx = b->x;
    bxe = bx + n;
    result = *bxe >> k;
    *bxe &= mask;
    if (w == 2) {
        JS_ASSERT(!(bxe[1] & ~mask));
        if (k)
            result |= bxe[1] << (32 - k);
    }
    n++;
    while (!*bxe && bxe != bx) {
        n--;
        bxe--;
    }
    b->wds = n;
    return result;
}





#define DTOBASESTR_BUFFER_SIZE 1078
#define BASEDIGIT(digit) ((char)(((digit) >= 10) ? 'a' - 10 + (digit) : '0' + (digit)))

JS_FRIEND_API(char *)
JS_dtobasestr(int base, double dinput)
{
    U d;
    char *buffer;        
    char *p;             
    char *pInt;          
    char *q;
    uint32 digit;
    U di;                
    U df;                

    JS_ASSERT(base >= 2 && base <= 36);

    dval(d) = dinput;
    buffer = (char*) malloc(DTOBASESTR_BUFFER_SIZE);
    if (buffer) {
        p = buffer;
        if (dval(d) < 0.0
#if defined(XP_WIN) || defined(XP_OS2)
            && !((word0(d) & Exp_mask) == Exp_mask && ((word0(d) & Frac_mask) || word1(d))) 
#endif
           ) {
            *p++ = '-';
            dval(d) = -dval(d);
        }

        
        if ((word0(d) & Exp_mask) == Exp_mask) {
            strcpy(p, !word1(d) && !(word0(d) & Frac_mask) ? "Infinity" : "NaN");
            return buffer;
        }

        LOCK_DTOA();
        
        pInt = p;
        dval(di) = floor(dval(d));
        if (dval(di) <= 4294967295.0) {
            uint32 n = (uint32)dval(di);
            if (n)
                do {
                    uint32 m = n / base;
                    digit = n - m*base;
                    n = m;
                    JS_ASSERT(digit < (uint32)base);
                    *p++ = BASEDIGIT(digit);
                } while (n);
            else *p++ = '0';
        } else {
            int e;
            int bits;  
            Bigint *b = d2b(di, &e, &bits);
            if (!b)
                goto nomem1;
            b = lshift(b, e);
            if (!b) {
              nomem1:
                Bfree(b);
                UNLOCK_DTOA();
                free(buffer);
                return NULL;
            }
            do {
                digit = divrem(b, base);
                JS_ASSERT(digit < (uint32)base);
                *p++ = BASEDIGIT(digit);
            } while (b->wds);
            Bfree(b);
        }
        
        q = p-1;
        while (q > pInt) {
            char ch = *pInt;
            *pInt++ = *q;
            *q-- = ch;
        }

        dval(df) = dval(d) - dval(di);
        if (dval(df) != 0.0) {
            
            int e, bbits;
            int32 s2, done;
            Bigint *b, *s, *mlo, *mhi;

            b = s = mlo = mhi = NULL;

            *p++ = '.';
            b = d2b(df, &e, &bbits);
            if (!b) {
              nomem2:
                Bfree(b);
                Bfree(s);
                if (mlo != mhi)
                    Bfree(mlo);
                Bfree(mhi);
                UNLOCK_DTOA();
                free(buffer);
                return NULL;
            }
            JS_ASSERT(e < 0);
            

            s2 = -(int32)(word0(d) >> Exp_shift1 & Exp_mask>>Exp_shift1);
#ifndef Sudden_Underflow
            if (!s2)
                s2 = -1;
#endif
            s2 += Bias + P;
            
            JS_ASSERT(-s2 < e);
            mlo = i2b(1);
            if (!mlo)
                goto nomem2;
            mhi = mlo;
            if (!word1(d) && !(word0(d) & Bndry_mask)
#ifndef Sudden_Underflow
                && word0(d) & (Exp_mask & Exp_mask << 1)
#endif
                ) {
                

                s2 += Log2P;
                mhi = i2b(1<<Log2P);
                if (!mhi)
                    goto nomem2;
            }
            b = lshift(b, e + s2);
            if (!b)
                goto nomem2;
            s = i2b(1);
            if (!s)
                goto nomem2;
            s = lshift(s, s2);
            if (!s)
                goto nomem2;
            





            done = JS_FALSE;
            do {
                int32 j, j1;
                Bigint *delta;

                b = multadd(b, base, 0);
                if (!b)
                    goto nomem2;
                digit = quorem2(b, s2);
                if (mlo == mhi) {
                    mlo = mhi = multadd(mlo, base, 0);
                    if (!mhi)
                        goto nomem2;
                }
                else {
                    mlo = multadd(mlo, base, 0);
                    if (!mlo)
                        goto nomem2;
                    mhi = multadd(mhi, base, 0);
                    if (!mhi)
                        goto nomem2;
                }

                
                j = cmp(b, mlo);
                
                delta = diff(s, mhi);
                if (!delta)
                    goto nomem2;
                j1 = delta->sign ? 1 : cmp(b, delta);
                Bfree(delta);
                

#ifndef ROUND_BIASED
                if (j1 == 0 && !(word1(d) & 1)) {
                    if (j > 0)
                        digit++;
                    done = JS_TRUE;
                } else
#endif
                if (j < 0 || (j == 0
#ifndef ROUND_BIASED
                    && !(word1(d) & 1)
#endif
                    )) {
                    if (j1 > 0) {
                        

                        b = lshift(b, 1);
                        if (!b)
                            goto nomem2;
                        j1 = cmp(b, s);
                        if (j1 > 0) 

                            digit++;
                    }
                    done = JS_TRUE;
                } else if (j1 > 0) {
                    digit++;
                    done = JS_TRUE;
                }
                JS_ASSERT(digit < (uint32)base);
                *p++ = BASEDIGIT(digit);
            } while (!done);
            Bfree(b);
            Bfree(s);
            if (mlo != mhi)
                Bfree(mlo);
            Bfree(mhi);
        }
        JS_ASSERT(p < buffer + DTOBASESTR_BUFFER_SIZE);
        *p = '\0';
        UNLOCK_DTOA();
    }
    return buffer;
}
