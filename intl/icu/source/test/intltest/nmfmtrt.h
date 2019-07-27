





#ifndef _NUMBERFORMATROUNDTRIPTEST_
#define _NUMBERFORMATROUNDTRIPTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/numfmt.h"
#include "unicode/fmtable.h"
#include "intltest.h"




class NumberFormatRoundTripTest : public IntlTest {    
    
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:

    static UBool verbose;
    static UBool STRING_COMPARE;
    static UBool EXACT_NUMERIC_COMPARE;
    static UBool DEBUG;
    static double MAX_ERROR;
    static double max_numeric_error;
    static double min_numeric_error;


    void start(void);

    void test(NumberFormat *fmt);
    void test(NumberFormat *fmt, double value);
    void test(NumberFormat *fmt, int32_t value);
    void test(NumberFormat *fmt, const Formattable& value);

    static double randomDouble(double range);
    static double proportionalError(const Formattable& a, const Formattable& b);
    static UnicodeString& typeOf(const Formattable& n, UnicodeString& result);
    static UnicodeString& escape(UnicodeString& s);

    static inline UBool
    isDouble(const Formattable& n)
    { return (n.getType() == Formattable::kDouble); }

    static inline UBool
    isLong(const Formattable& n)
    { return (n.getType() == Formattable::kLong); }

    


    static uint32_t randLong();

    


    static double randFraction()
    {
        return (double)randLong() / (double)0xFFFFFFFF;
    }

protected:
    UBool failure(UErrorCode status, const char* msg, UBool possibleDataError=FALSE);

};

#endif 
 
#endif 

