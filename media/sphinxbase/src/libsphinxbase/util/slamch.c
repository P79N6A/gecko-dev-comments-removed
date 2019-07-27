











#include "sphinxbase/f2c.h"

#ifdef _MSC_VER
#pragma warning (disable: 4244)
#endif



static integer c__1 = 1;
static real c_b32 = 0.f;

doublereal
slamch_(char *cmach, ftnlen cmach_len)
{
    

    static logical first = TRUE_;

    
    integer i__1;
    real ret_val;

    
    double pow_ri(real *, integer *);

    
    static real t;
    static integer it;
    static real rnd, eps, base;
    static integer beta;
    static real emin, prec, emax;
    static integer imin, imax;
    static logical lrnd;
    static real rmin, rmax, rmach;
    extern logical lsame_(char *, char *, ftnlen, ftnlen);
    static real small, sfmin;
    extern  int slamc2_(integer *, integer *, logical *, real
                                        *, integer *, real *, integer *,
                                        real *);




























































    if (first) {
        first = FALSE_;
        slamc2_(&beta, &it, &lrnd, &eps, &imin, &rmin, &imax, &rmax);
        base = (real) beta;
        t = (real) it;
        if (lrnd) {
            rnd = 1.f;
            i__1 = 1 - it;
            eps = pow_ri(&base, &i__1) / 2;
        }
        else {
            rnd = 0.f;
            i__1 = 1 - it;
            eps = pow_ri(&base, &i__1);
        }
        prec = eps * base;
        emin = (real) imin;
        emax = (real) imax;
        sfmin = rmin;
        small = 1.f / rmax;
        if (small >= sfmin) {




            sfmin = small * (eps + 1.f);
        }
    }

    if (lsame_(cmach, "E", (ftnlen) 1, (ftnlen) 1)) {
        rmach = eps;
    }
    else if (lsame_(cmach, "S", (ftnlen) 1, (ftnlen) 1)) {
        rmach = sfmin;
    }
    else if (lsame_(cmach, "B", (ftnlen) 1, (ftnlen) 1)) {
        rmach = base;
    }
    else if (lsame_(cmach, "P", (ftnlen) 1, (ftnlen) 1)) {
        rmach = prec;
    }
    else if (lsame_(cmach, "N", (ftnlen) 1, (ftnlen) 1)) {
        rmach = t;
    }
    else if (lsame_(cmach, "R", (ftnlen) 1, (ftnlen) 1)) {
        rmach = rnd;
    }
    else if (lsame_(cmach, "M", (ftnlen) 1, (ftnlen) 1)) {
        rmach = emin;
    }
    else if (lsame_(cmach, "U", (ftnlen) 1, (ftnlen) 1)) {
        rmach = rmin;
    }
    else if (lsame_(cmach, "L", (ftnlen) 1, (ftnlen) 1)) {
        rmach = emax;
    }
    else if (lsame_(cmach, "O", (ftnlen) 1, (ftnlen) 1)) {
        rmach = rmax;
    }

    ret_val = rmach;
    return ret_val;



}                               




 int
slamc1_(integer * beta, integer * t, logical * rnd, logical * ieee1)
{
    

    static logical first = TRUE_;

    
    real r__1, r__2;

    
    static real a, b, c__, f, t1, t2;
    static integer lt;
    static real one, qtr;
    static logical lrnd;
    static integer lbeta;
    static real savec;
    static logical lieee1;
    extern doublereal slamc3_(real *, real *);




























































    if (first) {
        first = FALSE_;
        one = 1.f;













        a = 1.f;
        c__ = 1.f;


      L10:
        if (c__ == one) {
            a *= 2;
            c__ = slamc3_(&a, &one);
            r__1 = -a;
            c__ = slamc3_(&c__, &r__1);
            goto L10;
        }







        b = 1.f;
        c__ = slamc3_(&a, &b);


      L20:
        if (c__ == a) {
            b *= 2;
            c__ = slamc3_(&a, &b);
            goto L20;
        }







        qtr = one / 4;
        savec = c__;
        r__1 = -a;
        c__ = slamc3_(&c__, &r__1);
        lbeta = c__ + qtr;




        b = (real) lbeta;
        r__1 = b / 2;
        r__2 = -b / 100;
        f = slamc3_(&r__1, &r__2);
        c__ = slamc3_(&f, &a);
        if (c__ == a) {
            lrnd = TRUE_;
        }
        else {
            lrnd = FALSE_;
        }
        r__1 = b / 2;
        r__2 = b / 100;
        f = slamc3_(&r__1, &r__2);
        c__ = slamc3_(&f, &a);
        if (lrnd && c__ == a) {
            lrnd = FALSE_;
        }







        r__1 = b / 2;
        t1 = slamc3_(&r__1, &a);
        r__1 = b / 2;
        t2 = slamc3_(&r__1, &savec);
        lieee1 = t1 == a && t2 > savec && lrnd;








        lt = 0;
        a = 1.f;
        c__ = 1.f;


      L30:
        if (c__ == one) {
            ++lt;
            a *= lbeta;
            c__ = slamc3_(&a, &one);
            r__1 = -a;
            c__ = slamc3_(&c__, &r__1);
            goto L30;
        }


    }

    *beta = lbeta;
    *t = lt;
    *rnd = lrnd;
    *ieee1 = lieee1;
    return 0;



}                               




 int
slamc2_(integer * beta, integer * t, logical * rnd, real *
        eps, integer * emin, real * rmin, integer * emax, real * rmax)
{
    

    static logical first = TRUE_;
    static logical iwarn = FALSE_;

    
    static char fmt_9999[] =
        "(//\002 WARNING. The value EMIN may be incorre"
        "ct:-\002,\002  EMIN = \002,i8,/\002 If, after inspection, the va"
        "lue EMIN looks\002,\002 acceptable please comment out \002,/\002"
        " the IF block as marked within the code of routine\002,\002 SLAM"
        "C2,\002,/\002 otherwise supply EMIN explicitly.\002,/)";

    
    integer i__1;
    real r__1, r__2, r__3, r__4, r__5;

    
    double pow_ri(real *, integer *);
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen),
        e_wsfe(void);

    
    static real a, b, c__;
    static integer i__, lt;
    static real one, two;
    static logical ieee;
    static real half;
    static logical lrnd;
    static real leps, zero;
    static integer lbeta;
    static real rbase;
    static integer lemin, lemax, gnmin;
    static real small;
    static integer gpmin;
    static real third, lrmin, lrmax, sixth;
    static logical lieee1;
    extern  int slamc1_(integer *, integer *, logical *,
                                        logical *);
    extern doublereal slamc3_(real *, real *);
    extern  int slamc4_(integer *, real *, integer *),
        slamc5_(integer *, integer *, integer *, logical *, integer *,
                real *);
    static integer ngnmin, ngpmin;

    
    static cilist io___58 = { 0, 6, 0, fmt_9999, 0 };













































































    if (first) {
        first = FALSE_;
        zero = 0.f;
        one = 1.f;
        two = 2.f;










        slamc1_(&lbeta, &lt, &lrnd, &lieee1);



        b = (real) lbeta;
        i__1 = -lt;
        a = pow_ri(&b, &i__1);
        leps = a;



        b = two / 3;
        half = one / 2;
        r__1 = -half;
        sixth = slamc3_(&b, &r__1);
        third = slamc3_(&sixth, &sixth);
        r__1 = -half;
        b = slamc3_(&third, &r__1);
        b = slamc3_(&b, &sixth);
        b = dabs(b);
        if (b < leps) {
            b = leps;
        }

        leps = 1.f;


      L10:
        if (leps > b && b > zero) {
            leps = b;
            r__1 = half * leps;

            r__3 = two, r__4 = r__3, r__3 *= r__3;

            r__5 = leps;
            r__2 = r__4 * (r__3 * r__3) * (r__5 * r__5);
            c__ = slamc3_(&r__1, &r__2);
            r__1 = -c__;
            c__ = slamc3_(&half, &r__1);
            b = slamc3_(&half, &c__);
            r__1 = -b;
            c__ = slamc3_(&half, &r__1);
            b = slamc3_(&half, &c__);
            goto L10;
        }


        if (a < leps) {
            leps = a;
        }







        rbase = one / lbeta;
        small = one;
        for (i__ = 1; i__ <= 3; ++i__) {
            r__1 = small * rbase;
            small = slamc3_(&r__1, &zero);

        }
        a = slamc3_(&one, &small);
        slamc4_(&ngpmin, &one, &lbeta);
        r__1 = -one;
        slamc4_(&ngnmin, &r__1, &lbeta);
        slamc4_(&gpmin, &a, &lbeta);
        r__1 = -a;
        slamc4_(&gnmin, &r__1, &lbeta);
        ieee = FALSE_;

        if (ngpmin == ngnmin && gpmin == gnmin) {
            if (ngpmin == gpmin) {
                lemin = ngpmin;


            }
            else if (gpmin - ngpmin == 3) {
                lemin = ngpmin - 1 + lt;
                ieee = TRUE_;


            }
            else {
                lemin = min(ngpmin, gpmin);

                iwarn = TRUE_;
            }

        }
        else if (ngpmin == gpmin && ngnmin == gnmin) {
            if ((i__1 = ngpmin - ngnmin, abs(i__1)) == 1) {
                lemin = max(ngpmin, ngnmin);


            }
            else {
                lemin = min(ngpmin, ngnmin);

                iwarn = TRUE_;
            }

        }
        else if ((i__1 = ngpmin - ngnmin, abs(i__1)) == 1
                 && gpmin == gnmin) {
            if (gpmin - min(ngpmin, ngnmin) == 3) {
                lemin = max(ngpmin, ngnmin) - 1 + lt;


            }
            else {
                lemin = min(ngpmin, ngnmin);

                iwarn = TRUE_;
            }

        }
        else {

            i__1 = min(ngpmin, ngnmin), i__1 = min(i__1, gpmin);
            lemin = min(i__1, gnmin);

            iwarn = TRUE_;
        }


        if (iwarn) {
            first = TRUE_;
            s_wsfe(&io___58);
            do_fio(&c__1, (char *) &lemin, (ftnlen) sizeof(integer));
            e_wsfe();
        }







        ieee = ieee || lieee1;





        lrmin = 1.f;
        i__1 = 1 - lemin;
        for (i__ = 1; i__ <= i__1; ++i__) {
            r__1 = lrmin * rbase;
            lrmin = slamc3_(&r__1, &zero);

        }



        slamc5_(&lbeta, &lt, &lemin, &ieee, &lemax, &lrmax);
    }

    *beta = lbeta;
    *t = lt;
    *rnd = lrnd;
    *eps = leps;
    *emin = lemin;
    *rmin = lrmin;
    *emax = lemax;
    *rmax = lrmax;

    return 0;




}                               




doublereal
slamc3_(real * a, real * b)
{
    
    real ret_val;



























    ret_val = *a + *b;

    return ret_val;



}                               




 int
slamc4_(integer * emin, real * start, integer * base)
{
    
    integer i__1;
    real r__1;

    
    static real a;
    static integer i__;
    static real b1, b2, c1, c2, d1, d2, one, zero, rbase;
    extern doublereal slamc3_(real *, real *);





































    a = *start;
    one = 1.f;
    rbase = one / *base;
    zero = 0.f;
    *emin = 1;
    r__1 = a * rbase;
    b1 = slamc3_(&r__1, &zero);
    c1 = a;
    c2 = a;
    d1 = a;
    d2 = a;


  L10:
    if (c1 == a && c2 == a && d1 == a && d2 == a) {
        --(*emin);
        a = b1;
        r__1 = a / *base;
        b1 = slamc3_(&r__1, &zero);
        r__1 = b1 * *base;
        c1 = slamc3_(&r__1, &zero);
        d1 = zero;
        i__1 = *base;
        for (i__ = 1; i__ <= i__1; ++i__) {
            d1 += b1;

        }
        r__1 = a * rbase;
        b2 = slamc3_(&r__1, &zero);
        r__1 = b2 / rbase;
        c2 = slamc3_(&r__1, &zero);
        d2 = zero;
        i__1 = *base;
        for (i__ = 1; i__ <= i__1; ++i__) {
            d2 += b2;

        }
        goto L10;
    }


    return 0;



}                               




 int
slamc5_(integer * beta, integer * p, integer * emin,
        logical * ieee, integer * emax, real * rmax)
{
    
    integer i__1;
    real r__1;

    
    static integer i__;
    static real y, z__;
    static integer try__, lexp;
    static real oldy;
    static integer uexp, nbits;
    extern doublereal slamc3_(real *, real *);
    static real recbas;
    static integer exbits, expsum;




























































    lexp = 1;
    exbits = 1;
  L10:
    try__ = lexp << 1;
    if (try__ <= -(*emin)) {
        lexp = try__;
        ++exbits;
        goto L10;
    }
    if (lexp == -(*emin)) {
        uexp = lexp;
    }
    else {
        uexp = try__;
        ++exbits;
    }





    if (uexp + *emin > -lexp - *emin) {
        expsum = lexp << 1;
    }
    else {
        expsum = uexp << 1;
    }




    *emax = expsum + *emin - 1;
    nbits = exbits + 1 + *p;




    if (nbits % 2 == 1 && *beta == 2) {












        --(*emax);
    }

    if (*ieee) {




        --(*emax);
    }







    recbas = 1.f / *beta;
    z__ = *beta - 1.f;
    y = 0.f;
    i__1 = *p;
    for (i__ = 1; i__ <= i__1; ++i__) {
        z__ *= recbas;
        if (y < 1.f) {
            oldy = y;
        }
        y = slamc3_(&y, &z__);

    }
    if (y >= 1.f) {
        y = oldy;
    }



    i__1 = *emax;
    for (i__ = 1; i__ <= i__1; ++i__) {
        r__1 = y * *beta;
        y = slamc3_(&r__1, &c_b32);

    }

    *rmax = y;
    return 0;



}                               
