




#ifndef _TESTMESSAGEFORMAT
#define _TESTMESSAGEFORMAT

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/fmtable.h"
#include "unicode/msgfmt.h"
#include "intltest.h"




class TestMessageFormat: public IntlTest {
public:
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    


    void testBug1(void);
    


    void testBug2(void);
    


    void testBug3(void);
    


    void PatternTest(void);
    


    void sample(void);

    


    void testStaticFormat();
    


    void testSimpleFormat();
    


    void testMsgFormatChoice();
    


    void testMsgFormatPlural();

    


    void testMsgFormatSelect();

    void testApostropheInPluralAndSelect();

    


    void internalFormat(MessageFormat* msgFmt ,
        Formattable* args , int32_t numOfArgs ,
        UnicodeString expected, const char* errMsg);

    


    MessageFormat* internalCreate(
        UnicodeString pattern ,Locale locale , UErrorCode& err, char* errMsg);

    



    void TestUnlimitedArgsAndSubformats();

    


    void TestRBNF();

    void TestApostropheMode();

    void TestCompatibleApostrophe();

    





    void testCopyConstructor(void);
    void testCopyConstructor2(void);
    void testAssignment(void);
    void testClone(void);
    void testEquals(void);
    void testNotEquals(void);
    void testSetLocale(void);
    void testFormat(void);
    void testParse(void);
    void testAdopt(void);
    void TestTurkishCasing(void);
    void testAutoQuoteApostrophe(void);
    void testCoverage();
    void testGetFormatNames();
    void TestTrimArgumentName();
    void TestSelectOrdinal();
    void TestDecimals();

private:
    UnicodeString GetPatternAndSkipSyntax(const MessagePattern& pattern);
};

#endif 

#endif
