













































#ifndef jslong_h___
#define jslong_h___

#include "jstypes.h"

JS_BEGIN_EXTERN_C

#ifdef JS_HAVE_LONG_LONG

#if JS_BYTES_PER_LONG == 8
#define JSLL_INIT(hi, lo)  ((hi ## L << 32) + lo ## L)
#elif (defined(WIN32) || defined(WIN16)) && !defined(__GNUC__)
#define JSLL_INIT(hi, lo)  ((hi ## i64 << 32) + lo ## i64)
#else
#define JSLL_INIT(hi, lo)  ((hi ## LL << 32) + lo ## LL)
#endif


















#define JSLL_IS_ZERO(a)       ((a) == 0)
#define JSLL_EQ(a, b)         ((a) == (b))
#define JSLL_NE(a, b)         ((a) != (b))
#define JSLL_GE_ZERO(a)       ((a) >= 0)
#define JSLL_CMP(a, op, b)    ((JSInt64)(a) op (JSInt64)(b))
#define JSLL_UCMP(a, op, b)   ((JSUint64)(a) op (JSUint64)(b))










#define JSLL_AND(r, a, b)        ((r) = (a) & (b))
#define JSLL_OR(r, a, b)        ((r) = (a) | (b))
#define JSLL_XOR(r, a, b)        ((r) = (a) ^ (b))
#define JSLL_OR2(r, a)        ((r) = (r) | (a))
#define JSLL_NOT(r, a)        ((r) = ~(a))








#define JSLL_NEG(r, a)        ((r) = -(a))
#define JSLL_ADD(r, a, b)     ((r) = (a) + (b))
#define JSLL_SUB(r, a, b)     ((r) = (a) - (b))








#define JSLL_MUL(r, a, b)        ((r) = (a) * (b))
#define JSLL_DIV(r, a, b)        ((r) = (a) / (b))
#define JSLL_MOD(r, a, b)        ((r) = (a) % (b))









#define JSLL_SHL(r, a, b)     ((r) = (JSInt64)(a) << (b))
#define JSLL_SHR(r, a, b)     ((r) = (JSInt64)(a) >> (b))
#define JSLL_USHR(r, a, b)    ((r) = (JSUint64)(a) >> (b))
#define JSLL_ISHL(r, a, b)    ((r) = (JSInt64)(a) << (b))













#define JSLL_L2I(i, l)        ((i) = (JSInt32)(l))
#define JSLL_L2UI(ui, l)        ((ui) = (JSUint32)(l))
#define JSLL_L2F(f, l)        ((f) = (JSFloat64)(l))
#define JSLL_L2D(d, l)        ((d) = (JSFloat64)(l))

#define JSLL_I2L(l, i)        ((l) = (JSInt64)(i))
#define JSLL_UI2L(l, ui)        ((l) = (JSInt64)(ui))
#define JSLL_F2L(l, f)        ((l) = (JSInt64)(f))
#define JSLL_D2L(l, d)        ((l) = (JSInt64)(d))










#define JSLL_UDIVMOD(qp, rp, a, b) \
    (*(qp) = ((JSUint64)(a) / (b)), \
     *(rp) = ((JSUint64)(a) % (b)))

#else  

#ifdef IS_LITTLE_ENDIAN
#define JSLL_INIT(hi, lo) {JS_INT32(lo), JS_INT32(hi)}
#else
#define JSLL_INIT(hi, lo) {JS_INT32(hi), JS_INT32(lo)}
#endif

#define JSLL_IS_ZERO(a)         (((a).hi == 0) && ((a).lo == 0))
#define JSLL_EQ(a, b)           (((a).hi == (b).hi) && ((a).lo == (b).lo))
#define JSLL_NE(a, b)           (((a).hi != (b).hi) || ((a).lo != (b).lo))
#define JSLL_GE_ZERO(a)         (((a).hi >> 31) == 0)

#ifdef DEBUG
#define JSLL_CMP(a, op, b)      (JS_ASSERT((#op)[1] != '='), JSLL_REAL_CMP(a, op, b))
#define JSLL_UCMP(a, op, b)     (JS_ASSERT((#op)[1] != '='), JSLL_REAL_UCMP(a, op, b))
#else
#define JSLL_CMP(a, op, b)      JSLL_REAL_CMP(a, op, b)
#define JSLL_UCMP(a, op, b)     JSLL_REAL_UCMP(a, op, b)
#endif

#define JSLL_REAL_CMP(a,op,b)   (((JSInt32)(a).hi op (JSInt32)(b).hi) || \
                                 (((a).hi == (b).hi) && ((a).lo op (b).lo)))
#define JSLL_REAL_UCMP(a,op,b)  (((a).hi op (b).hi) || \
                                 (((a).hi == (b).hi) && ((a).lo op (b).lo)))

#define JSLL_AND(r, a, b)       ((r).lo = (a).lo & (b).lo, \
                                 (r).hi = (a).hi & (b).hi)
#define JSLL_OR(r, a, b)        ((r).lo = (a).lo | (b).lo, \
                                 (r).hi = (a).hi | (b).hi)
#define JSLL_XOR(r, a, b)       ((r).lo = (a).lo ^ (b).lo, \
                                 (r).hi = (a).hi ^ (b).hi)
#define JSLL_OR2(r, a)          ((r).lo = (r).lo | (a).lo, \
                                 (r).hi = (r).hi | (a).hi)
#define JSLL_NOT(r, a)          ((r).lo = ~(a).lo, \
                                 (r).hi = ~(a).hi)

#define JSLL_NEG(r, a)          ((r).lo = -(JSInt32)(a).lo, \
                                 (r).hi = -(JSInt32)(a).hi - ((r).lo != 0))
#define JSLL_ADD(r, a, b) { \
    JSInt64 _a, _b; \
    _a = a; _b = b; \
    (r).lo = _a.lo + _b.lo; \
    (r).hi = _a.hi + _b.hi + ((r).lo < _b.lo); \
}

#define JSLL_SUB(r, a, b) { \
    JSInt64 _a, _b; \
    _a = a; _b = b; \
    (r).lo = _a.lo - _b.lo; \
    (r).hi = _a.hi - _b.hi - (_a.lo < _b.lo); \
}

#define JSLL_MUL(r, a, b) { \
    JSInt64 _a, _b; \
    _a = a; _b = b; \
    JSLL_MUL32(r, _a.lo, _b.lo); \
    (r).hi += _a.hi * _b.lo + _a.lo * _b.hi; \
}

#define jslo16(a)        ((a) & JS_BITMASK(16))
#define jshi16(a)        ((a) >> 16)

#define JSLL_MUL32(r, a, b) { \
     JSUint32 _a1, _a0, _b1, _b0, _y0, _y1, _y2, _y3; \
     _a1 = jshi16(a), _a0 = jslo16(a); \
     _b1 = jshi16(b), _b0 = jslo16(b); \
     _y0 = _a0 * _b0; \
     _y1 = _a0 * _b1; \
     _y2 = _a1 * _b0; \
     _y3 = _a1 * _b1; \
     _y1 += jshi16(_y0);                         /* can't carry */ \
     _y1 += _y2;                                /* might carry */ \
     if (_y1 < _y2)    \
        _y3 += (JSUint32)(JS_BIT(16));  /* propagate */ \
     (r).lo = (jslo16(_y1) << 16) + jslo16(_y0); \
     (r).hi = _y3 + jshi16(_y1); \
}

#define JSLL_UDIVMOD(qp, rp, a, b)    jsll_udivmod(qp, rp, a, b)

extern JS_PUBLIC_API(void) jsll_udivmod(JSUint64 *qp, JSUint64 *rp, JSUint64 a, JSUint64 b);

#define JSLL_DIV(r, a, b) { \
    JSInt64 _a, _b; \
    JSUint32 _negative = (JSInt32)(a).hi < 0; \
    if (_negative) { \
    JSLL_NEG(_a, a); \
    } else { \
    _a = a; \
    } \
    if ((JSInt32)(b).hi < 0) { \
    _negative ^= 1; \
    JSLL_NEG(_b, b); \
    } else { \
    _b = b; \
    } \
    JSLL_UDIVMOD(&(r), 0, _a, _b); \
    if (_negative) \
    JSLL_NEG(r, r); \
}

#define JSLL_MOD(r, a, b) { \
    JSInt64 _a, _b; \
    JSUint32 _negative = (JSInt32)(a).hi < 0; \
    if (_negative) { \
    JSLL_NEG(_a, a); \
    } else { \
    _a = a; \
    } \
    if ((JSInt32)(b).hi < 0) { \
    JSLL_NEG(_b, b); \
    } else { \
    _b = b; \
    } \
    JSLL_UDIVMOD(0, &(r), _a, _b); \
    if (_negative) \
    JSLL_NEG(r, r); \
}

#define JSLL_SHL(r, a, b) { \
    if (b) { \
    JSInt64 _a; \
        _a = a; \
        if ((b) < 32) { \
        (r).lo = _a.lo << ((b) & 31); \
        (r).hi = (_a.hi << ((b) & 31)) | (_a.lo >> (32 - (b))); \
    } else { \
        (r).lo = 0; \
        (r).hi = _a.lo << ((b) & 31); \
    } \
    } else { \
    (r) = (a); \
    } \
}


#define JSLL_ISHL(r, a, b) { \
    if (b) { \
    JSInt64 _a; \
    _a.lo = (a); \
    _a.hi = 0; \
        if ((b) < 32) { \
        (r).lo = (a) << ((b) & 31); \
        (r).hi = ((a) >> (32 - (b))); \
    } else { \
        (r).lo = 0; \
        (r).hi = (a) << ((b) & 31); \
    } \
    } else { \
    (r).lo = (a); \
    (r).hi = 0; \
    } \
}

#define JSLL_SHR(r, a, b) { \
    if (b) { \
    JSInt64 _a; \
        _a = a; \
    if ((b) < 32) { \
        (r).lo = (_a.hi << (32 - (b))) | (_a.lo >> ((b) & 31)); \
        (r).hi = (JSInt32)_a.hi >> ((b) & 31); \
    } else { \
        (r).lo = (JSInt32)_a.hi >> ((b) & 31); \
        (r).hi = (JSInt32)_a.hi >> 31; \
    } \
    } else { \
    (r) = (a); \
    } \
}

#define JSLL_USHR(r, a, b) { \
    if (b) { \
    JSInt64 _a; \
        _a = a; \
    if ((b) < 32) { \
        (r).lo = (_a.hi << (32 - (b))) | (_a.lo >> ((b) & 31)); \
        (r).hi = _a.hi >> ((b) & 31); \
    } else { \
        (r).lo = _a.hi >> ((b) & 31); \
        (r).hi = 0; \
    } \
    } else { \
    (r) = (a); \
    } \
}

#define JSLL_L2I(i, l)        ((i) = (l).lo)
#define JSLL_L2UI(ui, l)        ((ui) = (l).lo)
#define JSLL_L2F(f, l)        { double _d; JSLL_L2D(_d, l); (f) = (JSFloat64)_d; }

#define JSLL_L2D(d, l) { \
    int _negative; \
    JSInt64 _absval; \
 \
    _negative = (l).hi >> 31; \
    if (_negative) { \
    JSLL_NEG(_absval, l); \
    } else { \
    _absval = l; \
    } \
    (d) = (double)_absval.hi * 4.294967296e9 + _absval.lo; \
    if (_negative) \
    (d) = -(d); \
}

#define JSLL_I2L(l, i)        { JSInt32 _i = (i) >> 31; (l).lo = (i); (l).hi = _i; }
#define JSLL_UI2L(l, ui)      ((l).lo = (ui), (l).hi = 0)
#define JSLL_F2L(l, f)        { double _d = (double)f; JSLL_D2L(l, _d); }

#define JSLL_D2L(l, d) { \
    int _negative; \
    double _absval, _d_hi; \
    JSInt64 _lo_d; \
 \
    _negative = ((d) < 0); \
    _absval = _negative ? -(d) : (d); \
 \
    (l).hi = _absval / 4.294967296e9; \
    (l).lo = 0; \
    JSLL_L2D(_d_hi, l); \
    _absval -= _d_hi; \
    _lo_d.hi = 0; \
    if (_absval < 0) { \
    _lo_d.lo = -_absval; \
    JSLL_SUB(l, l, _lo_d); \
    } else { \
    _lo_d.lo = _absval; \
    JSLL_ADD(l, l, _lo_d); \
    } \
 \
    if (_negative) \
    JSLL_NEG(l, l); \
}

#endif 

JS_END_EXTERN_C

#endif 
