





#include "loctest.h"
#include "unicode/decimfmt.h"
#include "unicode/ucurr.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/brkiter.h"
#include "unicode/coll.h"
#include "cstring.h"
#include <stdio.h>
#include <string.h>
#include "putilimp.h"
#include "unicode/ustring.h"
#include "hash.h"

static const char* const rawData[33][8] = {

        
        {   "en",   "fr",   "ca",   "el",   "no",   "it",   "xx",   "zh"  },
        
        {   "",     "",     "",     "",     "",     "",     "",     "Hans" },
        
        {   "US",   "FR",   "ES",   "GR",   "NO",   "",     "YY",   "CN"  },
        
        {   "",     "",     "",     "",     "NY",   "",     "",   ""    },
        
        {   "en_US",    "fr_FR",    "ca_ES",    "el_GR",    "no_NO_NY", "it",   "xx_YY",   "zh_Hans_CN" },
        
        {   "eng",  "fra",  "cat",  "ell",  "nor",  "ita",  "",   "zho"   },
        
        {   "USA",  "FRA",  "ESP",  "GRC",  "NOR",  "",     "",   "CHN"   },
        
        {   "409", "40c", "403", "408", "814", "10",     "0",   "804"  },

        
        {   "English",  "French",   "Catalan", "Greek",    "Norwegian",    "Italian",  "xx",   "Chinese" },
        
        {   "",     "",     "",     "",     "",   "",     "",   "Simplified Han" },
        
        {   "United States",    "France",   "Spain",  "Greece",   "Norway",   "",     "YY",   "China" },
        
        {   "",     "",     "",     "",     "NY",   "",     "",   ""},
        
        
        
        {   "English (United States)", "French (France)", "Catalan (Spain)", "Greek (Greece)", "Norwegian (Norway, NY)", "Italian", "xx (YY)", "Chinese (Simplified, China)" },

        
        {   "anglais",  "fran\\u00E7ais",   "catalan", "grec",    "norv\\u00E9gien",    "italien", "xx", "chinois" },
        
        {   "",     "",     "",     "",     "",     "",     "",   "sinogrammes simplifi\\u00E9s" },
        
        {   "\\u00C9tats-Unis",    "France",   "Espagne",  "Gr\\u00E8ce",   "Norv\\u00E8ge", "", "YY", "Chine" },
        
        {   "",     "",     "",     "",     "NY",     "",     "",   "" },
        
        
        {   "anglais (\\u00C9tats-Unis)", "fran\\u00E7ais (France)", "catalan (Espagne)", "grec (Gr\\u00E8ce)", "norv\\u00E9gien (Norv\\u00E8ge, NY)", "italien", "xx (YY)", "chinois (simplifi\\u00E9, Chine)" },


        
        {   "angl\\u00E8s", "franc\\u00E8s", "catal\\u00E0", "grec",  "noruec", "itali\\u00E0", "", "xin\\u00E8s" },
        
        {   "", "", "",                    "", "", "", "", "han simplificat" },
        
        {   "Estats Units", "Fran\\u00E7a", "Espanya",  "Gr\\u00E8cia", "Noruega", "", "", "Xina" },
        
        {   "", "", "",                    "", "NY", "", "" },
        
        {   "angl\\u00E8s (Estats Units)", "franc\\u00E8s (Fran\\u00E7a)", "catal\\u00E0 (Espanya)", "grec (Gr\\u00E8cia)", "noruec (Noruega, NY)", "itali\\u00E0", "", "xin\\u00E8s (simplificat, Xina)" },

        
        {   "\\u0391\\u03b3\\u03b3\\u03bb\\u03b9\\u03ba\\u03ac",
            "\\u0393\\u03b1\\u03bb\\u03bb\\u03b9\\u03ba\\u03ac",
            "\\u039a\\u03b1\\u03c4\\u03b1\\u03bb\\u03b1\\u03bd\\u03b9\\u03ba\\u03ac",
            "\\u0395\\u03bb\\u03bb\\u03b7\\u03bd\\u03b9\\u03ba\\u03ac",
            "\\u039d\\u03bf\\u03c1\\u03b2\\u03b7\\u03b3\\u03b9\\u03ba\\u03ac",
            "\\u0399\\u03c4\\u03b1\\u03bb\\u03b9\\u03ba\\u03ac",
            "",
            "\\u039A\\u03B9\\u03BD\\u03B5\\u03B6\\u03B9\\u03BA\\u03AC"
        },
        
        {   "", "", "", "", "", "", "", "\\u0391\\u03c0\\u03bb\\u03bf\\u03c0\\u03bf\\u03b9\\u03b7\\u03bc\\u03ad\\u03bd\\u03bf \\u03a7\\u03b1\\u03bd" },
        
        {   "\\u0397\\u03BD\\u03C9\\u03BC\\u03AD\\u03BD\\u03B5\\u03C2 \\u03A0\\u03BF\\u03BB\\u03B9\\u03C4\\u03B5\\u03AF\\u03B5\\u03C2",
            "\\u0393\\u03b1\\u03bb\\u03bb\\u03af\\u03b1",
            "\\u0399\\u03c3\\u03c0\\u03b1\\u03bd\\u03af\\u03b1",
            "\\u0395\\u03bb\\u03bb\\u03ac\\u03b4\\u03b1",
            "\\u039d\\u03bf\\u03c1\\u03b2\\u03b7\\u03b3\\u03af\\u03b1",
            "",
            "",
            "\\u039A\\u03AF\\u03BD\\u03B1"
        },
        
        {   "", "", "", "", "NY", "", "" },
        
        {   "\\u0391\\u03b3\\u03b3\\u03bb\\u03b9\\u03ba\\u03ac (\\u0397\\u03BD\\u03C9\\u03BC\\u03AD\\u03BD\\u03B5\\u03C2 \\u03A0\\u03BF\\u03BB\\u03B9\\u03C4\\u03B5\\u03AF\\u03B5\\u03C2)",
            "\\u0393\\u03b1\\u03bb\\u03bb\\u03b9\\u03ba\\u03ac (\\u0393\\u03b1\\u03bb\\u03bb\\u03af\\u03b1)",
            "\\u039a\\u03b1\\u03c4\\u03b1\\u03bb\\u03b1\\u03bd\\u03b9\\u03ba\\u03ac (\\u0399\\u03c3\\u03c0\\u03b1\\u03bd\\u03af\\u03b1)",
            "\\u0395\\u03bb\\u03bb\\u03b7\\u03bd\\u03b9\\u03ba\\u03ac (\\u0395\\u03bb\\u03bb\\u03ac\\u03b4\\u03b1)",
            "\\u039d\\u03bf\\u03c1\\u03b2\\u03b7\\u03b3\\u03b9\\u03ba\\u03ac (\\u039d\\u03bf\\u03c1\\u03b2\\u03b7\\u03b3\\u03af\\u03b1, NY)",
            "\\u0399\\u03c4\\u03b1\\u03bb\\u03b9\\u03ba\\u03ac",
            "",
            "\\u039A\\u03B9\\u03BD\\u03B5\\u03B6\\u03B9\\u03BA\\u03AC (\\u0391\\u03c0\\u03bb\\u03bf\\u03c0\\u03bf\\u03b9\\u03b7\\u03bc\\u03ad\\u03bd\\u03bf, \\u039A\\u03AF\\u03BD\\u03B1)"
        },

        
        {   "English",  "French",   "Catalan", "Greek",    "Norwegian",    "Italian",  "xx", "" },
        
        {   "",     "",     "",     "",     "",   "",     "", ""},
        
        {   "United States",    "France",   "Spain",  "Greece",   "Norway",   "",     "YY", "" },
        
        {   "",     "",     "",     "",     "Nynorsk",   "",     "", ""},
        
        
        {   "English (United States)", "French (France)", "Catalan (Spain)", "Greek (Greece)", "Norwegian (Norway,NY)", "Italian", "xx (YY)", "" }
};












#define test_assert(test) \
    { \
        if(!(test)) \
            errln("FAIL: " #test " was not true. In " __FILE__ " on line %d", __LINE__ ); \
        else \
            logln("PASS: asserted " #test); \
    }











#define test_assert_print(test,print) \
    { \
        if(!(test)) \
            errln("FAIL: " #test " was not true. " + UnicodeString(print) ); \
        else \
            logln("PASS: asserted " #test "-> " + UnicodeString(print)); \
    }


#define test_dumpLocale(l) { logln(#l " = " + UnicodeString(l.getName(), "")); }

LocaleTest::LocaleTest()
: dataTable(NULL)
{
    setUpDataTable();
}

LocaleTest::~LocaleTest()
{
    if (dataTable != 0) {
        for (int32_t i = 0; i < 33; i++) {
            delete []dataTable[i];
        }
        delete []dataTable;
        dataTable = 0;
    }
}

void LocaleTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(TestBug11421);         
    TESTCASE_AUTO(TestBasicGetters);
    TESTCASE_AUTO(TestSimpleResourceInfo);
    TESTCASE_AUTO(TestDisplayNames);
    TESTCASE_AUTO(TestSimpleObjectStuff);
    TESTCASE_AUTO(TestPOSIXParsing);
    TESTCASE_AUTO(TestGetAvailableLocales);
    TESTCASE_AUTO(TestDataDirectory);
    TESTCASE_AUTO(TestISO3Fallback);
    TESTCASE_AUTO(TestGetLangsAndCountries);
    TESTCASE_AUTO(TestSimpleDisplayNames);
    TESTCASE_AUTO(TestUninstalledISO3Names);
    TESTCASE_AUTO(TestAtypicalLocales);
#if !UCONFIG_NO_FORMATTING
    TESTCASE_AUTO(TestThaiCurrencyFormat);
    TESTCASE_AUTO(TestEuroSupport);
#endif
    TESTCASE_AUTO(TestToString);
#if !UCONFIG_NO_FORMATTING
    TESTCASE_AUTO(Test4139940);
    TESTCASE_AUTO(Test4143951);
#endif
    TESTCASE_AUTO(Test4147315);
    TESTCASE_AUTO(Test4147317);
    TESTCASE_AUTO(Test4147552);
    TESTCASE_AUTO(TestVariantParsing);
#if !UCONFIG_NO_FORMATTING
    TESTCASE_AUTO(Test4105828);
#endif
    TESTCASE_AUTO(TestSetIsBogus);
    TESTCASE_AUTO(TestParallelAPIValues);
    TESTCASE_AUTO(TestKeywordVariants);
    TESTCASE_AUTO(TestKeywordVariantParsing);
    TESTCASE_AUTO(TestSetKeywordValue);
    TESTCASE_AUTO(TestGetBaseName);
#if !UCONFIG_NO_FILE_IO
    TESTCASE_AUTO(TestGetLocale);
#endif
    TESTCASE_AUTO(TestVariantWithOutCountry);
    TESTCASE_AUTO(TestCanonicalization);
    TESTCASE_AUTO(TestCurrencyByDate);
    TESTCASE_AUTO(TestGetVariantWithKeywords);
    TESTCASE_AUTO(TestIsRightToLeft);
    TESTCASE_AUTO_END;
}

void LocaleTest::TestBasicGetters() {
    UnicodeString   temp;

    int32_t i;
    for (i = 0; i <= MAX_LOCALES; i++) {
        Locale testLocale("");
        if (rawData[SCRIPT][i] && rawData[SCRIPT][i][0] != 0) {
            testLocale = Locale(rawData[LANG][i], rawData[SCRIPT][i], rawData[CTRY][i], rawData[VAR][i]);
        }
        else {
            testLocale = Locale(rawData[LANG][i], rawData[CTRY][i], rawData[VAR][i]);
        }
        logln("Testing " + (UnicodeString)testLocale.getName() + "...");

        if ( (temp=testLocale.getLanguage()) != (dataTable[LANG][i]))
            errln("  Language code mismatch: " + temp + " versus "
                        + dataTable[LANG][i]);
        if ( (temp=testLocale.getScript()) != (dataTable[SCRIPT][i]))
            errln("  Script code mismatch: " + temp + " versus "
                        + dataTable[SCRIPT][i]);
        if ( (temp=testLocale.getCountry()) != (dataTable[CTRY][i]))
            errln("  Country code mismatch: " + temp + " versus "
                        + dataTable[CTRY][i]);
        if ( (temp=testLocale.getVariant()) != (dataTable[VAR][i]))
            errln("  Variant code mismatch: " + temp + " versus "
                        + dataTable[VAR][i]);
        if ( (temp=testLocale.getName()) != (dataTable[NAME][i]))
            errln("  Locale name mismatch: " + temp + " versus "
                        + dataTable[NAME][i]);
    }

    logln("Same thing without variant codes...");
    for (i = 0; i <= MAX_LOCALES; i++) {
        Locale testLocale("");
        if (rawData[SCRIPT][i] && rawData[SCRIPT][i][0] != 0) {
            testLocale = Locale(rawData[LANG][i], rawData[SCRIPT][i], rawData[CTRY][i]);
        }
        else {
            testLocale = Locale(rawData[LANG][i], rawData[CTRY][i]);
        }
        logln("Testing " + (temp=testLocale.getName()) + "...");

        if ( (temp=testLocale.getLanguage()) != (dataTable[LANG][i]))
            errln("Language code mismatch: " + temp + " versus "
                        + dataTable[LANG][i]);
        if ( (temp=testLocale.getScript()) != (dataTable[SCRIPT][i]))
            errln("Script code mismatch: " + temp + " versus "
                        + dataTable[SCRIPT][i]);
        if ( (temp=testLocale.getCountry()) != (dataTable[CTRY][i]))
            errln("Country code mismatch: " + temp + " versus "
                        + dataTable[CTRY][i]);
        if (testLocale.getVariant()[0] != 0)
            errln("Variant code mismatch: something versus \"\"");
    }

    logln("Testing long language names and getters");
    Locale  test8 = Locale::createFromName("x-klingon-latn-zx.utf32be@special");

    temp = test8.getLanguage();
    if (temp != UnicodeString("x-klingon") )
        errln("Language code mismatch: " + temp + "  versus \"x-klingon\"");

    temp = test8.getScript();
    if (temp != UnicodeString("Latn") )
        errln("Script code mismatch: " + temp + "  versus \"Latn\"");

    temp = test8.getCountry();
    if (temp != UnicodeString("ZX") )
        errln("Country code mismatch: " + temp + "  versus \"ZX\"");

    temp = test8.getVariant();
    
    
    
    if (temp != UnicodeString("") )
        errln("Variant code mismatch: " + temp + "  versus \"\"");

    if (Locale::getDefault() != Locale::createFromName(NULL))
        errln("Locale::getDefault() == Locale::createFromName(NULL)");

    
    
    
    
    

    Locale bogusLang("THISISABOGUSLANGUAGE"); 
    if(!bogusLang.isBogus()) {
        errln("Locale(\"THISISABOGUSLANGUAGE\").isBogus()==FALSE");
    }

    bogusLang=Locale("eo");
    if( bogusLang.isBogus() ||
        strcmp(bogusLang.getLanguage(), "eo")!=0 ||
        *bogusLang.getCountry()!=0 ||
        *bogusLang.getVariant()!=0 ||
        strcmp(bogusLang.getName(), "eo")!=0
    ) {
        errln("assignment to bogus Locale does not unbogus it or sets bad data");
    }

    Locale a("eo_DE@currency=DEM");
    Locale *pb=a.clone();
    if(pb==&a || *pb!=a) {
        errln("Locale.clone() failed");
    }
    delete pb;
}

void LocaleTest::TestParallelAPIValues() {
    logln("Test synchronization between C and C++ API");
    if (strcmp(Locale::getChinese().getName(), ULOC_CHINESE) != 0) {
        errln("Differences for ULOC_CHINESE Locale");
    }
    if (strcmp(Locale::getEnglish().getName(), ULOC_ENGLISH) != 0) {
        errln("Differences for ULOC_ENGLISH Locale");
    }
    if (strcmp(Locale::getFrench().getName(), ULOC_FRENCH) != 0) {
        errln("Differences for ULOC_FRENCH Locale");
    }
    if (strcmp(Locale::getGerman().getName(), ULOC_GERMAN) != 0) {
        errln("Differences for ULOC_GERMAN Locale");
    }
    if (strcmp(Locale::getItalian().getName(), ULOC_ITALIAN) != 0) {
        errln("Differences for ULOC_ITALIAN Locale");
    }
    if (strcmp(Locale::getJapanese().getName(), ULOC_JAPANESE) != 0) {
        errln("Differences for ULOC_JAPANESE Locale");
    }
    if (strcmp(Locale::getKorean().getName(), ULOC_KOREAN) != 0) {
        errln("Differences for ULOC_KOREAN Locale");
    }
    if (strcmp(Locale::getSimplifiedChinese().getName(), ULOC_SIMPLIFIED_CHINESE) != 0) {
        errln("Differences for ULOC_SIMPLIFIED_CHINESE Locale");
    }
    if (strcmp(Locale::getTraditionalChinese().getName(), ULOC_TRADITIONAL_CHINESE) != 0) {
        errln("Differences for ULOC_TRADITIONAL_CHINESE Locale");
    }


    if (strcmp(Locale::getCanada().getName(), ULOC_CANADA) != 0) {
        errln("Differences for ULOC_CANADA Locale");
    }
    if (strcmp(Locale::getCanadaFrench().getName(), ULOC_CANADA_FRENCH) != 0) {
        errln("Differences for ULOC_CANADA_FRENCH Locale");
    }
    if (strcmp(Locale::getChina().getName(), ULOC_CHINA) != 0) {
        errln("Differences for ULOC_CHINA Locale");
    }
    if (strcmp(Locale::getPRC().getName(), ULOC_PRC) != 0) {
        errln("Differences for ULOC_PRC Locale");
    }
    if (strcmp(Locale::getFrance().getName(), ULOC_FRANCE) != 0) {
        errln("Differences for ULOC_FRANCE Locale");
    }
    if (strcmp(Locale::getGermany().getName(), ULOC_GERMANY) != 0) {
        errln("Differences for ULOC_GERMANY Locale");
    }
    if (strcmp(Locale::getItaly().getName(), ULOC_ITALY) != 0) {
        errln("Differences for ULOC_ITALY Locale");
    }
    if (strcmp(Locale::getJapan().getName(), ULOC_JAPAN) != 0) {
        errln("Differences for ULOC_JAPAN Locale");
    }
    if (strcmp(Locale::getKorea().getName(), ULOC_KOREA) != 0) {
        errln("Differences for ULOC_KOREA Locale");
    }
    if (strcmp(Locale::getTaiwan().getName(), ULOC_TAIWAN) != 0) {
        errln("Differences for ULOC_TAIWAN Locale");
    }
    if (strcmp(Locale::getUK().getName(), ULOC_UK) != 0) {
        errln("Differences for ULOC_UK Locale");
    }
    if (strcmp(Locale::getUS().getName(), ULOC_US) != 0) {
        errln("Differences for ULOC_US Locale");
    }
}


void LocaleTest::TestSimpleResourceInfo() {
    UnicodeString   temp;
    char            temp2[20];
    UErrorCode err = U_ZERO_ERROR;
    int32_t i = 0;

    for (i = 0; i <= MAX_LOCALES; i++) {
        Locale testLocale(rawData[LANG][i], rawData[CTRY][i], rawData[VAR][i]);
        logln("Testing " + (temp=testLocale.getName()) + "...");

        if ( (temp=testLocale.getISO3Language()) != (dataTable[LANG3][i]))
            errln("  ISO-3 language code mismatch: " + temp
                + " versus " + dataTable[LANG3][i]);
        if ( (temp=testLocale.getISO3Country()) != (dataTable[CTRY3][i]))
            errln("  ISO-3 country code mismatch: " + temp
                + " versus " + dataTable[CTRY3][i]);

        sprintf(temp2, "%x", (int)testLocale.getLCID());
        if (UnicodeString(temp2) != dataTable[LCID][i])
            errln((UnicodeString)"  LCID mismatch: " + temp2 + " versus "
                + dataTable[LCID][i]);

        if(U_FAILURE(err))
        {
            errln((UnicodeString)"Some error on number " + i + u_errorName(err));
        }
        err = U_ZERO_ERROR;
    }

    Locale locale("en");
    if(strcmp(locale.getName(), "en") != 0||
        strcmp(locale.getLanguage(), "en") != 0) {
        errln("construction of Locale(en) failed\n");
    }
    

}







void
LocaleTest::TestDisplayNames()
{
    Locale  english("en", "US");
    Locale  french("fr", "FR");
    Locale  croatian("ca", "ES");
    Locale  greek("el", "GR");

    logln("  In locale = en_US...");
    doTestDisplayNames(english, DLANG_EN);
    logln("  In locale = fr_FR...");
    doTestDisplayNames(french, DLANG_FR);
    logln("  In locale = ca_ES...");
    doTestDisplayNames(croatian, DLANG_CA);
    logln("  In locale = el_GR...");
    doTestDisplayNames(greek, DLANG_EL);

    UnicodeString s;
    UErrorCode status = U_ZERO_ERROR;

#if !UCONFIG_NO_FORMATTING
    DecimalFormatSymbols symb(status);
    
    if (symb.getLocale(ULOC_VALID_LOCALE, status) != Locale("root")) {
        
        
        if (uprv_strcmp(Locale().getLanguage(), "tl") != 0) {
            Locale().getDisplayLanguage(Locale(), s);
            if(s.length()<=3 && s.charAt(0)<=0x7f) {
                
                dataerrln("unable to get a display string for the language of the default locale: " + s);
            }

            




            s.remove();
            Locale().getDisplayLanguage(s);
            if(s.length()<=3 && s.charAt(0)<=0x7f) {
                dataerrln("unable to get a display string for the language of the default locale [2]: " + s);
            }
        }
    }
    else {
        logln("Default locale %s is unsupported by ICU\n", Locale().getName());
    }
    s.remove();
#endif

    french.getDisplayCountry(s);
    if(s.isEmpty()) {
        errln("unable to get any default-locale display string for the country of fr_FR\n");
    }
    s.remove();
    Locale("zh", "Hant").getDisplayScript(s);
    if(s.isEmpty()) {
        errln("unable to get any default-locale display string for the country of zh_Hant\n");
    }
}

void LocaleTest::TestSimpleObjectStuff() {
    Locale  test1("aa", "AA");
    Locale  test2("aa", "AA");
    Locale  test3(test1);
    Locale  test4("zz", "ZZ");
    Locale  test5("aa", "AA", "");
    Locale  test6("aa", "AA", "ANTARES");
    Locale  test7("aa", "AA", "JUPITER");
    Locale  test8 = Locale::createFromName("aa-aa-jupiTER"); 

    
    test_dumpLocale(test1);
    test_dumpLocale(test2);
    test_dumpLocale(test3);
    test_dumpLocale(test4);
    test_dumpLocale(test5);
    test_dumpLocale(test6);
    test_dumpLocale(test7);
    test_dumpLocale(test8);

    
    test_assert(test1 == test1);
    test_assert(test2 == test2);
    test_assert(test3 == test3);
    test_assert(test4 == test4);
    test_assert(test5 == test5);
    test_assert(test6 == test6);
    test_assert(test7 == test7);
    test_assert(test8 == test8);

    
    test_assert(!(test1 != test1));
    test_assert(!(test2 != test2));
    test_assert(!(test3 != test3));
    test_assert(!(test4 != test4));
    test_assert(!(test5 != test5));
    test_assert(!(test6 != test6));
    test_assert(!(test7 != test7));
    test_assert(!(test8 != test8));

    
    test_assert(!(test1 != test2));
    test_assert(!(test2 != test1));
    test_assert(!(test1 != test3));
    test_assert(!(test2 != test3));
    test_assert(test5 == test1);
    test_assert(test6 != test2);
    test_assert(test6 != test5);

    test_assert(test6 != test7);

    
    test_assert(!(test1 == test4));
    test_assert(!(test2 == test4));
    test_assert(!(test3 == test4));

    test_assert(test7 == test8);

    
    int32_t hash1 = test1.hashCode();
    int32_t hash2 = test2.hashCode();
    int32_t hash3 = test3.hashCode();

    test_assert(hash1 == hash2);
    test_assert(hash1 == hash3);
    test_assert(hash2 == hash3);

    
    test4 = test1;
    logln("test4=test1;");
    test_dumpLocale(test4);
    test_assert(test4 == test4);

    test_assert(!(test1 != test4));
    test_assert(!(test2 != test4));
    test_assert(!(test3 != test4));
    test_assert(test1 == test4);
    test_assert(test4 == test1);

    
    logln("test7 = test6");
    test7 = test6;
    test_dumpLocale(test7);
    test_assert(test7 == test7);
    test_assert(test7 == test6);
    test_assert(test7 != test5);

    logln("test6 = test1");
    test6=test1;
    test_dumpLocale(test6);
    test_assert(test6 != test7);
    test_assert(test6 == test1);
    test_assert(test6 == test6);
}


class POSIXLocale : public Locale
{
public:
    POSIXLocale(const UnicodeString& l)
        :Locale()
    {
      char *ch;
      ch = new char[l.length() + 1];
      ch[l.extract(0, 0x7fffffff, ch, "")] = 0;
      setFromPOSIXID(ch);
      delete [] ch;
    }
    POSIXLocale(const char *l)
        :Locale()
    {
        setFromPOSIXID(l);
    }
};

void LocaleTest::TestPOSIXParsing()
{
    POSIXLocale  test1("ab_AB");
    POSIXLocale  test2(UnicodeString("ab_AB"));
    Locale  test3("ab","AB");

    POSIXLocale test4("ab_AB_Antares");
    POSIXLocale test5(UnicodeString("ab_AB_Antares"));
    Locale  test6("ab", "AB", "Antares");

    test_dumpLocale(test1);
    test_dumpLocale(test2);
    test_dumpLocale(test3);
    test_dumpLocale(test4);
    test_dumpLocale(test5);
    test_dumpLocale(test6);

    test_assert(test1 == test1);

    test_assert(test1 == test2);
    test_assert(test2 == test3);
    test_assert(test3 == test1);

    test_assert(test4 == test5);
    test_assert(test5 == test6);
    test_assert(test6 == test4);

    test_assert(test1 != test4);
    test_assert(test5 != test3);
    test_assert(test5 != test2);

    int32_t hash1 = test1.hashCode();
    int32_t hash2 = test2.hashCode();
    int32_t hash3 = test3.hashCode();

    test_assert(hash1 == hash2);
    test_assert(hash2 == hash3);
    test_assert(hash3 == hash1);
}

void LocaleTest::TestGetAvailableLocales()
{
    int32_t locCount = 0;
    const Locale* locList = Locale::getAvailableLocales(locCount);

    if (locCount == 0)
        dataerrln("getAvailableLocales() returned an empty list!");
    else {
        logln(UnicodeString("Number of locales returned = ") + locCount);
        UnicodeString temp;
        for(int32_t i = 0; i < locCount; ++i)
            logln(locList[i].getName());
    }
    
}



void LocaleTest::TestDataDirectory()
{

































}



void LocaleTest::doTestDisplayNames(Locale& displayLocale, int32_t compareIndex) {
    UnicodeString   temp;

    for (int32_t i = 0; i <= MAX_LOCALES; i++) {
        Locale testLocale("");
        if (rawData[SCRIPT][i] && rawData[SCRIPT][i][0] != 0) {
            testLocale = Locale(rawData[LANG][i], rawData[SCRIPT][i], rawData[CTRY][i], rawData[VAR][i]);
        }
        else {
            testLocale = Locale(rawData[LANG][i], rawData[CTRY][i], rawData[VAR][i]);
        }
        logln("  Testing " + (temp=testLocale.getName()) + "...");

        UnicodeString  testLang;
        UnicodeString  testScript;
        UnicodeString  testCtry;
        UnicodeString  testVar;
        UnicodeString  testName;

        testLocale.getDisplayLanguage(displayLocale, testLang);
        testLocale.getDisplayScript(displayLocale, testScript);
        testLocale.getDisplayCountry(displayLocale, testCtry);
        testLocale.getDisplayVariant(displayLocale, testVar);
        testLocale.getDisplayName(displayLocale, testName);

        UnicodeString  expectedLang;
        UnicodeString  expectedScript;
        UnicodeString  expectedCtry;
        UnicodeString  expectedVar;
        UnicodeString  expectedName;

        expectedLang = dataTable[compareIndex][i];
        if (expectedLang.length() == 0)
            expectedLang = dataTable[DLANG_EN][i];

        expectedScript = dataTable[compareIndex + 1][i];
        if (expectedScript.length() == 0)
            expectedScript = dataTable[DSCRIPT_EN][i];

        expectedCtry = dataTable[compareIndex + 2][i];
        if (expectedCtry.length() == 0)
            expectedCtry = dataTable[DCTRY_EN][i];

        expectedVar = dataTable[compareIndex + 3][i];
        if (expectedVar.length() == 0)
            expectedVar = dataTable[DVAR_EN][i];

        expectedName = dataTable[compareIndex + 4][i];
        if (expectedName.length() == 0)
            expectedName = dataTable[DNAME_EN][i];

        if (testLang != expectedLang)
            dataerrln("Display language (" + UnicodeString(displayLocale.getName()) + ") of (" + UnicodeString(testLocale.getName()) + ") got " + testLang + " expected " + expectedLang);
        if (testScript != expectedScript)
            dataerrln("Display script (" + UnicodeString(displayLocale.getName()) + ") of (" + UnicodeString(testLocale.getName()) + ") got " + testScript + " expected " + expectedScript);
        if (testCtry != expectedCtry)
            dataerrln("Display country (" + UnicodeString(displayLocale.getName()) + ") of (" + UnicodeString(testLocale.getName()) + ") got " + testCtry + " expected " + expectedCtry);
        if (testVar != expectedVar)
            dataerrln("Display variant (" + UnicodeString(displayLocale.getName()) + ") of (" + UnicodeString(testLocale.getName()) + ") got " + testVar + " expected " + expectedVar);
        if (testName != expectedName)
            dataerrln("Display name (" + UnicodeString(displayLocale.getName()) + ") of (" + UnicodeString(testLocale.getName()) + ") got " + testName + " expected " + expectedName);
    }
}







void LocaleTest::setUpDataTable()
{
    if (dataTable == 0) {
        dataTable = new UnicodeString*[33];

        for (int32_t i = 0; i < 33; i++) {
            dataTable[i] = new UnicodeString[8];
            for (int32_t j = 0; j < 8; j++) {
                dataTable[i][j] = CharsToUnicodeString(rawData[i][j]);
            }
        }
    }
}







void
LocaleTest::TestISO3Fallback()
{
    Locale test("xx", "YY");

    const char * result;

    result = test.getISO3Language();

    

    if (!result || (result[0] != 0))
        errln("getISO3Language() on xx_YY returned " + UnicodeString(result) + " instead of \"\"");

    result = test.getISO3Country();

    if (!result || (result[0] != 0))
        errln("getISO3Country() on xx_YY returned " + UnicodeString(result) + " instead of \"\"");
}




void
LocaleTest::TestGetLangsAndCountries()
{
    
    
    
    
    
    
    
    
    int32_t testCount = 0;
    const char * const * test = Locale::getISOLanguages();
    const char spotCheck1[ ][4] = { "en", "es", "fr", "de", "it",
                                    "ja", "ko", "zh", "th", "he",
                                    "id", "iu", "ug", "yi", "za" };

    int32_t i;

    for(testCount = 0;test[testCount];testCount++)
      ;

    
    if (testCount != 560)
        errln("Expected getISOLanguages() to return 560 languages; it returned %d", testCount);
    else {
        for (i = 0; i < 15; i++) {
            int32_t j;
            for (j = 0; j < testCount; j++)
              if (uprv_strcmp(test[j],spotCheck1[i])== 0)
                    break;
            if (j == testCount || (uprv_strcmp(test[j],spotCheck1[i])!=0))
                errln("Couldn't find " + (UnicodeString)spotCheck1[i] + " in language list.");
        }
    }
    for (i = 0; i < testCount; i++) {
        UnicodeString testee(test[i],"");
        UnicodeString lc(test[i],"");
        if (testee != lc.toLower())
            errln(lc + " is not all lower case.");
        if ( (testee.length() != 2) && (testee.length() != 3))
            errln(testee + " is not two or three characters long.");
        if (i > 0 && testee.compare(test[i - 1]) <= 0)
            errln(testee + " appears in an out-of-order position in the list.");
    }

    test = Locale::getISOCountries();
    UnicodeString spotCheck2 [] = { "US", "CA", "GB", "FR", "DE",
                                    "IT", "JP", "KR", "CN", "TW",
                                    "TH" };
    int32_t spot2Len = 11;
    for(testCount=0;test[testCount];testCount++)
      ;

    if (testCount != 249){
        errln("Expected getISOCountries to return 249 countries; it returned %d", testCount);
    }else {
        for (i = 0; i < spot2Len; i++) {
            int32_t j;
            for (j = 0; j < testCount; j++)
              {
                UnicodeString testee(test[j],"");

                if (testee == spotCheck2[i])
                    break;
              }
                UnicodeString testee(test[j],"");
            if (j == testCount || testee != spotCheck2[i])
                errln("Couldn't find " + spotCheck2[i] + " in country list.");
        }
    }
    for (i = 0; i < testCount; i++) {
        UnicodeString testee(test[i],"");
        UnicodeString uc(test[i],"");
        if (testee != uc.toUpper())
            errln(testee + " is not all upper case.");
        if (testee.length() != 2)
            errln(testee + " is not two characters long.");
        if (i > 0 && testee.compare(test[i - 1]) <= 0)
            errln(testee + " appears in an out-of-order position in the list.");
    }

    
    {
        int32_t numOfLocales;
        Locale  enLoc ("en");
        const Locale *pLocales = Locale::getAvailableLocales(numOfLocales);

        for (int i = 0; i < numOfLocales; i++) {
            const Locale    &loc(pLocales[i]);
            UnicodeString   name;
            char        szName[200];

            loc.getDisplayName (enLoc, name);
            name.extract (0, 200, szName, sizeof(szName));

            if (strlen(loc.getISO3Language()) == 0) {
                errln("getISO3Language() returned an empty string for: " + name);
            }
        }
    }
}




void
LocaleTest::TestSimpleDisplayNames()
{
    
    
    
    
    char languageCodes[] [4] = { "he", "id", "iu", "ug", "yi", "za" };
    UnicodeString languageNames [] = { "Hebrew", "Indonesian", "Inuktitut", "Uyghur", "Yiddish",
                               "Zhuang" };

    for (int32_t i = 0; i < 6; i++) {
        UnicodeString test;
        Locale l(languageCodes[i], "", "");
        l.getDisplayLanguage(Locale::getUS(), test);
        if (test != languageNames[i])
            dataerrln("Got wrong display name for " + UnicodeString(languageCodes[i]) + ": Expected \"" +
                  languageNames[i] + "\", got \"" + test + "\".");
    }
}




void
LocaleTest::TestUninstalledISO3Names()
{
    
    
    const char iso2Languages [][4] = {     "am", "ba", "fy", "mr", "rn",
                                        "ss", "tw", "zu" };
    const char iso3Languages [][5] = {     "amh", "bak", "fry", "mar", "run",
                                        "ssw", "twi", "zul" };

    int32_t i;

    for (i = 0; i < 8; i++) {
      UErrorCode err = U_ZERO_ERROR;

      UnicodeString test;
        Locale l(iso2Languages[i], "", "");
        test = l.getISO3Language();
        if((test != iso3Languages[i]) || U_FAILURE(err))
            errln("Got wrong ISO3 code for " + UnicodeString(iso2Languages[i]) + ": Expected \"" +
                    iso3Languages[i] + "\", got \"" + test + "\"." + UnicodeString(u_errorName(err)));
    }

    char iso2Countries [][4] = {     "AF", "BW", "KZ", "MO", "MN",
                                        "SB", "TC", "ZW" };
    char iso3Countries [][4] = {     "AFG", "BWA", "KAZ", "MAC", "MNG",
                                        "SLB", "TCA", "ZWE" };

    for (i = 0; i < 8; i++) {
      UErrorCode err = U_ZERO_ERROR;
        Locale l("", iso2Countries[i], "");
        UnicodeString test(l.getISO3Country(), "");
        if (test != iso3Countries[i])
            errln("Got wrong ISO3 code for " + UnicodeString(iso2Countries[i]) + ": Expected \"" +
                    UnicodeString(iso3Countries[i]) + "\", got \"" + test + "\"." + u_errorName(err));
    }
}







void
LocaleTest::TestAtypicalLocales()
{
    Locale localesToTest [] = { Locale("de", "CA"),
                                  Locale("ja", "ZA"),
                                   Locale("ru", "MX"),
                                   Locale("en", "FR"),
                                   Locale("es", "DE"),
                                   Locale("", "HR"),
                                   Locale("", "SE"),
                                   Locale("", "DO"),
                                   Locale("", "BE") };

    UnicodeString englishDisplayNames [] = { "German (Canada)",
                                     "Japanese (South Africa)",
                                     "Russian (Mexico)",
                                     "English (France)",
                                     "Spanish (Germany)",
                                     "Croatia",
                                     "Sweden",
                                     "Dominican Republic",
                                     "Belgium" };
    UnicodeString frenchDisplayNames []= { "allemand (Canada)",
                                    "japonais (Afrique du Sud)",
                                    "russe (Mexique)",
                                     "anglais (France)",
                                     "espagnol (Allemagne)",
                                    "Croatie",
                                    CharsToUnicodeString("Su\\u00E8de"),
                                    CharsToUnicodeString("R\\u00E9publique dominicaine"),
                                    "Belgique" };
    UnicodeString spanishDisplayNames [] = {
                                     CharsToUnicodeString("alem\\u00E1n (Canad\\u00E1)"),
                                     CharsToUnicodeString("japon\\u00E9s (Sud\\u00E1frica)"),
                                     CharsToUnicodeString("ruso (M\\u00E9xico)"),
                                     CharsToUnicodeString("ingl\\u00E9s (Francia)"),
                                     CharsToUnicodeString("espa\\u00F1ol (Alemania)"),
                                     "Croacia",
                                     "Suecia",
                                     CharsToUnicodeString("Rep\\u00FAblica Dominicana"),
                                     CharsToUnicodeString("B\\u00E9lgica") };
    
    
    UnicodeString invDisplayNames [] = { "German (Canada)",
                                     "Japanese (South Africa)",
                                     "Russian (Mexico)",
                                     "English (France)",
                                     "Spanish (Germany)",
                                     "Croatia",
                                     "Sweden",
                                     "Dominican Republic",
                                     "Belgium" };

    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    Locale saveLocale;
    Locale::setDefault(Locale::getUS(), status);
    for (i = 0; i < 9; ++i) {
        UnicodeString name;
        localesToTest[i].getDisplayName(Locale::getUS(), name);
        logln(name);
        if (name != englishDisplayNames[i])
        {
            dataerrln("Lookup in English failed: expected \"" + englishDisplayNames[i]
                        + "\", got \"" + name + "\"");
            logln("Locale name was-> " + (name=localesToTest[i].getName()));
        }
    }

    for (i = 0; i < 9; i++) {
        UnicodeString name;
        localesToTest[i].getDisplayName(Locale("es", "ES"), name);
        logln(name);
        if (name != spanishDisplayNames[i])
            dataerrln("Lookup in Spanish failed: expected \"" + spanishDisplayNames[i]
                        + "\", got \"" + name + "\"");
    }

    for (i = 0; i < 9; i++) {
        UnicodeString name;
        localesToTest[i].getDisplayName(Locale::getFrance(), name);
        logln(name);
        if (name != frenchDisplayNames[i])
            dataerrln("Lookup in French failed: expected \"" + frenchDisplayNames[i]
                        + "\", got \"" + name + "\"");
    }

    for (i = 0; i < 9; i++) {
        UnicodeString name;
        localesToTest[i].getDisplayName(Locale("inv", "IN"), name);
        logln(name + " Locale fallback to be, and data fallback to root");
        if (name != invDisplayNames[i])
            dataerrln("Lookup in INV failed: expected \"" + prettify(invDisplayNames[i])
                        + "\", got \"" + prettify(name) + "\"");
        localesToTest[i].getDisplayName(Locale("inv", "BD"), name);
        logln(name + " Data fallback to root");
        if (name != invDisplayNames[i])
            dataerrln("Lookup in INV failed: expected \"" + prettify(invDisplayNames[i])
                        + "\", got \"" + prettify(name )+ "\"");
    }
    Locale::setDefault(saveLocale, status);
}

#if !UCONFIG_NO_FORMATTING






void
LocaleTest::TestThaiCurrencyFormat()
{
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormat *thaiCurrency = (DecimalFormat*)NumberFormat::createCurrencyInstance(
                    Locale("th", "TH"), status);
    UnicodeString posPrefix("THB", 3, US_INV);  
    UnicodeString temp;

    if(U_FAILURE(status) || !thaiCurrency)
    {
        dataerrln("Couldn't get th_TH currency -> " + UnicodeString(u_errorName(status)));
        return;
    }
    if (thaiCurrency->getPositivePrefix(temp) != posPrefix)
        errln("Thai currency prefix wrong: expected THB, got \"" +
                        thaiCurrency->getPositivePrefix(temp) + "\"");
    if (thaiCurrency->getPositiveSuffix(temp) != "")
        errln("Thai currency suffix wrong: expected \"\", got \"" +
                        thaiCurrency->getPositiveSuffix(temp) + "\"");

    delete thaiCurrency;
}












void
LocaleTest::TestEuroSupport()
{
    UChar euro = 0x20ac;
    const UnicodeString EURO_CURRENCY(&euro, 1, 1); 
    const char* localeArr[] = {
                            "ca_ES",
                            "de_AT",
                            "de_DE",
                            "de_LU",
                            "el_GR",
                            "en_BE",
                            "en_IE",
                            "en_GB_EURO",
                            "en_US_EURO",
                            "es_ES",
                            "eu_ES",
                            "fi_FI",
                            "fr_BE",
                            "fr_FR",
                            "fr_LU",
                            "ga_IE",
                            "gl_ES",
                            "it_IT",
                            "nl_BE",
                            "nl_NL",
                            "pt_PT",
                            NULL
                        };
    const char** locales = localeArr;

    UErrorCode status = U_ZERO_ERROR;

    UnicodeString temp;

    for (;*locales!=NULL;locales++) {
        Locale loc (*locales);
        UnicodeString temp;
        NumberFormat *nf = NumberFormat::createCurrencyInstance(loc, status);
        UnicodeString pos;

        if (U_FAILURE(status)) {
            dataerrln("Error calling NumberFormat::createCurrencyInstance(%s)", *locales);
            continue;
        }

        nf->format(271828.182845, pos);
        UnicodeString neg;
        nf->format(-271828.182845, neg);
        if (pos.indexOf(EURO_CURRENCY) >= 0 &&
            neg.indexOf(EURO_CURRENCY) >= 0) {
            logln("Ok: " + (temp=loc.getName()) +
                ": " + pos + " / " + neg);
        }
        else {
            errln("Fail: " + (temp=loc.getName()) +
                " formats without " + EURO_CURRENCY +
                ": " + pos + " / " + neg +
                "\n*** THIS FAILURE MAY ONLY MEAN THAT LOCALE DATA HAS CHANGED ***");
        }

        delete nf;
    }

    UnicodeString dollarStr("USD", ""), euroStr("EUR", ""), genericStr((UChar)0x00a4), resultStr;
    UChar tmp[4];
    status = U_ZERO_ERROR;

    ucurr_forLocale("en_US", tmp, 4, &status);
    resultStr.setTo(tmp);
    if (dollarStr != resultStr) {
        errcheckln(status, "Fail: en_US didn't return USD - %s", u_errorName(status));
    }
    ucurr_forLocale("en_US_EURO", tmp, 4, &status);
    resultStr.setTo(tmp);
    if (euroStr != resultStr) {
        errcheckln(status, "Fail: en_US_EURO didn't return EUR - %s", u_errorName(status));
    }
    ucurr_forLocale("en_GB_EURO", tmp, 4, &status);
    resultStr.setTo(tmp);
    if (euroStr != resultStr) {
        errcheckln(status, "Fail: en_GB_EURO didn't return EUR - %s", u_errorName(status));
    }
    ucurr_forLocale("en_US_PREEURO", tmp, 4, &status);
    resultStr.setTo(tmp);
    if (dollarStr != resultStr) {
        errcheckln(status, "Fail: en_US_PREEURO didn't fallback to en_US - %s", u_errorName(status));
    }
    ucurr_forLocale("en_US_Q", tmp, 4, &status);
    resultStr.setTo(tmp);
    if (dollarStr != resultStr) {
        errcheckln(status, "Fail: en_US_Q didn't fallback to en_US - %s", u_errorName(status));
    }
    int32_t invalidLen = ucurr_forLocale("en_QQ", tmp, 4, &status);
    if (invalidLen || U_SUCCESS(status)) {
        errln("Fail: en_QQ didn't return NULL");
    }
}

#endif





void
LocaleTest::TestToString() {
    Locale DATA [] = {
        Locale("xx", "", ""),
        Locale("", "YY", ""),
        Locale("", "", "ZZ"),
        Locale("xx", "YY", ""),
        Locale("xx", "", "ZZ"),
        Locale("", "YY", "ZZ"),
        Locale("xx", "YY", "ZZ"),
    };

    const char DATA_S [][20] = {
        "xx",
        "_YY",
        "__ZZ",
        "xx_YY",
        "xx__ZZ",
        "_YY_ZZ",
        "xx_YY_ZZ",
    };

    for (int32_t i=0; i < 7; ++i) {
      const char *name;
      name = DATA[i].getName();

      if (strcmp(name, DATA_S[i]) != 0)
        {
            errln("Fail: Locale.getName(), got:" + UnicodeString(name) + ", expected: " + DATA_S[i]);
        }
        else
            logln("Pass: Locale.getName(), got:" + UnicodeString(name) );
    }
}

#if !UCONFIG_NO_FORMATTING

















void
LocaleTest::Test4139940()
{
    Locale mylocale("hu", "", "");
    UDate mydate = date(98,3,13); 
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat df_full("EEEE", mylocale, status);
    if(U_FAILURE(status)){
        dataerrln(UnicodeString("Could not create SimpleDateFormat object for locale hu. Error: ") + UnicodeString(u_errorName(status)));
        return;
    }
    UnicodeString str;
    FieldPosition pos(FieldPosition::DONT_CARE);
    df_full.format(mydate, str, pos);
    
    
    UChar ocf = 0x00f4;
    UChar oda = 0x0151;
    if (str.indexOf(oda) < 0 || str.indexOf(ocf) >= 0) {
      
      if (strcmp(Locale::getDefault().getLanguage(), "th") != 0) {
        errln("Fail: Monday in Hungarian is wrong - oda's index is %d and ocf's is %d",
              str.indexOf(oda), str.indexOf(ocf));
      } else {
        logln(UnicodeString("An error is produce in buddhist calendar."));
      }
      logln(UnicodeString("String is: ") + str );
    }
}

UDate
LocaleTest::date(int32_t y, int32_t m, int32_t d, int32_t hr, int32_t min, int32_t sec)
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(status);
    if (cal == 0)
        return 0.0;
    cal->clear();
    cal->set(1900 + y, m, d, hr, min, sec); 
    UDate dt = cal->getTime(status);
    if (U_FAILURE(status))
        return 0.0;

    delete cal;
    return dt;
}





void
LocaleTest::Test4143951()
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(Locale("ru", "", ""), status);
    if(U_SUCCESS(status)) {
      if (cal->getFirstDayOfWeek(status) != UCAL_MONDAY) {
          dataerrln("Fail: First day of week in Russia should be Monday");
      }
    }
    delete cal;
}

#endif






void
LocaleTest::Test4147315()
{
  UnicodeString temp;
    
    
    Locale locale("xxx", "CCC");

    const char *result = locale.getISO3Country();

    
    if((result==NULL)||(result[0] != 0))
      errln("ERROR: getISO3Country() returns: " + UnicodeString(result,"") +
                " for locale '" + (temp=locale.getName()) + "' rather than exception" );
}






void
LocaleTest::Test4147317()
{
    UnicodeString temp;
    
    
    Locale locale("xxx", "CCC");

    const char *result = locale.getISO3Language();

    
    if((result==NULL)||(result[0] != 0))
      errln("ERROR: getISO3Language() returns: " + UnicodeString(result,"") +
                " for locale '" + (temp=locale.getName()) + "' rather than exception" );
}




void
LocaleTest::Test4147552()
{
    Locale locales [] = {     Locale("no", "NO"),
                            Locale("no", "NO", "B"),
                             Locale("no", "NO", "NY")
    };

    UnicodeString edn("Norwegian (Norway, B)");
    UnicodeString englishDisplayNames [] = {
                                                "Norwegian (Norway)",
                                                 edn,
                                                 
                                                 
                                                 "Norwegian (Norway, NY)"
    };
    UnicodeString ndn("norsk (Norge, B");
    UnicodeString norwegianDisplayNames [] = {
                                                "norsk (Norge)",
                                                "norsk (Norge, B)",
                                                
                                                 "norsk (Noreg, NY)"
                                                 
    };
    UErrorCode status = U_ZERO_ERROR;

    Locale saveLocale;
    Locale::setDefault(Locale::getEnglish(), status);
    for (int32_t i = 0; i < 3; ++i) {
        Locale loc = locales[i];
        UnicodeString temp;
        if (loc.getDisplayName(temp) != englishDisplayNames[i])
           dataerrln("English display-name mismatch: expected " +
                   englishDisplayNames[i] + ", got " + loc.getDisplayName(temp));
        if (loc.getDisplayName(loc, temp) != norwegianDisplayNames[i])
            dataerrln("Norwegian display-name mismatch: expected " +
                   norwegianDisplayNames[i] + ", got " +
                   loc.getDisplayName(loc, temp));
    }
    Locale::setDefault(saveLocale, status);
}

void
LocaleTest::TestVariantParsing()
{
    Locale en_US_custom("en", "US", "De Anza_Cupertino_California_United States_Earth");

    UnicodeString dispName("English (United States, DE ANZA_CUPERTINO_CALIFORNIA_UNITED STATES_EARTH)");
    UnicodeString dispVar("DE ANZA_CUPERTINO_CALIFORNIA_UNITED STATES_EARTH");

    UnicodeString got;

    en_US_custom.getDisplayVariant(Locale::getUS(), got);
    if(got != dispVar) {
        errln("FAIL: getDisplayVariant()");
        errln("Wanted: " + dispVar);
        errln("Got   : " + got);
    }

    en_US_custom.getDisplayName(Locale::getUS(), got);
    if(got != dispName) {
        dataerrln("FAIL: getDisplayName()");
        dataerrln("Wanted: " + dispName);
        dataerrln("Got   : " + got);
    }

    Locale shortVariant("fr", "FR", "foo");
    shortVariant.getDisplayVariant(got);

    if(got != "FOO") {
        errln("FAIL: getDisplayVariant()");
        errln("Wanted: foo");
        errln("Got   : " + got);
    }

    Locale bogusVariant("fr", "FR", "_foo");
    bogusVariant.getDisplayVariant(got);

    if(got != "FOO") {
        errln("FAIL: getDisplayVariant()");
        errln("Wanted: foo");
        errln("Got   : " + got);
    }

    Locale bogusVariant2("fr", "FR", "foo_");
    bogusVariant2.getDisplayVariant(got);

    if(got != "FOO") {
        errln("FAIL: getDisplayVariant()");
        errln("Wanted: foo");
        errln("Got   : " + got);
    }

    Locale bogusVariant3("fr", "FR", "_foo_");
    bogusVariant3.getDisplayVariant(got);

    if(got != "FOO") {
        errln("FAIL: getDisplayVariant()");
        errln("Wanted: foo");
        errln("Got   : " + got);
    }
}

#if !UCONFIG_NO_FORMATTING






void
LocaleTest::Test4105828()
{
    Locale LOC [] = { Locale::getChinese(),  Locale("zh", "CN", ""),
                     Locale("zh", "TW", ""), Locale("zh", "HK", "") };
    UErrorCode status = U_ZERO_ERROR;
    for (int32_t i = 0; i < 4; ++i) {
        NumberFormat *fmt = NumberFormat::createPercentInstance(LOC[i], status);
        if(U_FAILURE(status)) {
            dataerrln("Couldn't create NumberFormat - %s", u_errorName(status));
            return;
        }
        UnicodeString result;
        FieldPosition pos(0);
        fmt->format((int32_t)1, result, pos);
        UnicodeString temp;
        if(result != "100%") {
            errln(UnicodeString("Percent for ") + LOC[i].getDisplayName(temp) + " should be 100%, got " + result);
        }
        delete fmt;
    }
}

#endif



void
LocaleTest::TestSetIsBogus() {
    Locale l("en_US");
    l.setToBogus();
    if(l.isBogus() != TRUE) {
        errln("After setting bogus, didn't return TRUE");
    }
    l = "en_US"; 
    if(l.isBogus() != FALSE) {
        errln("After resetting bogus, didn't return FALSE");
    }
}


void
LocaleTest::TestKeywordVariants(void) {
    static const struct {
        const char *localeID;
        const char *expectedLocaleID;
        
        
        const char *expectedKeywords[10];
        int32_t numKeywords;
        UErrorCode expectedStatus;
    } testCases[] = {
        {
            "de_DE@  currency = euro; C o ll A t i o n   = Phonebook   ; C alen dar = buddhist   ", 
            "de_DE@calendar=buddhist;collation=Phonebook;currency=euro", 
            
            
            {"calendar", "collation", "currency"},
            3,
            U_ZERO_ERROR
        },
        {
            "de_DE@euro",
            "de_DE@euro",
            
            
            {"","","","","","",""},
            0,
            U_INVALID_FORMAT_ERROR 
        }
    };
    UErrorCode status = U_ZERO_ERROR;

    int32_t i = 0, j = 0;
    const char *result = NULL;
    StringEnumeration *keywords;
    int32_t keyCount = 0;
    const char *keyword = NULL;
    const UnicodeString *keywordString;
    int32_t keywordLen = 0;

    for(i = 0; i < (int32_t)(sizeof(testCases)/sizeof(testCases[0])); i++) {
        status = U_ZERO_ERROR;
        Locale l(testCases[i].localeID);
        keywords = l.createKeywords(status);

        if(status != testCases[i].expectedStatus) {
            err("Expected to get status %s. Got %s instead\n",
                u_errorName(testCases[i].expectedStatus), u_errorName(status));
        }
        status = U_ZERO_ERROR;
        if(keywords) {
            if((keyCount = keywords->count(status)) != testCases[i].numKeywords) {
                err("Expected to get %i keywords, got %i\n", testCases[i].numKeywords, keyCount);
            }
            if(keyCount) {
                for(j = 0;;) {
                    if((j&1)==0) {
                        if((keyword = keywords->next(&keywordLen, status)) == NULL) {
                            break;
                        }
                        if(strcmp(keyword, testCases[i].expectedKeywords[j]) != 0) {
                            err("Expected to get keyword value %s, got %s\n", testCases[i].expectedKeywords[j], keyword);
                        }
                    } else {
                        if((keywordString = keywords->snext(status)) == NULL) {
                            break;
                        }
                        if(*keywordString != UnicodeString(testCases[i].expectedKeywords[j], "")) {
                            err("Expected to get keyword UnicodeString %s, got %s\n", testCases[i].expectedKeywords[j], keyword);
                        }
                    }
                    j++;

                    if(j == keyCount / 2) {
                        
                        StringEnumeration *k2 = keywords->clone();
                        if(k2 == NULL || keyCount != k2->count(status)) {
                            errln("KeywordEnumeration.clone() failed");
                        } else {
                            delete keywords;
                            keywords = k2;
                        }
                    }
                }
                keywords->reset(status); 
                for(j = 0;;) {
                    if((keyword = keywords->next(&keywordLen, status)) == NULL) {
                        break;
                    }
                    if(strcmp(keyword, testCases[i].expectedKeywords[j]) != 0) {
                        err("Expected to get keyword value %s, got %s\n", testCases[i].expectedKeywords[j], keyword);
                    }
                    j++;
                }
            }
            delete keywords;
        }
        result = l.getName();
        if(uprv_strcmp(testCases[i].expectedLocaleID, result) != 0) {
            err("Expected to get \"%s\" from \"%s\". Got \"%s\" instead\n",
                testCases[i].expectedLocaleID, testCases[i].localeID, result);
        }

    }

}

void
LocaleTest::TestKeywordVariantParsing(void) {
    static const struct {
        const char *localeID;
        const char *keyword;
        const char *expectedValue;
    } testCases[] = {
        { "de_DE@  C o ll A t i o n   = Phonebook   ", "collation", "Phonebook" },
        { "de_DE", "collation", ""},
        { "de_DE@collation= PHONEBOOK", "collation", "PHONEBOOK" },
        { "de_DE@ currency = euro   ; CoLLaTion   = PHONEBOOk   ", "collation", "PHONEBOOk" },
    };

    UErrorCode status = U_ZERO_ERROR;

    int32_t i = 0;
    int32_t resultLen = 0;
    char buffer[256];

    for(i = 0; i < (int32_t)(sizeof(testCases)/sizeof(testCases[0])); i++) {
        *buffer = 0;
        Locale l(testCases[i].localeID);
        resultLen = l.getKeywordValue(testCases[i].keyword, buffer, 256, status);
        (void)resultLen;  
        if(uprv_strcmp(testCases[i].expectedValue, buffer) != 0) {
            err("Expected to extract \"%s\" from \"%s\" for keyword \"%s\". Got \"%s\" instead\n",
                testCases[i].expectedValue, testCases[i].localeID, testCases[i].keyword, buffer);
        }
    }
}

void
LocaleTest::TestSetKeywordValue(void) {
    static const struct {
        const char *keyword;
        const char *value;
    } testCases[] = {
        { "collation", "phonebook" },
        { "currency", "euro" },
        { "calendar", "buddhist" }
    };

    UErrorCode status = U_ZERO_ERROR;

    int32_t i = 0;
    int32_t resultLen = 0;
    char buffer[256];

    Locale l(Locale::getGerman());

    for(i = 0; i < (int32_t)(sizeof(testCases)/sizeof(testCases[0])); i++) {
        l.setKeywordValue(testCases[i].keyword, testCases[i].value, status);
        if(U_FAILURE(status)) {
            err("FAIL: Locale::setKeywordValue failed - %s\n", u_errorName(status));
        }

        *buffer = 0;
        resultLen = l.getKeywordValue(testCases[i].keyword, buffer, 256, status);
        (void)resultLen;  
        if(uprv_strcmp(testCases[i].value, buffer) != 0) {
            err("Expected to extract \"%s\" for keyword \"%s\". Got \"%s\" instead\n",
                testCases[i].value, testCases[i].keyword, buffer);
        }
    }
}

void
LocaleTest::TestGetBaseName(void) {
    static const struct {
        const char *localeID;
        const char *baseName;
    } testCases[] = {
        { "de_DE@  C o ll A t i o n   = Phonebook   ", "de_DE" },
        { "de@currency = euro; CoLLaTion   = PHONEBOOk", "de" },
        { "ja@calendar = buddhist", "ja" },
        { "de-u-co-phonebk", "de"}
    };

    int32_t i = 0;

    for(i = 0; i < UPRV_LENGTHOF(testCases); i++) {
        Locale loc(testCases[i].localeID);
        if(strcmp(testCases[i].baseName, loc.getBaseName())) {
            errln("For locale \"%s\" expected baseName \"%s\", but got \"%s\"",
                testCases[i].localeID, testCases[i].baseName, loc.getBaseName());
            return;
        }
    }

    
    UErrorCode status = U_ZERO_ERROR;
    Locale loc2("en-US");
    if (strcmp("en_US", loc2.getBaseName())) {
        errln("%s:%d Expected \"en_US\", got \"%s\"", __FILE__, __LINE__, loc2.getBaseName());
    }
    loc2.setKeywordValue("key", "value", status);
    if (strcmp("en_US@key=value", loc2.getName())) {
        errln("%s:%d Expected \"en_US@key=value\", got \"%s\"", __FILE__, __LINE__, loc2.getName());
    }
    if (strcmp("en_US", loc2.getBaseName())) {
        errln("%s:%d Expected \"en_US\", got \"%s\"", __FILE__, __LINE__, loc2.getBaseName());
    }
}






static UBool _loccmp(const char* string, const char* prefix) {
    int32_t slen = (int32_t)strlen(string),
            plen = (int32_t)strlen(prefix);
    int32_t c = uprv_strncmp(string, prefix, plen);
    
    if (uprv_strcmp(prefix, "root") == 0) {
        return (uprv_strcmp(string, "root") == 0) ? 0 : 1;
    }
    if (c) return -1; 
    if (slen == plen) return 0;
    if (string[plen] == '_') return 1;
    return -2; 
}










void LocaleTest::_checklocs(const char* label,
                            const char* req,
                            const Locale& validLoc,
                            const Locale& actualLoc,
                            const char* expReqValid,
                            const char* expValidActual) {
    const char* valid = validLoc.getName();
    const char* actual = actualLoc.getName();
    int32_t reqValid = _loccmp(req, valid);
    int32_t validActual = _loccmp(valid, actual);
    if (((0 == uprv_strcmp(expReqValid, "gt") && reqValid > 0) ||
         (0 == uprv_strcmp(expReqValid, "ge") && reqValid >= 0) ||
         (0 == uprv_strcmp(expReqValid, "eq") && reqValid == 0)) &&
        ((0 == uprv_strcmp(expValidActual, "gt") && validActual > 0) ||
         (0 == uprv_strcmp(expValidActual, "ge") && validActual >= 0) ||
         (0 == uprv_strcmp(expValidActual, "eq") && validActual == 0))) {
        logln("%s; req=%s, valid=%s, actual=%s",
              label, req, valid, actual);
    } else {
        dataerrln("FAIL: %s; req=%s, valid=%s, actual=%s.  Require (R %s V) and (V %s A)",
              label, req, valid, actual,
              expReqValid, expValidActual);
    }
}

void LocaleTest::TestGetLocale(void) {
#if !UCONFIG_NO_SERVICE
    const char *req;
    Locale valid, actual, reqLoc;
    
    
#if !UCONFIG_NO_FORMATTING
    {
        UErrorCode ec = U_ZERO_ERROR;  
        req = "en_US_BROOKLYN";
        Calendar* cal = Calendar::createInstance(Locale::createFromName(req), ec);
        if (U_FAILURE(ec)) {
            dataerrln("FAIL: Calendar::createInstance failed - %s", u_errorName(ec));
        } else {
            valid = cal->getLocale(ULOC_VALID_LOCALE, ec);
            actual = cal->getLocale(ULOC_ACTUAL_LOCALE, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: Calendar::getLocale() failed");
            } else {
                _checklocs("Calendar", req, valid, actual);
            }
            
            ec = U_FILE_ACCESS_ERROR;
            if (cal->getLocale(ULOC_VALID_LOCALE, ec).getName()[0] != 0) {
                errln("FAIL: Calendar::getLocale() failed to fail correctly. It should have returned \"\"");
            }
            ec = U_ZERO_ERROR;
        }
        delete cal;
    }
#endif

    
#if !UCONFIG_NO_FORMATTING
    {
        UErrorCode ec = U_ZERO_ERROR;  
        req = "fr_FR_NICE";
        NumberFormat* nf = NumberFormat::createInstance(Locale::createFromName(req), ec);
        if (U_FAILURE(ec)) {
            dataerrln("FAIL: NumberFormat::createInstance failed - %s", u_errorName(ec));
        } else {
            DecimalFormat* dec = dynamic_cast<DecimalFormat*>(nf);
            if (dec == NULL) {
                errln("FAIL: NumberFormat::createInstance does not return a DecimalFormat");
                return;
            }
            valid = dec->getLocale(ULOC_VALID_LOCALE, ec);
            actual = dec->getLocale(ULOC_ACTUAL_LOCALE, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: DecimalFormat::getLocale() failed");
            } else {
                _checklocs("DecimalFormat", req, valid, actual);
            }

            const DecimalFormatSymbols* sym = dec->getDecimalFormatSymbols();
            if (sym == NULL) {
                errln("FAIL: getDecimalFormatSymbols returned NULL");
                return;
            }
            valid = sym->getLocale(ULOC_VALID_LOCALE, ec);
            actual = sym->getLocale(ULOC_ACTUAL_LOCALE, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: DecimalFormatSymbols::getLocale() failed");
            } else {
                _checklocs("DecimalFormatSymbols", req, valid, actual);
            }        
        }
        delete nf;
    }
#endif

    
#if !UCONFIG_NO_FORMATTING
    {
        UErrorCode ec = U_ZERO_ERROR;  
        req = "de_CH_LUCERNE";
        DateFormat* df =
            DateFormat::createDateInstance(DateFormat::kDefault,
                                           Locale::createFromName(req));
        if (df == 0){
            dataerrln("Error calling DateFormat::createDateInstance()");
        } else {
            SimpleDateFormat* dat = dynamic_cast<SimpleDateFormat*>(df);
            if (dat == NULL) {
                errln("FAIL: DateFormat::createInstance does not return a SimpleDateFormat");
                return;
            }
            valid = dat->getLocale(ULOC_VALID_LOCALE, ec);
            actual = dat->getLocale(ULOC_ACTUAL_LOCALE, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: SimpleDateFormat::getLocale() failed");
            } else {
                _checklocs("SimpleDateFormat", req, valid, actual);
            }
    
            const DateFormatSymbols* sym = dat->getDateFormatSymbols();
            if (sym == NULL) {
                errln("FAIL: getDateFormatSymbols returned NULL");
                return;
            }
            valid = sym->getLocale(ULOC_VALID_LOCALE, ec);
            actual = sym->getLocale(ULOC_ACTUAL_LOCALE, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: DateFormatSymbols::getLocale() failed");
            } else {
                _checklocs("DateFormatSymbols", req, valid, actual);
            }        
        }
        delete df;
    }
#endif

    
#if !UCONFIG_NO_BREAK_ITERATION
    {
        UErrorCode ec = U_ZERO_ERROR;  
        req = "es_ES_BARCELONA";
        reqLoc = Locale::createFromName(req);
        BreakIterator* brk = BreakIterator::createWordInstance(reqLoc, ec);
        if (U_FAILURE(ec)) {
            dataerrln("FAIL: BreakIterator::createWordInstance failed - %s", u_errorName(ec));
        } else {
            valid = brk->getLocale(ULOC_VALID_LOCALE, ec);
            actual = brk->getLocale(ULOC_ACTUAL_LOCALE, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: BreakIterator::getLocale() failed");
            } else {
                _checklocs("BreakIterator", req, valid, actual);
            }
        
            
            URegistryKey key = BreakIterator::registerInstance(brk, reqLoc, UBRK_WORD, ec);
            brk = 0; 
            if (U_FAILURE(ec)) {
                errln("FAIL: BreakIterator::registerInstance() failed");
            } else {
                brk = BreakIterator::createWordInstance(reqLoc, ec);
                if (U_FAILURE(ec)) {
                    errln("FAIL: BreakIterator::createWordInstance failed");
                } else {
                    valid = brk->getLocale(ULOC_VALID_LOCALE, ec);
                    actual = brk->getLocale(ULOC_ACTUAL_LOCALE, ec);
                    if (U_FAILURE(ec)) {
                        errln("FAIL: BreakIterator::getLocale() failed");
                    } else {
                        
                        _checklocs("BreakIterator(registered)",
                                   req, valid, actual, "eq", "eq");
                    }
                }
                
                BreakIterator::unregister(key, ec);
                if (U_FAILURE(ec)) {
                    errln("FAIL: BreakIterator::unregister() failed");
                }
                delete brk;
                brk = 0;
            }

            
            brk = BreakIterator::createWordInstance(reqLoc, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: BreakIterator::createWordInstance failed");
            } else {
                valid = brk->getLocale(ULOC_VALID_LOCALE, ec);
                actual = brk->getLocale(ULOC_ACTUAL_LOCALE, ec);
                if (U_FAILURE(ec)) {
                    errln("FAIL: BreakIterator::getLocale() failed");
                } else {
                    _checklocs("BreakIterator(unregistered)", req, valid, actual);
                }
            }
        }
        delete brk;
    }
#endif

    
#if !UCONFIG_NO_COLLATION
    {
        UErrorCode ec = U_ZERO_ERROR;  

        checkRegisteredCollators(NULL); 

        req = "hi_IN_BHOPAL";
        reqLoc = Locale::createFromName(req);
        Collator* coll = Collator::createInstance(reqLoc, ec);
        if (U_FAILURE(ec)) {
            dataerrln("FAIL: Collator::createInstance failed - %s", u_errorName(ec));
        } else {
            valid = coll->getLocale(ULOC_VALID_LOCALE, ec);
            actual = coll->getLocale(ULOC_ACTUAL_LOCALE, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: Collator::getLocale() failed");
            } else {
                _checklocs("Collator", req, valid, actual);
            }

            
            URegistryKey key = Collator::registerInstance(coll, reqLoc, ec);
            coll = 0; 
            if (U_FAILURE(ec)) {
                errln("FAIL: Collator::registerInstance() failed");
            } else {
                coll = Collator::createInstance(reqLoc, ec);
                if (U_FAILURE(ec)) {
                    errln("FAIL: Collator::createWordInstance failed");
                } else {
                    valid = coll->getLocale(ULOC_VALID_LOCALE, ec);
                    actual = coll->getLocale(ULOC_ACTUAL_LOCALE, ec);
                    if (U_FAILURE(ec)) {
                        errln("FAIL: Collator::getLocale() failed");
                    } else {
                        
                        _checklocs("Collator(registered)",
                                   req, valid, actual, "eq", "eq");
                    }
                }
                checkRegisteredCollators(req); 

                
                Collator::unregister(key, ec);
                if (U_FAILURE(ec)) {
                    errln("FAIL: Collator::unregister() failed");
                }
                delete coll;
                coll = 0;
            }

            
            coll = Collator::createInstance(reqLoc, ec);
            if (U_FAILURE(ec)) {
                errln("FAIL: Collator::createInstance failed");
            } else {
                valid = coll->getLocale(ULOC_VALID_LOCALE, ec);
                actual = coll->getLocale(ULOC_ACTUAL_LOCALE, ec);
                if (U_FAILURE(ec)) {
                    errln("FAIL: Collator::getLocale() failed");
                } else {
                    _checklocs("Collator(unregistered)", req, valid, actual);
                }
            }
        }
        delete coll;

        checkRegisteredCollators(NULL); 
    }
#endif
#endif
}

#if !UCONFIG_NO_COLLATION








void LocaleTest::checkRegisteredCollators(const char *expectExtra) {
    UErrorCode status = U_ZERO_ERROR;
    int32_t count1=0,count2=0;
    Hashtable oldHash(status);
    Hashtable newHash(status);
    TEST_ASSERT_STATUS(status);

    UnicodeString expectStr(expectExtra?expectExtra:"n/a", "");

    
    const Locale*  oldList = Collator::getAvailableLocales(count1);
    if(oldList == NULL) {
        dataerrln("Error: Collator::getAvailableLocales(count) returned NULL");
        return;
    }

    
    LocalPointer<StringEnumeration> newEnum(Collator::getAvailableLocales());
    if(newEnum.isNull()) {
       errln("Error: collator::getAvailableLocales() returned NULL");
       return;
    }

    
    
    

    
    for(int32_t i=0;i<count1;i++) {
        const UnicodeString key(oldList[i].getName(), "");
        int32_t oldI = oldHash.puti(key, 1, status);
        if( oldI == 1 ){
            errln("Error: duplicate key %s in Collator::getAvailableLocales(count) list.\n",
                oldList[i].getName());
            return;
        }
        if(expectExtra != NULL && !strcmp(expectExtra, oldList[i].getName())) {
            errln("Inexplicably, Collator::getAvailableCollators(count) had registered collator %s. This shouldn't happen, so I am going to consider it an error.\n", expectExtra);
        }
    }

    
    const UnicodeString *locStr;
    UBool foundExpected = FALSE;
    while((locStr = newEnum->snext(status)) && U_SUCCESS(status)) {
        count2++;

        if(expectExtra != NULL && expectStr == *locStr) {
            foundExpected = TRUE;
            logln(UnicodeString("Found expected registered collator: ","") + expectStr);
        }
        (void)foundExpected;    

        if( oldHash.geti(*locStr) == 0 ) {
            if(expectExtra != NULL && expectStr==*locStr) {
                logln(UnicodeString("As expected, Collator::getAvailableLocales(count) is missing registered collator ") + expectStr);
            } else {
                errln(UnicodeString("Error: Collator::getAvailableLocales(count) is missing: ","")
                    + *locStr);
            }
        }
        newHash.puti(*locStr, 1, status);
    }

    
    for(int32_t i=0;i<count1;i++) {
        const UnicodeString key(oldList[i].getName(), "");
        int32_t newI = newHash.geti(key);
        if(newI == 0) {
            errln(UnicodeString("Error: Collator::getAvailableLocales() is missing: ","")
                + key);
        }
    }

    int32_t expectCount2 = count1;
    if(expectExtra != NULL) {
        expectCount2 ++; 
    }

    assertEquals("Collator::getAvail() count", expectCount2, count2);
}
#endif



void LocaleTest::TestVariantWithOutCountry(void) {
    Locale loc("en","","POSIX");
    if (0 != strcmp(loc.getVariant(), "POSIX")) {
        errln("FAIL: en__POSIX didn't get parsed correctly - name is %s - expected %s got %s", loc.getName(), "POSIX", loc.getVariant());
    }
    Locale loc2("en","","FOUR");
    if (0 != strcmp(loc2.getVariant(), "FOUR")) {
        errln("FAIL: en__FOUR didn't get parsed correctly - name is %s - expected %s got %s", loc2.getName(), "FOUR", loc2.getVariant());
    }
    Locale loc3("en","Latn","","FOUR");
    if (0 != strcmp(loc3.getVariant(), "FOUR")) {
        errln("FAIL: en_Latn__FOUR didn't get parsed correctly - name is %s - expected %s got %s", loc3.getName(), "FOUR", loc3.getVariant());
    }
    Locale loc4("","Latn","","FOUR");
    if (0 != strcmp(loc4.getVariant(), "FOUR")) {
        errln("FAIL: _Latn__FOUR didn't get parsed correctly - name is %s - expected %s got %s", loc4.getName(), "FOUR", loc4.getVariant());
    }
    Locale loc5("","Latn","US","FOUR");
    if (0 != strcmp(loc5.getVariant(), "FOUR")) {
        errln("FAIL: _Latn_US_FOUR didn't get parsed correctly - name is %s - expected %s got %s", loc5.getName(), "FOUR", loc5.getVariant());
    }
    Locale loc6("de-1901");
    if (0 != strcmp(loc6.getVariant(), "1901")) {
        errln("FAIL: de-1901 didn't get parsed correctly - name is %s - expected %s got %s", loc6.getName(), "1901", loc6.getVariant());
    }
}

static Locale _canonicalize(int32_t selector, 
                            const char* localeID) {
    switch (selector) {
    case 0:
        return Locale::createFromName(localeID);
    case 1:
        return Locale::createCanonical(localeID);
    case 2:
        return Locale(localeID);
    default:
        return Locale("");
    }
}

void LocaleTest::TestCanonicalization(void)
{
    static const struct {
        const char *localeID;    
        const char *getNameID;   
        const char *canonicalID; 
    } testCases[] = {
        { "", "", "en_US_POSIX" },
        { "C", "c", "en_US_POSIX" },
        { "POSIX", "posix", "en_US_POSIX" },
        { "ca_ES_PREEURO-with-extra-stuff-that really doesn't make any sense-unless-you're trying to increase code coverage",
          "ca_ES_PREEURO_WITH_EXTRA_STUFF_THAT REALLY DOESN'T MAKE ANY SENSE_UNLESS_YOU'RE TRYING TO INCREASE CODE COVERAGE",
          "ca_ES_PREEURO_WITH_EXTRA_STUFF_THAT REALLY DOESN'T MAKE ANY SENSE_UNLESS_YOU'RE TRYING TO INCREASE CODE COVERAGE"},
        { "ca_ES_PREEURO", "ca_ES_PREEURO", "ca_ES@currency=ESP" },
        { "de_AT_PREEURO", "de_AT_PREEURO", "de_AT@currency=ATS" },
        { "de_DE_PREEURO", "de_DE_PREEURO", "de_DE@currency=DEM" },
        { "de_LU_PREEURO", "de_LU_PREEURO", "de_LU@currency=LUF" },
        { "el_GR_PREEURO", "el_GR_PREEURO", "el_GR@currency=GRD" },
        { "en_BE_PREEURO", "en_BE_PREEURO", "en_BE@currency=BEF" },
        { "en_IE_PREEURO", "en_IE_PREEURO", "en_IE@currency=IEP" },
        { "es_ES_PREEURO", "es_ES_PREEURO", "es_ES@currency=ESP" },
        { "eu_ES_PREEURO", "eu_ES_PREEURO", "eu_ES@currency=ESP" },
        { "fi_FI_PREEURO", "fi_FI_PREEURO", "fi_FI@currency=FIM" },
        { "fr_BE_PREEURO", "fr_BE_PREEURO", "fr_BE@currency=BEF" },
        { "fr_FR_PREEURO", "fr_FR_PREEURO", "fr_FR@currency=FRF" },
        { "fr_LU_PREEURO", "fr_LU_PREEURO", "fr_LU@currency=LUF" },
        { "ga_IE_PREEURO", "ga_IE_PREEURO", "ga_IE@currency=IEP" },
        { "gl_ES_PREEURO", "gl_ES_PREEURO", "gl_ES@currency=ESP" },
        { "it_IT_PREEURO", "it_IT_PREEURO", "it_IT@currency=ITL" },
        { "nl_BE_PREEURO", "nl_BE_PREEURO", "nl_BE@currency=BEF" },
        { "nl_NL_PREEURO", "nl_NL_PREEURO", "nl_NL@currency=NLG" },
        { "pt_PT_PREEURO", "pt_PT_PREEURO", "pt_PT@currency=PTE" },
        { "de__PHONEBOOK", "de__PHONEBOOK", "de@collation=phonebook" },
        { "en_GB_EURO", "en_GB_EURO", "en_GB@currency=EUR" },
        { "en_GB@EURO", "en_GB@EURO", "en_GB@currency=EUR" }, 
        { "es__TRADITIONAL", "es__TRADITIONAL", "es@collation=traditional" },
        { "hi__DIRECT", "hi__DIRECT", "hi@collation=direct" },
        { "ja_JP_TRADITIONAL", "ja_JP_TRADITIONAL", "ja_JP@calendar=japanese" },
        { "th_TH_TRADITIONAL", "th_TH_TRADITIONAL", "th_TH@calendar=buddhist" },
        { "zh_TW_STROKE", "zh_TW_STROKE", "zh_TW@collation=stroke" },
        { "zh__PINYIN", "zh__PINYIN", "zh@collation=pinyin" },
        { "zh@collation=pinyin", "zh@collation=pinyin", "zh@collation=pinyin" },
        { "zh_CN@collation=pinyin", "zh_CN@collation=pinyin", "zh_CN@collation=pinyin" },
        { "zh_CN_CA@collation=pinyin", "zh_CN_CA@collation=pinyin", "zh_CN_CA@collation=pinyin" },
        { "en_US_POSIX", "en_US_POSIX", "en_US_POSIX" }, 
        { "hy_AM_REVISED", "hy_AM_REVISED", "hy_AM_REVISED" }, 
        { "no_NO_NY", "no_NO_NY", "no_NO_NY"  },
        { "no@ny", "no@ny", "no__NY"  }, 
        { "no-no.utf32@B", "no_NO.utf32@B", "no_NO_B"  }, 
        { "qz-qz@Euro", "qz_QZ@Euro", "qz_QZ@currency=EUR" }, 
        
        
        { "en-BOONT", "en__BOONT", "en__BOONT" }, 
        { "de-1901", "de__1901", "de__1901" }, 
        { "de-1906", "de__1906", "de__1906" }, 
        { "sr-SP-Cyrl", "sr_SP_CYRL", "sr_Cyrl_RS" }, 
        { "sr-SP-Latn", "sr_SP_LATN", "sr_Latn_RS" }, 
        { "sr_YU_CYRILLIC", "sr_YU_CYRILLIC", "sr_Cyrl_RS" }, 
        { "uz-UZ-Cyrl", "uz_UZ_CYRL", "uz_Cyrl_UZ" }, 
        { "uz-UZ-Latn", "uz_UZ_LATN", "uz_Latn_UZ" }, 
        { "zh-CHS", "zh_CHS", "zh_Hans" }, 
        { "zh-CHT", "zh_CHT", "zh_Hant" }, 

        
        { "mr.utf8", "mr.utf8", "mr" },
        { "de-tv.koi8r", "de_TV.koi8r", "de_TV" },
        { "x-piglatin_ML.MBE", "x-piglatin_ML.MBE", "x-piglatin_ML" },
        { "i-cherokee_US.utf7", "i-cherokee_US.utf7", "i-cherokee_US" },
        { "x-filfli_MT_FILFLA.gb-18030", "x-filfli_MT_FILFLA.gb-18030", "x-filfli_MT_FILFLA" },
        { "no-no-ny.utf8@B", "no_NO_NY.utf8@B", "no_NO_NY_B"  }, 

        
        
        { "en_Hant_IL_VALLEY_GIRL@ currency = EUR; calendar = Japanese ;", "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR", "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR" },
        
        { "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR", "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR", "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR" },
        
        { "es_ES_PREEURO@CALendar=Japanese", "es_ES_PREEURO@calendar=Japanese", "es_ES@calendar=Japanese;currency=ESP" },
        { "es_ES_EURO@SHOUT=zipeedeedoodah", "es_ES_EURO@shout=zipeedeedoodah", "es_ES@currency=EUR;shout=zipeedeedoodah" },
        
        { "es_ES_PREEURO@currency=EUR", "es_ES_PREEURO@currency=EUR", "es_ES@currency=EUR" },
        { "es_ES_EURO@currency=ESP", "es_ES_EURO@currency=ESP", "es_ES@currency=ESP" },
        
        { "no-Hant-GB_NY@currency=$$$", "no_Hant_GB_NY@currency=$$$", "no_Hant_GB_NY@currency=$$$"  },

        
        { "root@kw=foo", "root@kw=foo", "root@kw=foo" },
        { "@calendar=gregorian", "@calendar=gregorian", "@calendar=gregorian" },
        { "ja_JP@calendar=Japanese", "ja_JP@calendar=Japanese", "ja_JP@calendar=Japanese" }
    };
    
    static const char* label[] = { "createFromName", "createCanonical", "Locale" };

    int32_t i, j;
    
    for (i=0; i < (int)(sizeof(testCases)/sizeof(testCases[0])); i++) {
        for (j=0; j<3; ++j) {
            const char* expected = (j==1) ? testCases[i].canonicalID : testCases[i].getNameID;
            Locale loc = _canonicalize(j, testCases[i].localeID);
            const char* getName = loc.isBogus() ? "BOGUS" : loc.getName();
            if(uprv_strcmp(expected, getName) != 0) {
                errln("FAIL: %s(%s).getName() => \"%s\", expected \"%s\"",
                      label[j], testCases[i].localeID, getName, expected);
            } else {
                logln("Ok: %s(%s) => \"%s\"",
                      label[j], testCases[i].localeID, getName);
            }
        }
    }
}

void LocaleTest::TestCurrencyByDate(void)
{
#if !UCONFIG_NO_FORMATTING
    UErrorCode status = U_ZERO_ERROR;
    UDate date = uprv_getUTCtime();
	UChar TMP[4];
	int32_t index = 0;
	int32_t resLen = 0;
    UnicodeString tempStr, resultStr;

	
    date = (UDate)-630720000000.0; 
    index = ucurr_countCurrencies("eo_AM", date, &status);
    if (index != 0)
	{
		errcheckln(status, "FAIL: didn't return 0 for eo_AM - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AM", date, index, TMP, 4, &status);
    if (resLen != 0) {
		errcheckln(status, "FAIL: eo_AM didn't return NULL - %s", u_errorName(status));
    }
    status = U_ZERO_ERROR;

    date = (UDate)0.0; 
    index = ucurr_countCurrencies("eo_AM", date, &status);
    if (index != 1)
	{
		errcheckln(status, "FAIL: didn't return 1 for eo_AM - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AM", date, index, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("SUR");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return SUR for eo_AM - %s", u_errorName(status));
    }

    date = (UDate)693792000000.0; 
	index = ucurr_countCurrencies("eo_AM", date, &status);
    if (index != 1)
	{
		errcheckln(status, "FAIL: didn't return 1 for eo_AM - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AM", date, index, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("RUR");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return RUR for eo_AM - %s", u_errorName(status));
    }

	date = (UDate)977616000000.0; 
	index = ucurr_countCurrencies("eo_AM", date, &status);
    if (index != 1)
	{
		errcheckln(status, "FAIL: didn't return 1 for eo_AM - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AM", date, index, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("AMD");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return AMD for eo_AM - %s", u_errorName(status));
    }

    
	date = (UDate)977616000000.0; 
	index = ucurr_countCurrencies("eo_AD", date, &status);
    if (index != 4)
	{
		errcheckln(status, "FAIL: didn't return 4 for eo_AD - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 1, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("EUR");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return EUR for eo_AD - %s", u_errorName(status));
    }
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 2, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("ESP");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return ESP for eo_AD - %s", u_errorName(status));
    }
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 3, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("FRF");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return FRF for eo_AD - %s", u_errorName(status));
    }
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 4, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("ADP");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return ADP for eo_AD - %s", u_errorName(status));
    }

	date = (UDate)0.0; 
	index = ucurr_countCurrencies("eo_AD", date, &status);
    if (index != 3)
	{
		errcheckln(status, "FAIL: didn't return 3 for eo_AD - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 1, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("ESP");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return ESP for eo_AD - %s", u_errorName(status));
    }
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 2, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("FRF");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return FRF for eo_AD - %s", u_errorName(status));
    }
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 3, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("ADP");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return ADP for eo_AD - %s", u_errorName(status));
    }

	date = (UDate)-630720000000.0; 
	index = ucurr_countCurrencies("eo_AD", date, &status);
    if (index != 2)
	{
		errcheckln(status, "FAIL: didn't return 2 for eo_AD - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 1, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("ESP");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return ESP for eo_AD - %s", u_errorName(status));
    }
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 2, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("ADP");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return ADP for eo_AD - %s", u_errorName(status));
    }

	date = (UDate)-2207520000000.0; 
	index = ucurr_countCurrencies("eo_AD", date, &status);
    if (index != 1)
	{
		errcheckln(status, "FAIL: didn't return 1 for eo_AD - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AD", date, 1, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("ESP");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return ESP for eo_AD - %s", u_errorName(status));
    }

	
	date = (UDate)788400000000.0;
	index = ucurr_countCurrencies("eo_UA", date, &status);
    if (index != 0)
	{
		errcheckln(status, "FAIL: didn't return 0 for eo_UA - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_UA", date, index, TMP, 4, &status);
    if (resLen != 0) {
		errcheckln(status, "FAIL: eo_UA didn't return NULL - %s", u_errorName(status));
    }
    status = U_ZERO_ERROR;

	
    resLen = ucurr_forLocaleAndDate("eo_UA", date, 100, TMP, 4, &status);
    if (resLen != 0) {
		errcheckln(status, "FAIL: eo_UA didn't return NULL - %s", u_errorName(status));
    }
    status = U_ZERO_ERROR;

    resLen = ucurr_forLocaleAndDate("eo_UA", date, 0, TMP, 4, &status);
    if (resLen != 0) {
		errcheckln(status, "FAIL: eo_UA didn't return NULL - %s", u_errorName(status));
    }
    status = U_ZERO_ERROR;

	
	index = ucurr_countCurrencies("eo_QQ", date, &status);
    if (index != 0)
	{
		errcheckln(status, "FAIL: didn't return 0 for eo_QQ - %s", u_errorName(status));
	}
    status = U_ZERO_ERROR;
    resLen = ucurr_forLocaleAndDate("eo_QQ", date, 1, TMP, 4, &status);
    if (resLen != 0) {
		errcheckln(status, "FAIL: eo_QQ didn't return NULL - %s", u_errorName(status));
    }
    status = U_ZERO_ERROR;
    resLen = ucurr_forLocaleAndDate("eo_QQ", date, 0, TMP, 4, &status);
    if (resLen != 0) {
		errcheckln(status, "FAIL: eo_QQ didn't return NULL - %s", u_errorName(status));
    }
    status = U_ZERO_ERROR;

    
	date = (UDate)977616000000.0; 
	index = ucurr_countCurrencies("eo_AO", date, &status);
    if (index != 1)
	{
		errcheckln(status, "FAIL: didn't return 1 for eo_AO - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AO", date, 1, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("AOA");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return AOA for eo_AO - %s", u_errorName(status));
    }

	date = (UDate)819936000000.0; 
	index = ucurr_countCurrencies("eo_AO", date, &status);
    if (index != 2)
	{
		errcheckln(status, "FAIL: didn't return 1 for eo_AO - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AO", date, 1, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("AOR");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return AOR for eo_AO - %s", u_errorName(status));
    }
    resLen = ucurr_forLocaleAndDate("eo_AO", date, 2, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("AON");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return AON for eo_AO - %s", u_errorName(status));
    }

	date = (UDate)662256000000.0; 
	index = ucurr_countCurrencies("eo_AO", date, &status);
    if (index != 2)
	{
		errcheckln(status, "FAIL: didn't return 1 for eo_AO - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AO", date, 1, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("AON");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return AON for eo_AO - %s", u_errorName(status));
    }
    resLen = ucurr_forLocaleAndDate("eo_AO", date, 2, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("AOK");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return AOK for eo_AO - %s", u_errorName(status));
    }

	date = (UDate)315360000000.0; 
	index = ucurr_countCurrencies("eo_AO", date, &status);
    if (index != 1)
	{
		errcheckln(status, "FAIL: didn't return 1 for eo_AO - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AO", date, 1, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("AOK");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return AOK for eo_AO - %s", u_errorName(status));
    }

	date = (UDate)0.0; 
	index = ucurr_countCurrencies("eo_AO", date, &status);
    if (index != 0)
	{
		errcheckln(status, "FAIL: didn't return 1 for eo_AO - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_AO", date, 1, TMP, 4, &status);
    if (resLen != 0) {
		errcheckln(status, "FAIL: eo_AO didn't return NULL - %s", u_errorName(status));
    }
    status = U_ZERO_ERROR;

    
	date = (UDate)977616000000.0; 
	index = ucurr_countCurrencies("eo_DE@currency=DEM", date, &status);
    if (index != 2)
	{
		errcheckln(status, "FAIL: didn't return 2 for eo_DE@currency=DEM - %s", u_errorName(status));
	}
    resLen = ucurr_forLocaleAndDate("eo_DE@currency=DEM", date, 1, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("EUR");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return EUR for eo_DE@currency=DEM - %s", u_errorName(status));
    }
    resLen = ucurr_forLocaleAndDate("eo_DE@currency=DEM", date, 2, TMP, 4, &status);
	tempStr.setTo(TMP);
    resultStr.setTo("DEM");
    if (resultStr != tempStr) {
        errcheckln(status, "FAIL: didn't return DEM for eo_DE@currency=DEM - %s", u_errorName(status));
    }

    
	status = U_ZERO_ERROR; 
    date = uprv_getUTCtime();

    UChar USD[4];
    ucurr_forLocaleAndDate("en_US", date, 1, USD, 4, &status);
    
	UChar YEN[4];
    ucurr_forLocaleAndDate("ja_JP", date, 1, YEN, 4, &status);

    ucurr_forLocaleAndDate("en_US", date, 1, TMP, 4, &status);
    if (u_strcmp(USD, TMP) != 0) {
        errcheckln(status, "Fail: en_US didn't return USD - %s", u_errorName(status));
    }
    ucurr_forLocaleAndDate("en_US_PREEURO", date, 1, TMP, 4, &status);
    if (u_strcmp(USD, TMP) != 0) {
        errcheckln(status, "Fail: en_US_PREEURO didn't fallback to en_US - %s", u_errorName(status));
    }
    ucurr_forLocaleAndDate("en_US_Q", date, 1, TMP, 4, &status);
    if (u_strcmp(USD, TMP) != 0) {
        errcheckln(status, "Fail: en_US_Q didn't fallback to en_US - %s", u_errorName(status));
    }
    status = U_ZERO_ERROR; 
#endif
}

void LocaleTest::TestGetVariantWithKeywords(void)
{
  Locale l("en_US_VALLEY@foo=value");
  const char *variant = l.getVariant();
  logln(variant);
  test_assert(strcmp("VALLEY", variant) == 0);

  UErrorCode status = U_ZERO_ERROR;
  char buffer[50];
  int32_t len = l.getKeywordValue("foo", buffer, 50, status);
  buffer[len] = '\0';
  test_assert(strcmp("value", buffer) == 0);
}

void LocaleTest::TestIsRightToLeft() {
    assertFalse("root LTR", Locale::getRoot().isRightToLeft());
    assertFalse("zh LTR", Locale::getChinese().isRightToLeft());
    assertTrue("ar RTL", Locale("ar").isRightToLeft());
    assertTrue("und-EG RTL", Locale("und-EG").isRightToLeft(), FALSE, TRUE);
    assertFalse("fa-Cyrl LTR", Locale("fa-Cyrl").isRightToLeft());
    assertTrue("en-Hebr RTL", Locale("en-Hebr").isRightToLeft());
    assertTrue("ckb RTL", Locale("ckb").isRightToLeft(), FALSE, TRUE);  
    assertFalse("fil LTR", Locale("fil").isRightToLeft());
    assertFalse("he-Zyxw LTR", Locale("he-Zyxw").isRightToLeft());
}

void LocaleTest::TestBug11421() {
    Locale::getDefault().getBaseName();
    int32_t numLocales;
    const Locale *localeList = Locale::getAvailableLocales(numLocales);
    for (int localeIndex = 0; localeIndex < numLocales; localeIndex++) {
        const Locale &loc = localeList[localeIndex];
        if (strncmp(loc.getName(), loc.getBaseName(), strlen(loc.getBaseName()))) {
            errln("%s:%d loc.getName=\"%s\"; loc.getBaseName=\"%s\"",
                __FILE__, __LINE__, loc.getName(), loc.getBaseName());
            break;
        }
    }
}
