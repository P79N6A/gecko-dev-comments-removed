





#include "intltest.h"
#include "unicode/locid.h"




class LocaleTest: public IntlTest {
public:
    LocaleTest();
    virtual ~LocaleTest();
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    


    void TestBasicGetters(void);
    


    void TestParallelAPIValues(void);
    


    void TestSimpleResourceInfo(void);
    


    void TestDisplayNames(void);
    


    void TestSimpleObjectStuff(void);
    


    void TestPOSIXParsing(void);
    


    void TestGetAvailableLocales(void);
    


    void TestDataDirectory(void);

    void TestISO3Fallback(void);
    void TestGetLangsAndCountries(void);
    void TestSimpleDisplayNames(void);
    void TestUninstalledISO3Names(void);
    void TestAtypicalLocales(void);
#if !UCONFIG_NO_FORMATTING
    void TestThaiCurrencyFormat(void);
    void TestEuroSupport(void);
#endif
    void TestToString(void);
#if !UCONFIG_NO_FORMATTING
    void Test4139940(void);
    void Test4143951(void);
#endif
    void Test4147315(void);
    void Test4147317(void);
    void Test4147552(void);
    
    void TestVariantParsing(void);

   
   void TestKeywordVariants(void);

   
   void TestKeywordVariantParsing(void);

   
   void TestSetKeywordValue(void);

   
   void TestGetBaseName(void);
    
#if !UCONFIG_NO_FORMATTING
    void Test4105828(void) ;
#endif

    void TestSetIsBogus(void);

    void TestGetLocale(void);

    void TestVariantWithOutCountry(void);

    void TestCanonicalization(void);

#if !UCONFIG_NO_FORMATTING
    static UDate date(int32_t y, int32_t m, int32_t d, int32_t hr = 0, int32_t min = 0, int32_t sec = 0);
#endif

    void TestCurrencyByDate(void);

    void TestGetVariantWithKeywords(void);
    void TestIsRightToLeft();
    void TestBug11421();

private:
    void _checklocs(const char* label,
                    const char* req,
                    const Locale& validLoc,
                    const Locale& actualLoc,
                    const char* expReqValid="gt",
                    const char* expValidActual="ge"); 

    


    void doTestDisplayNames(Locale& inLocale, int32_t compareIndex);
    


    void setUpDataTable(void);

    UnicodeString** dataTable;
    
    enum {
        ENGLISH = 0,
        FRENCH = 1,
        CROATIAN = 2,
        GREEK = 3,
        NORWEGIAN = 4,
        ITALIAN = 5,
        XX = 6,
        CHINESE = 7,
        MAX_LOCALES = 7
    };

    enum {
        LANG = 0,
        SCRIPT,
        CTRY,
        VAR,
        NAME,
        LANG3,
        CTRY3,
        LCID,
        DLANG_EN,
        DSCRIPT_EN,
        DCTRY_EN,
        DVAR_EN,
        DNAME_EN,
        DLANG_FR,
        DSCRIPT_FR,
        DCTRY_FR,
        DVAR_FR,
        DNAME_FR,
        DLANG_CA,
        DSCRIPT_CA,
        DCTRY_CA,
        DVAR_CA,
        DNAME_CA,
        DLANG_EL,
        DSCRIPT_EL,
        DCTRY_EL,
        DVAR_EL,
        DNAME_EL,
        DLANG_NO,
        DSCRIPT_NO,
        DCTRY_NO,
        DVAR_NO,
        DNAME_NO
    };

#if !UCONFIG_NO_COLLATION
    



    void checkRegisteredCollators(const char *expectExtra = NULL);
#endif
};
