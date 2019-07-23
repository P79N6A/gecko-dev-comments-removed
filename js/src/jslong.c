





































#include "jsstddef.h"
#include "jstypes.h"
#include "jslong.h"

static JSInt64 ll_zero = JSLL_INIT( 0x00000000,0x00000000 );
static JSInt64 ll_maxint = JSLL_INIT( 0x7fffffff, 0xffffffff );
static JSInt64 ll_minint = JSLL_INIT( 0x80000000, 0x00000000 );

#ifdef HAVE_WATCOM_BUG_2
JSInt64 __pascal __loadds __export
    JSLL_Zero(void) { return ll_zero; }
JSInt64 __pascal __loadds __export
    JSLL_MaxInt(void) { return ll_maxint; }
JSInt64 __pascal __loadds __export
    JSLL_MinInt(void) { return ll_minint; }
#else
JS_PUBLIC_API(JSInt64) JSLL_Zero(void) { return ll_zero; }
JS_PUBLIC_API(JSInt64) JSLL_MaxInt(void) { return ll_maxint; }
JS_PUBLIC_API(JSInt64) JSLL_MinInt(void) { return ll_minint; }
#endif

#ifndef JS_HAVE_LONG_LONG



static void norm_udivmod32(JSUint32 *qp, JSUint32 *rp, JSUint64 a, JSUint32 b)
{
    JSUint32 d1, d0, q1, q0;
    JSUint32 r1, r0, m;

    d1 = jshi16(b);
    d0 = jslo16(b);
    r1 = a.hi % d1;
    q1 = a.hi / d1;
    m = q1 * d0;
    r1 = (r1 << 16) | jshi16(a.lo);
    if (r1 < m) {
        q1--, r1 += b;
        if (r1 >= b     
            && r1 < m) {
            q1--, r1 += b;
        }
    }
    r1 -= m;
    r0 = r1 % d1;
    q0 = r1 / d1;
    m = q0 * d0;
    r0 = (r0 << 16) | jslo16(a.lo);
    if (r0 < m) {
        q0--, r0 += b;
        if (r0 >= b
            && r0 < m) {
            q0--, r0 += b;
        }
    }
    *qp = (q1 << 16) | q0;
    *rp = r0 - m;
}

static JSUint32 CountLeadingZeros(JSUint32 a)
{
    JSUint32 t;
    JSUint32 r = 32;

    if ((t = a >> 16) != 0)
        r -= 16, a = t;
    if ((t = a >> 8) != 0)
        r -= 8, a = t;
    if ((t = a >> 4) != 0)
        r -= 4, a = t;
    if ((t = a >> 2) != 0)
        r -= 2, a = t;
    if ((t = a >> 1) != 0)
        r -= 1, a = t;
    if (a & 1)
        r--;
    return r;
}

JS_PUBLIC_API(void) jsll_udivmod(JSUint64 *qp, JSUint64 *rp, JSUint64 a, JSUint64 b)
{
    JSUint32 n0, n1, n2;
    JSUint32 q0, q1;
    JSUint32 rsh, lsh;

    n0 = a.lo;
    n1 = a.hi;

    if (b.hi == 0) {
        if (b.lo > n1) {
            

            lsh = CountLeadingZeros(b.lo);

            if (lsh) {
                



                b.lo = b.lo << lsh;
                n1 = (n1 << lsh) | (n0 >> (32 - lsh));
                n0 = n0 << lsh;
            }

            a.lo = n0, a.hi = n1;
            norm_udivmod32(&q0, &n0, a, b.lo);
            q1 = 0;

            
        } else {
            

            if (b.lo == 0)              
                b.lo = 1 / b.lo;        

            lsh = CountLeadingZeros(b.lo);

            if (lsh == 0) {
                









                n1 -= b.lo;
                q1 = 1;
            } else {
                


                rsh = 32 - lsh;

                b.lo = b.lo << lsh;
                n2 = n1 >> rsh;
                n1 = (n1 << lsh) | (n0 >> rsh);
                n0 = n0 << lsh;

                a.lo = n1, a.hi = n2;
                norm_udivmod32(&q1, &n1, a, b.lo);
            }

            

            a.lo = n0, a.hi = n1;
            norm_udivmod32(&q0, &n0, a, b.lo);

            
        }

        if (rp) {
            rp->lo = n0 >> lsh;
            rp->hi = 0;
        }
    } else {
        if (b.hi > n1) {
            

            q0 = 0;
            q1 = 0;

            
            if (rp) {
                rp->lo = n0;
                rp->hi = n1;
            }
        } else {
            

            lsh = CountLeadingZeros(b.hi);
            if (lsh == 0) {
                









                



                if (n1 > b.hi || n0 >= b.lo) {
                    q0 = 1;
                    a.lo = n0, a.hi = n1;
                    JSLL_SUB(a, a, b);
                } else {
                    q0 = 0;
                }
                q1 = 0;

                if (rp) {
                    rp->lo = n0;
                    rp->hi = n1;
                }
            } else {
                JSInt64 m;

                


                rsh = 32 - lsh;

                b.hi = (b.hi << lsh) | (b.lo >> rsh);
                b.lo = b.lo << lsh;
                n2 = n1 >> rsh;
                n1 = (n1 << lsh) | (n0 >> rsh);
                n0 = n0 << lsh;

                a.lo = n1, a.hi = n2;
                norm_udivmod32(&q0, &n1, a, b.hi);
                JSLL_MUL32(m, q0, b.lo);

                if ((m.hi > n1) || ((m.hi == n1) && (m.lo > n0))) {
                    q0--;
                    JSLL_SUB(m, m, b);
                }

                q1 = 0;

                
                if (rp) {
                    a.lo = n0, a.hi = n1;
                    JSLL_SUB(a, a, m);
                    rp->lo = (a.hi << rsh) | (a.lo >> lsh);
                    rp->hi = a.hi >> lsh;
                }
            }
        }
    }

    if (qp) {
        qp->lo = q0;
        qp->hi = q1;
    }
}
#endif 
