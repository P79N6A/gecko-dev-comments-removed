





#ifndef _DATEFORMATTEST_
#define _DATEFORMATTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "caltztst.h"




class DateFormatTest: public CalendarTimeZoneTest {
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:
    



    void TestPatterns();
    



    virtual void TestWallyWedel(void);
    


    virtual void TestEquals(void);
    


    virtual void TestTwoDigitYearDSTParse(void);
 
public: 
    
    static UnicodeString& escape(UnicodeString& s);
 
public:
    


    void TestFieldPosition(void);
 
    void TestGeneral();

public: 
    
    static void getFieldText(DateFormat* df, int32_t field, UDate date, UnicodeString& str);
 
public:
    




    virtual void TestPartialParse994(void);
 
public: 
    
    virtual void tryPat994(SimpleDateFormat* format, const char* pat, const char* str, UDate expected);
 
public:
    



    virtual void TestRunTogetherPattern985(void);
    



    virtual void TestRunTogetherPattern917(void);
 
public: 
    
    virtual void testIt917(SimpleDateFormat* fmt, UnicodeString& str, UDate expected);
 
public:
    



    virtual void TestCzechMonths459(void);
    


    virtual void TestLetterDPattern212(void);
    


    virtual void TestDayOfYearPattern195(void);
 
public: 
    
    virtual void tryPattern(SimpleDateFormat& sdf, UDate d, const char* pattern, UDate expected);
 
public:
    


    virtual void TestQuotePattern161(void);
    


    virtual void TestBadInput135(void);
 
public:
    




    virtual void TestBadInput135a(void);
    


    virtual void TestTwoDigitYear(void);
 
public: 
    
    virtual void parse2DigitYear(DateFormat& fmt, const char* str, UDate expected);
 
public:
    


    virtual void TestDateFormatZone061(void);
    


    virtual void TestDateFormatZone146(void);

    void TestTimeZoneStringsAPI(void);

    void TestGMTParsing(void);

public: 
    


    virtual void TestLocaleDateFormat(void);

    virtual void TestFormattingLocaleTimeSeparator(void);

    virtual void TestDateFormatCalendar(void);

    virtual void TestSpaceParsing(void);

    void TestExactCountFormat(void);

    void TestWhiteSpaceParsing(void);

    void TestInvalidPattern(void);

    void TestGreekMay(void);

    void TestGenericTime(void);

    void TestGenericTimeZoneOrder(void);

    void Test6338(void);

    void Test6726(void);

    void Test6880(void);

    void TestISOEra(void);

    void TestFormalChineseDate(void);

    void TestStandAloneGMTParse(void);

    void TestParsePosition(void);

    void TestMonthPatterns(void);

    void TestContext(void);

    void TestNonGregoFmtParse(void);

public:
    


    void TestHost(void);

public:
    


    void TestEras(void);

    void TestNarrowNames(void);

    void TestShortDays(void);

    void TestStandAloneDays(void);

    void TestStandAloneMonths(void);

    void TestQuarters(void);
    
    void TestZTimeZoneParsing(void);

    void TestRelativeClone(void);
    
    void TestHostClone(void);

    void TestHebrewClone(void);

    void TestDateFormatSymbolsClone(void);

    void TestTimeZoneDisplayName(void);

    void TestRoundtripWithCalendar(void);

public:
    


     void TestRelative(void);




    void TestDotAndAtLeniency();

    void TestDateFormatLeniency();

    void TestParseMultiPatternMatch();

    void TestParseLeniencyAPIs();

    
    void TestNumberFormatOverride();
    void TestCreateInstanceForSkeleton();
    void TestCreateInstanceForSkeletonDefault();
    void TestCreateInstanceForSkeletonWithCalendar();
    void TestDFSCreateForLocaleNonGregorianLocale();
    void TestDFSCreateForLocaleWithCalendarInLocale();
    void TestChangeCalendar();

private:
    UBool showParse(DateFormat &format, const UnicodeString &formattedString);

public:
    


    void TestNumberAsStringParsing(void);

 private:
      void TestRelative(int daysdelta, 
                                  const Locale& loc,
                                  const char *expectChars);

 private:
    void expectParse(const char** data, int32_t data_length,
                     const Locale& locale);

    void expect(const char** data, int32_t data_length,
                const Locale& loc);

    void expectFormat(const char **data, int32_t data_length,
                      const Locale &locale);
};

#endif 
 
#endif 

