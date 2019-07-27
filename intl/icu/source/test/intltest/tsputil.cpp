





#include "tsputil.h"

#include <float.h> 
#include "putilimp.h"

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void 
PUtilTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    
    switch (index) {
        CASE(0, testMaxMin)
        CASE(1, testNaN)
        CASE(2, testPositiveInfinity)
        CASE(3, testNegativeInfinity)
        CASE(4, testZero)


        default: name = ""; break; 
    }
}

#if 0
void
PUtilTest::testIEEEremainder()
{
    double    pinf  = uprv_getInfinity();
    double    ninf  = -uprv_getInfinity();
    double    nan   = uprv_getNaN();
    double    pzero = 0.0;
    double    nzero = 0.0;

    nzero *= -1;

    
    remainderTest(7.0, 2.5, -0.5);
    remainderTest(7.0, -2.5, -0.5);
#if U_PLATFORM != U_PF_OS390
    
    
    
    
    remainderTest(-7.0, 2.5, 0.5);
    remainderTest(-7.0, -2.5, 0.5);
#endif
    remainderTest(5.0, 3.0, -1.0);
    
    
    

    


















}

void
PUtilTest::remainderTest(double x, double y, double exp)
{
    double result = uprv_IEEEremainder(x,y);

    if(        uprv_isNaN(result) && 
        ! ( uprv_isNaN(x) || uprv_isNaN(y))) {
        errln(UnicodeString("FAIL: got NaN as result without NaN as argument"));
        errln(UnicodeString("      IEEEremainder(") + x + ", " + y + ") is " + result + ", expected " + exp);
    }
    else if(result != exp)
        errln(UnicodeString("FAIL: IEEEremainder(") + x + ", " + y + ") is " + result + ", expected " + exp);
    else
        logln(UnicodeString("OK: IEEEremainder(") + x + ", " + y + ") is " + result);

}
#endif

void
PUtilTest::testMaxMin()
{
    double    pinf        = uprv_getInfinity();
    double    ninf        = -uprv_getInfinity();
    double    nan        = uprv_getNaN();
    double    pzero        = 0.0;
    double    nzero        = 0.0;

    nzero *= -1;

    
    maxMinTest(pinf, ninf, pinf, TRUE);
    maxMinTest(pinf, ninf, ninf, FALSE);

    
    maxMinTest(pinf, pzero, pinf, TRUE);
    maxMinTest(pinf, pzero, pzero, FALSE);
    maxMinTest(pinf, nzero, pinf, TRUE);
    maxMinTest(pinf, nzero, nzero, FALSE);

    
    maxMinTest(ninf, pzero, pzero, TRUE);
    maxMinTest(ninf, pzero, ninf, FALSE);
    maxMinTest(ninf, nzero, nzero, TRUE);
    maxMinTest(ninf, nzero, ninf, FALSE);

    
    maxMinTest(pinf, nan, nan, TRUE);
    maxMinTest(pinf, nan, nan, FALSE);
    maxMinTest(ninf, nan, nan, TRUE);
    maxMinTest(ninf, nan, nan, FALSE);

    
    maxMinTest(nan, nan, nan, TRUE);
    maxMinTest(nan, nan, nan, FALSE);

    
    maxMinTest(nan, pzero, nan, TRUE);
    maxMinTest(nan, pzero, nan, FALSE);
    maxMinTest(nan, nzero, nan, TRUE);
    maxMinTest(nan, nzero, nan, FALSE);

    
    maxMinTest(pinf, DBL_MAX, pinf, TRUE);
    maxMinTest(pinf, -DBL_MAX, pinf, TRUE);
    maxMinTest(pinf, DBL_MIN, pinf, TRUE);
    maxMinTest(pinf, -DBL_MIN, pinf, TRUE);
    maxMinTest(pinf, DBL_MIN, DBL_MIN, FALSE);
    maxMinTest(pinf, -DBL_MIN, -DBL_MIN, FALSE);
    maxMinTest(pinf, DBL_MAX, DBL_MAX, FALSE);
    maxMinTest(pinf, -DBL_MAX, -DBL_MAX, FALSE);

    
    maxMinTest(ninf, DBL_MAX, DBL_MAX, TRUE);
    maxMinTest(ninf, -DBL_MAX, -DBL_MAX, TRUE);
    maxMinTest(ninf, DBL_MIN, DBL_MIN, TRUE);
    maxMinTest(ninf, -DBL_MIN, -DBL_MIN, TRUE);
    maxMinTest(ninf, DBL_MIN, ninf, FALSE);
    maxMinTest(ninf, -DBL_MIN, ninf, FALSE);
    maxMinTest(ninf, DBL_MAX, ninf, FALSE);
    maxMinTest(ninf, -DBL_MAX, ninf, FALSE);

    
    maxMinTest(pzero, DBL_MAX, DBL_MAX, TRUE);
    maxMinTest(pzero, -DBL_MAX, pzero, TRUE);
    maxMinTest(pzero, DBL_MIN, DBL_MIN, TRUE);
    maxMinTest(pzero, -DBL_MIN, pzero, TRUE);
    maxMinTest(pzero, DBL_MIN, pzero, FALSE);
    maxMinTest(pzero, -DBL_MIN, -DBL_MIN, FALSE);
    maxMinTest(pzero, DBL_MAX, pzero, FALSE);
    maxMinTest(pzero, -DBL_MAX, -DBL_MAX, FALSE);

    
    maxMinTest(nzero, DBL_MAX, DBL_MAX, TRUE);
    maxMinTest(nzero, -DBL_MAX, nzero, TRUE);
    maxMinTest(nzero, DBL_MIN, DBL_MIN, TRUE);
    maxMinTest(nzero, -DBL_MIN, nzero, TRUE);
    maxMinTest(nzero, DBL_MIN, nzero, FALSE);
    maxMinTest(nzero, -DBL_MIN, -DBL_MIN, FALSE);
    maxMinTest(nzero, DBL_MAX, nzero, FALSE);
    maxMinTest(nzero, -DBL_MAX, -DBL_MAX, FALSE);
}

void
PUtilTest::maxMinTest(double a, double b, double exp, UBool max)
{
    double result = 0.0;

    if(max)
        result = uprv_fmax(a, b);
    else
        result = uprv_fmin(a, b);

    UBool nanResultOK = (uprv_isNaN(a) || uprv_isNaN(b));

    if(uprv_isNaN(result) && ! nanResultOK) {
        errln(UnicodeString("FAIL: got NaN as result without NaN as argument"));
        if(max)
            errln(UnicodeString("      max(") + a + ", " + b + ") is " + result + ", expected " + exp);
        else
            errln(UnicodeString("      min(") + a + ", " + b + ") is " + result + ", expected " + exp);
    }
    else if(result != exp && ! (uprv_isNaN(result) || uprv_isNaN(exp)))
        if(max)
            errln(UnicodeString("FAIL: max(") + a + ", " + b + ") is " + result + ", expected " + exp);
        else
            errln(UnicodeString("FAIL: min(") + a + ", " + b + ") is " + result + ", expected " + exp);
    else {
        if (verbose) {
            if(max)
                logln(UnicodeString("OK: max(") + a + ", " + b + ") is " + result);
            else
                logln(UnicodeString("OK: min(") + a + ", " + b + ") is " + result);
        }
    }
}




void
PUtilTest::testNaN(void)
{
    logln("NaN tests may show that the expected NaN!=NaN etc. is not true on some");
    logln("platforms; however, ICU does not rely on them because it defines");
    logln("and uses uprv_isNaN(). Therefore, most failing NaN tests only report warnings.");

    PUtilTest::testIsNaN();
    PUtilTest::NaNGT();
    PUtilTest::NaNLT();
    PUtilTest::NaNGTE();
    PUtilTest::NaNLTE();
    PUtilTest::NaNE();
    PUtilTest::NaNNE();

    logln("End of NaN tests.");
}



void 
PUtilTest::testPositiveInfinity(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  ten     = 10.0;

    if(uprv_isInfinite(pinf) != TRUE) {
        errln("FAIL: isInfinite(+Infinity) returned FALSE, should be TRUE.");
    }

    if(uprv_isPositiveInfinity(pinf) != TRUE) {
        errln("FAIL: isPositiveInfinity(+Infinity) returned FALSE, should be TRUE.");
    }

    if(uprv_isNegativeInfinity(pinf) != FALSE) {
        errln("FAIL: isNegativeInfinity(+Infinity) returned TRUE, should be FALSE.");
    }

    if((pinf > DBL_MAX) != TRUE) {
        errln("FAIL: +Infinity > DBL_MAX returned FALSE, should be TRUE.");
    }

    if((pinf > DBL_MIN) != TRUE) {
        errln("FAIL: +Infinity > DBL_MIN returned FALSE, should be TRUE.");
    }

    if((pinf > ninf) != TRUE) {
        errln("FAIL: +Infinity > -Infinity returned FALSE, should be TRUE.");
    }

    if((pinf > ten) != TRUE) {
        errln("FAIL: +Infinity > 10.0 returned FALSE, should be TRUE.");
    }
}



void           
PUtilTest::testNegativeInfinity(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  ten     = 10.0;

    if(uprv_isInfinite(ninf) != TRUE) {
        errln("FAIL: isInfinite(-Infinity) returned FALSE, should be TRUE.");
    }

    if(uprv_isNegativeInfinity(ninf) != TRUE) {
        errln("FAIL: isNegativeInfinity(-Infinity) returned FALSE, should be TRUE.");
    }

    if(uprv_isPositiveInfinity(ninf) != FALSE) {
        errln("FAIL: isPositiveInfinity(-Infinity) returned TRUE, should be FALSE.");
    }

    if((ninf < DBL_MAX) != TRUE) {
        errln("FAIL: -Infinity < DBL_MAX returned FALSE, should be TRUE.");
    }

    if((ninf < DBL_MIN) != TRUE) {
        errln("FAIL: -Infinity < DBL_MIN returned FALSE, should be TRUE.");
    }

    if((ninf < pinf) != TRUE) {
        errln("FAIL: -Infinity < +Infinity returned FALSE, should be TRUE.");
    }

    if((ninf < ten) != TRUE) {
        errln("FAIL: -Infinity < 10.0 returned FALSE, should be TRUE.");
    }
}







void           
PUtilTest::testZero(void)
{
    
    volatile double pzero   = 0.0;
    volatile double nzero   = 0.0;

    nzero *= -1;

    if((pzero == nzero) != TRUE) {
        errln("FAIL: 0.0 == -0.0 returned FALSE, should be TRUE.");
    }

    if((pzero > nzero) != FALSE) {
        errln("FAIL: 0.0 > -0.0 returned TRUE, should be FALSE.");
    }

    if((pzero >= nzero) != TRUE) {
        errln("FAIL: 0.0 >= -0.0 returned FALSE, should be TRUE.");
    }

    if((pzero < nzero) != FALSE) {
        errln("FAIL: 0.0 < -0.0 returned TRUE, should be FALSE.");
    }

    if((pzero <= nzero) != TRUE) {
        errln("FAIL: 0.0 <= -0.0 returned FALSE, should be TRUE.");
    }
#if U_PLATFORM != U_PF_OS400 
    if(uprv_isInfinite(1/pzero) != TRUE) {
        errln("FAIL: isInfinite(1/0.0) returned FALSE, should be TRUE.");
    }

    if(uprv_isInfinite(1/nzero) != TRUE) {
        errln("FAIL: isInfinite(1/-0.0) returned FALSE, should be TRUE.");
    }

    if(uprv_isPositiveInfinity(1/pzero) != TRUE) {
        errln("FAIL: isPositiveInfinity(1/0.0) returned FALSE, should be TRUE.");
    }

    if(uprv_isNegativeInfinity(1/nzero) != TRUE) {
        errln("FAIL: isNegativeInfinity(1/-0.0) returned FALSE, should be TRUE.");
    }
#endif
}



void
PUtilTest::testIsNaN(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if(uprv_isNaN(nan) == FALSE) {
        errln("FAIL: isNaN() returned FALSE for NaN.");
    }

    if(uprv_isNaN(pinf) == TRUE) {
        errln("FAIL: isNaN() returned TRUE for +Infinity.");
    }

    if(uprv_isNaN(ninf) == TRUE) {
        errln("FAIL: isNaN() returned TRUE for -Infinity.");
    }

    if(uprv_isNaN(ten) == TRUE) {
        errln("FAIL: isNaN() returned TRUE for 10.0.");
    }
}



void
PUtilTest::NaNGT(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan > nan) != FALSE) {
        logln("WARNING: NaN > NaN returned TRUE, should be FALSE");
    }

    if((nan > pinf) != FALSE) {
        logln("WARNING: NaN > +Infinity returned TRUE, should be FALSE");
    }

    if((nan > ninf) != FALSE) {
        logln("WARNING: NaN > -Infinity returned TRUE, should be FALSE");
    }

    if((nan > ten) != FALSE) {
        logln("WARNING: NaN > 10.0 returned TRUE, should be FALSE");
    }
}



void 
PUtilTest::NaNLT(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan < nan) != FALSE) {
        logln("WARNING: NaN < NaN returned TRUE, should be FALSE");
    }

    if((nan < pinf) != FALSE) {
        logln("WARNING: NaN < +Infinity returned TRUE, should be FALSE");
    }

    if((nan < ninf) != FALSE) {
        logln("WARNING: NaN < -Infinity returned TRUE, should be FALSE");
    }

    if((nan < ten) != FALSE) {
        logln("WARNING: NaN < 10.0 returned TRUE, should be FALSE");
    }
}



void                   
PUtilTest::NaNGTE(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan >= nan) != FALSE) {
        logln("WARNING: NaN >= NaN returned TRUE, should be FALSE");
    }

    if((nan >= pinf) != FALSE) {
        logln("WARNING: NaN >= +Infinity returned TRUE, should be FALSE");
    }

    if((nan >= ninf) != FALSE) {
        logln("WARNING: NaN >= -Infinity returned TRUE, should be FALSE");
    }

    if((nan >= ten) != FALSE) {
        logln("WARNING: NaN >= 10.0 returned TRUE, should be FALSE");
    }
}



void                   
PUtilTest::NaNLTE(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan <= nan) != FALSE) {
        logln("WARNING: NaN <= NaN returned TRUE, should be FALSE");
    }

    if((nan <= pinf) != FALSE) {
        logln("WARNING: NaN <= +Infinity returned TRUE, should be FALSE");
    }

    if((nan <= ninf) != FALSE) {
        logln("WARNING: NaN <= -Infinity returned TRUE, should be FALSE");
    }

    if((nan <= ten) != FALSE) {
        logln("WARNING: NaN <= 10.0 returned TRUE, should be FALSE");
    }
}



void                   
PUtilTest::NaNE(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan == nan) != FALSE) {
        logln("WARNING: NaN == NaN returned TRUE, should be FALSE");
    }

    if((nan == pinf) != FALSE) {
        logln("WARNING: NaN == +Infinity returned TRUE, should be FALSE");
    }

    if((nan == ninf) != FALSE) {
        logln("WARNING: NaN == -Infinity returned TRUE, should be FALSE");
    }

    if((nan == ten) != FALSE) {
        logln("WARNING: NaN == 10.0 returned TRUE, should be FALSE");
    }
}



void 
PUtilTest::NaNNE(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan != nan) != TRUE) {
        logln("WARNING: NaN != NaN returned FALSE, should be TRUE");
    }

    if((nan != pinf) != TRUE) {
        logln("WARNING: NaN != +Infinity returned FALSE, should be TRUE");
    }

    if((nan != ninf) != TRUE) {
        logln("WARNING: NaN != -Infinity returned FALSE, should be TRUE");
    }

    if((nan != ten) != TRUE) {
        logln("WARNING: NaN != 10.0 returned FALSE, should be TRUE");
    }
}
