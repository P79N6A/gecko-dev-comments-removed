













































#ifndef jslong_h___
#define jslong_h___

#include "jstypes.h"

JS_BEGIN_EXTERN_C

#define JSLL_INIT(hi, lo)  (((JSInt64)(hi)) << 32 + (JSInt64)(lo))


















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

JS_END_EXTERN_C

#endif 
