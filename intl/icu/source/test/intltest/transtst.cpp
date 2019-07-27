









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "transtst.h"
#include "unicode/locid.h"
#include "unicode/dtfmtsym.h"
#include "unicode/normlzr.h"
#include "unicode/translit.h"
#include "unicode/uchar.h"
#include "unicode/unifilt.h"
#include "unicode/uniset.h"
#include "unicode/ustring.h"
#include "unicode/usetiter.h"
#include "unicode/uscript.h"
#include "unicode/utf16.h"
#include "cpdtrans.h"
#include "nultrans.h"
#include "rbt.h"
#include "rbt_pars.h"
#include "anytrans.h"
#include "esctrn.h"
#include "name2uni.h"
#include "nortrans.h"
#include "remtrans.h"
#include "titletrn.h"
#include "tolowtrn.h"
#include "toupptrn.h"
#include "unesctrn.h"
#include "uni2name.h"
#include "cstring.h"
#include "cmemory.h"
#include <stdio.h>















































enum {
    LEFT_BRACE=((UChar)0x007B), 
    PIPE      =((UChar)0x007C), 
    ZERO      =((UChar)0x0030), 
    UPPER_A   =((UChar)0x0041)  
};

TransliteratorTest::TransliteratorTest()
:   DESERET_DEE((UChar32)0x10414),
    DESERET_dee((UChar32)0x1043C)
{
}

TransliteratorTest::~TransliteratorTest() {}

void
TransliteratorTest::runIndexedTest(int32_t index, UBool exec,
                                   const char* &name, char* ) {
    switch (index) {
        TESTCASE(0,TestInstantiation);
        TESTCASE(1,TestSimpleRules);
        TESTCASE(2,TestRuleBasedInverse);
        TESTCASE(3,TestKeyboard);
        TESTCASE(4,TestKeyboard2);
        TESTCASE(5,TestKeyboard3);
        TESTCASE(6,TestArabic);
        TESTCASE(7,TestCompoundKana);
        TESTCASE(8,TestCompoundHex);
        TESTCASE(9,TestFiltering);
        TESTCASE(10,TestInlineSet);
        TESTCASE(11,TestPatternQuoting);
        TESTCASE(12,TestJ277);
        TESTCASE(13,TestJ243);
        TESTCASE(14,TestJ329);
        TESTCASE(15,TestSegments);
        TESTCASE(16,TestCursorOffset);
        TESTCASE(17,TestArbitraryVariableValues);
        TESTCASE(18,TestPositionHandling);
        TESTCASE(19,TestHiraganaKatakana);
        TESTCASE(20,TestCopyJ476);
        TESTCASE(21,TestAnchors);
        TESTCASE(22,TestInterIndic);
        TESTCASE(23,TestFilterIDs);
        TESTCASE(24,TestCaseMap);
        TESTCASE(25,TestNameMap);
        TESTCASE(26,TestLiberalizedID);
        TESTCASE(27,TestCreateInstance);
        TESTCASE(28,TestNormalizationTransliterator);
        TESTCASE(29,TestCompoundRBT);
        TESTCASE(30,TestCompoundFilter);
        TESTCASE(31,TestRemove);
        TESTCASE(32,TestToRules);
        TESTCASE(33,TestContext);
        TESTCASE(34,TestSupplemental);
        TESTCASE(35,TestQuantifier);
        TESTCASE(36,TestSTV);
        TESTCASE(37,TestCompoundInverse);
        TESTCASE(38,TestNFDChainRBT);
        TESTCASE(39,TestNullInverse);
        TESTCASE(40,TestAliasInverseID);
        TESTCASE(41,TestCompoundInverseID);
        TESTCASE(42,TestUndefinedVariable);
        TESTCASE(43,TestEmptyContext);
        TESTCASE(44,TestCompoundFilterID);
        TESTCASE(45,TestPropertySet);
        TESTCASE(46,TestNewEngine);
        TESTCASE(47,TestQuantifiedSegment);
        TESTCASE(48,TestDevanagariLatinRT);
        TESTCASE(49,TestTeluguLatinRT);
        TESTCASE(50,TestCompoundLatinRT);
        TESTCASE(51,TestSanskritLatinRT);
        TESTCASE(52,TestLocaleInstantiation);
        TESTCASE(53,TestTitleAccents);
        TESTCASE(54,TestLocaleResource);
        TESTCASE(55,TestParseError);
        TESTCASE(56,TestOutputSet);
        TESTCASE(57,TestVariableRange);
        TESTCASE(58,TestInvalidPostContext);
        TESTCASE(59,TestIDForms);
        TESTCASE(60,TestToRulesMark);
        TESTCASE(61,TestEscape);
        TESTCASE(62,TestAnchorMasking);
        TESTCASE(63,TestDisplayName);
        TESTCASE(64,TestSpecialCases);
#if !UCONFIG_NO_FILE_IO
        TESTCASE(65,TestIncrementalProgress);
#endif
        TESTCASE(66,TestSurrogateCasing);
        TESTCASE(67,TestFunction);
        TESTCASE(68,TestInvalidBackRef);
        TESTCASE(69,TestMulticharStringSet);
        TESTCASE(70,TestUserFunction);
        TESTCASE(71,TestAnyX);
        TESTCASE(72,TestSourceTargetSet);
        TESTCASE(73,TestGurmukhiDevanagari);
        TESTCASE(74,TestPatternWhiteSpace);
        TESTCASE(75,TestAllCodepoints);
        TESTCASE(76,TestBoilerplate);
        TESTCASE(77,TestAlternateSyntax);
        TESTCASE(78,TestBeginEnd);
        TESTCASE(79,TestBeginEndToRules);
        TESTCASE(80,TestRegisterAlias);
        TESTCASE(81,TestRuleStripping);
        TESTCASE(82,TestHalfwidthFullwidth);
        TESTCASE(83,TestThai);
        TESTCASE(84,TestAny);
        default: name = ""; break;
    }
}








void TransliteratorTest::TestInstantiation() {
    UErrorCode ec = U_ZERO_ERROR;
    StringEnumeration* avail = Transliterator::getAvailableIDs(ec);
    assertSuccess("getAvailableIDs()", ec);
    assertTrue("getAvailableIDs()!=NULL", avail!=NULL);
    int32_t n = Transliterator::countAvailableIDs();
    assertTrue("getAvailableIDs().count()==countAvailableIDs()",
               avail->count(ec) == n);
    assertSuccess("count()", ec);
    UnicodeString name;
    for (int32_t i=0; i<n; ++i) {
        const UnicodeString& id = *avail->snext(ec);
        if (!assertSuccess("snext()", ec) ||
            !assertTrue("snext()!=NULL", (&id)!=NULL, TRUE)) {
            break;
        }
        UnicodeString id2 = Transliterator::getAvailableID(i);
        if (id.length() < 1) {
            errln(UnicodeString("FAIL: getAvailableID(") +
                  i + ") returned empty string");
            continue;
        }
        if (id != id2) {
            errln(UnicodeString("FAIL: getAvailableID(") +
                  i + ") != getAvailableIDs().snext()");
            continue;
        }
        UParseError parseError;
        UErrorCode status = U_ZERO_ERROR;
        Transliterator* t = Transliterator::createInstance(id,
                              UTRANS_FORWARD, parseError,status);
        name.truncate(0);
        Transliterator::getDisplayName(id, name);
        if (t == 0) {
#if UCONFIG_NO_BREAK_ITERATION
            
            if (id.compare((UnicodeString)"Thai-Latin") != 0)
#endif
                dataerrln(UnicodeString("FAIL: Couldn't create ") + id +
                      
                      ", line " + parseError.line +
                      ", offset " + parseError.offset +
                      ", pre-context " + prettify(parseError.preContext, TRUE) +
                      ", post-context " +prettify(parseError.postContext,TRUE) +
                      ", Error: " + u_errorName(status));
                
                
                
            int32_t nn = Transliterator::countAvailableIDs();
            if (nn == (n - 1)) {
                n = nn;
                --i; 
            }
        } else {
            logln(UnicodeString("OK: ") + name + " (" + id + ")");

            
            UnicodeString rules;
            t->toRules(rules, TRUE);
            Transliterator *u = Transliterator::createFromRules("x",
                                    rules, UTRANS_FORWARD, parseError,status);
            if (u == 0) {
                errln(UnicodeString("FAIL: ") + id +
                      ".createFromRules() => bad rules" +
                      
                      ", line " + parseError.line +
                      ", offset " + parseError.offset +
                      ", context " + prettify(parseError.preContext, TRUE) +
                      ", rules: " + prettify(rules, TRUE));
            } else {
                delete u;
            }
            delete t;
        }
    }
    assertTrue("snext()==NULL", avail->snext(ec)==NULL);
    assertSuccess("snext()", ec);
    delete avail;

    
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString id("<Not a valid Transliterator ID>");
    Transliterator* t = Transliterator::createInstance(id, UTRANS_FORWARD, parseError, status);
    if (t != 0) {
        errln("FAIL: " + id + " returned a transliterator");
        delete t;
    } else {
        logln("OK: Bogus ID handled properly");
    }
}

void TransliteratorTest::TestSimpleRules(void) {
    








    expect(UnicodeString("ab>x|y;", "") +
           "yc>z",
           "eabcd", "exzd");

    











    expect(UnicodeString("ab>x|yzacw;") +
           "za>q;" +
           "qc>r;" +
           "cw>n",
           "ab", "xyqn");

    

    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    Transliterator *t = Transliterator::createFromRules(
        "<ID>",
        UnicodeString("$dummy=").append((UChar)0xE100) +
        UnicodeString(";"
                      "$vowel=[aeiouAEIOU];"
                      "$lu=[:Lu:];"
                      "$vowel } $lu > '!';"
                      "$vowel > '&';"
                      "'!' { $lu > '^';"
                      "$lu > '*';"
                      "a > ERROR", ""),
        UTRANS_FORWARD, parseError,
        status);
    if (U_FAILURE(status)) {
        dataerrln("FAIL: RBT constructor failed - %s", u_errorName(status));
        return;
    }
    expect(*t, "abcdefgABCDEFGU", "&bcd&fg!^**!^*&");
    delete t;
}




void TransliteratorTest::TestInlineSet(void) {
    expect("{ [:Ll:] } x > y; [:Ll:] > z;", "aAbxq", "zAyzz");
    expect("a[0-9]b > qrs", "1a7b9", "1qrs9");
    
    expect(UnicodeString(
           "$digit = [0-9];"
           "$alpha = [a-zA-Z];"
           "$alphanumeric = [$digit $alpha];" 
           "$special = [^$alphanumeric];"     
           "$alphanumeric > '-';"
           "$special > '*';", ""),
           
           "thx-1138", "---*----");
}









void TransliteratorTest::TestRuleBasedInverse(void) {
    UnicodeString RULES =
        UnicodeString("abc>zyx;") +
        "ab>yz;" +
        "bc>zx;" +
        "ca>xy;" +
        "a>x;" +
        "b>y;" +
        "c>z;" +

        "abc<zyx;" +
        "ab<yz;" +
        "bc<zx;" +
        "ca<xy;" +
        "a<x;" +
        "b<y;" +
        "c<z;" +

        "";

    const char* DATA[] = {
        
        
        
        "a", "x",
        "abcacab", "zyxxxyy",
        "caccb", "xyzzy",
    };

    int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    Transliterator *fwd = Transliterator::createFromRules("<ID>", RULES,
                                UTRANS_FORWARD, parseError, status);
    Transliterator *rev = Transliterator::createFromRules("<ID>", RULES,
                                UTRANS_REVERSE, parseError, status);
    if (U_FAILURE(status)) {
        errln("FAIL: RBT constructor failed");
        return;
    }
    for (int32_t i=0; i<DATA_length; i+=2) {
        expect(*fwd, DATA[i], DATA[i+1]);
        expect(*rev, DATA[i+1], DATA[i]);
    }
    delete fwd;
    delete rev;
}




void TransliteratorTest::TestKeyboard(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createFromRules("<ID>",
                              UnicodeString("psch>Y;")
                              +"ps>y;"
                              +"ch>x;"
                              +"a>A;",
                              UTRANS_FORWARD, parseError,
                              status);
    if (U_FAILURE(status)) {
        errln("FAIL: RBT constructor failed");
        return;
    }
    const char* DATA[] = {
        
        "a", "A",
        "p", "Ap",
        "s", "Aps",
        "c", "Apsc",
        "a", "AycA",
        "psch", "AycAY",
        0, "AycAY", 
    };

    keyboardAux(*t, DATA, (int32_t)(sizeof(DATA)/sizeof(DATA[0])));
    delete t;
}




void TransliteratorTest::TestKeyboard2(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createFromRules("<ID>",
                              UnicodeString("ych>Y;")
                              +"ps>|y;"
                              +"ch>x;"
                              +"a>A;",
                              UTRANS_FORWARD, parseError,
                              status);
    if (U_FAILURE(status)) {
        errln("FAIL: RBT constructor failed");
        return;
    }
    const char* DATA[] = {
        
        "a", "A",
        "p", "Ap",
        "s", "Aps", 
        "c", "Apsc", 
        "a", "AycA",
        "p", "AycAp",
        "s", "AycAps", 
        "c", "AycApsc", 
        "h", "AycAY",
        0, "AycAY", 
    };

    keyboardAux(*t, DATA, (int32_t)(sizeof(DATA)/sizeof(DATA[0])));
    delete t;
}




void TransliteratorTest::TestKeyboard3(void) {
    
    
    
    UnicodeString RULES("t>|y;"
                        "yh>z;");

    const char* DATA[] = {
        
        
        
        "a", "a",
        "b", "ab",
        "t", "abt", 
        "c", "abyc",
        "t", "abyct", 
        "h", "abycz",
        0, "abycz", 
    };

    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createFromRules("<ID>", RULES, UTRANS_FORWARD, parseError, status);
    if (U_FAILURE(status)) {
        errln("FAIL: RBT constructor failed");
        return;
    }
    keyboardAux(*t, DATA, (int32_t)(sizeof(DATA)/sizeof(DATA[0])));
    delete t;
}

void TransliteratorTest::keyboardAux(const Transliterator& t,
                                     const char* DATA[], int32_t DATA_length) {
    UErrorCode status = U_ZERO_ERROR;
    UTransPosition index={0, 0, 0, 0};
    UnicodeString s;
    for (int32_t i=0; i<DATA_length; i+=2) {
        UnicodeString log;
        if (DATA[i] != 0) {
            log = s + " + "
                + DATA[i]
                + " -> ";
            t.transliterate(s, index, DATA[i], status);
        } else {
            log = s + " => ";
            t.finishTransliteration(s, index);
        }
        
        UnicodeString a, b, c;
        s.extractBetween(0, index.contextStart, a);
        s.extractBetween(index.contextStart, index.start, b);
        s.extractBetween(index.start, s.length(), c);
        log.append(a).
            append((UChar)LEFT_BRACE).
            append(b).
            append((UChar)PIPE).
            append(c);
        if (s == DATA[i+1] && U_SUCCESS(status)) {
            logln(log);
        } else {
            errln(UnicodeString("FAIL: ") + log + ", expected " + DATA[i+1]);
        }
    }
}

void TransliteratorTest::TestArabic(void) {






























}





void TransliteratorTest::TestCompoundKana(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* t = Transliterator::createInstance("Latin-Hiragana;Hiragana-Latin", UTRANS_FORWARD, parseError, status);
    if (t == 0) {
        dataerrln("FAIL: construction of Latin-Hiragana;Hiragana-Latin failed - %s", u_errorName(status));
    } else {
        expect(*t, "aaaaa", "aaaaa");
        delete t;
    }
}




void TransliteratorTest::TestCompoundHex(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* a = Transliterator::createInstance("Any-Hex", UTRANS_FORWARD, parseError, status);
    Transliterator* b = Transliterator::createInstance("Hex-Any", UTRANS_FORWARD, parseError, status);
    Transliterator* transab[] = { a, b };
    Transliterator* transba[] = { b, a };
    if (a == 0 || b == 0) {
        errln("FAIL: construction failed");
        delete a;
        delete b;
        return;
    }
    
    expect(*a, "01", UnicodeString("\\u0030\\u0031", ""));
    
    expect(*b, UnicodeString("\\u0030\\u0031", ""), "01");

    Transliterator* ab = new CompoundTransliterator(transab, 2);
    UnicodeString s("abcde", "");
    expect(*ab, s, s);

    UnicodeString str(s);
    a->transliterate(str);
    Transliterator* ba = new CompoundTransliterator(transba, 2);
    expect(*ba, str, str);

    delete ab;
    delete ba;
    delete a;
    delete b;
}

int gTestFilterClassID = 0;



class TestFilter : public UnicodeFilter {
    virtual UnicodeFunctor* clone() const {
        return new TestFilter(*this);
    }
    virtual UBool contains(UChar32 c) const {
        return c != (UChar)0x0063 ;
    }
    
    virtual UnicodeString& toPattern(UnicodeString& result,
                                     UBool ) const {
        return result;
    }
    virtual UBool matchesIndexValue(uint8_t ) const {
        return FALSE;
    }
    virtual void addMatchSetTo(UnicodeSet& ) const {}
public:
    UClassID getDynamicClassID() const { return (UClassID)&gTestFilterClassID; }
};




void TransliteratorTest::TestFiltering(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* hex = Transliterator::createInstance("Any-Hex", UTRANS_FORWARD, parseError, status);
    if (hex == 0) {
        errln("FAIL: createInstance(Any-Hex) failed");
        return;
    }
    hex->adoptFilter(new TestFilter());
    UnicodeString s("abcde");
    hex->transliterate(s);
    UnicodeString exp("\\u0061\\u0062c\\u0064\\u0065", "");
    if (s == exp) {
        logln(UnicodeString("Ok:   \"") + exp + "\"");
    } else {
        logln(UnicodeString("FAIL: \"") + s + "\", wanted \"" + exp + "\"");
    }
    
    
    UnicodeFilter *f = hex->orphanFilter();
    if (f == NULL){
        errln("FAIL: orphanFilter() should get a UnicodeFilter");
    } else {
        delete f;
    }
    delete hex;
}




void TransliteratorTest::TestAnchors(void) {
    expect(UnicodeString("^a  > 0; a$ > 2 ; a > 1;", ""),
           "aaa",
           "012");
    expect(UnicodeString("$s=[z$]; $s{a>0; a}$s>2; a>1;", ""),
           "aaa",
           "012");
    expect(UnicodeString("^ab  > 01 ;"
           " ab  > |8 ;"
           "  b  > k ;"
           " 8x$ > 45 ;"
           " 8x  > 77 ;", ""),

           "ababbabxabx",
           "018k7745");
    expect(UnicodeString("$s = [z$] ;"
           "$s{ab    > 01 ;"
           "   ab    > |8 ;"
           "    b    > k ;"
           "   8x}$s > 45 ;"
           "   8x    > 77 ;", ""),

           "abzababbabxzabxabx",
           "01z018k45z01x45");
}




void TransliteratorTest::TestPatternQuoting(void) {
    
    
    const UnicodeString DATA[] = {
        UnicodeString(UChar(0x4E01)) + ">'[male adult]'",
        UnicodeString(UChar(0x4E01)),
        "[male adult]"
    };

    for (int32_t i=0; i<3; i+=3) {
        logln(UnicodeString("Pattern: ") + prettify(DATA[i]));
        UParseError parseError;
        UErrorCode status = U_ZERO_ERROR;
        Transliterator *t = Transliterator::createFromRules("<ID>", DATA[i], UTRANS_FORWARD, parseError, status);
        if (U_FAILURE(status)) {
            errln("RBT constructor failed");
        } else {
            expect(*t, DATA[i+1], DATA[i+2]);
        }
        delete t;
    }
}




void TransliteratorTest::TestJ277(void) {
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    Transliterator *gl = Transliterator::createInstance("Greek-Latin; NFD; [:M:]Remove; NFC", UTRANS_FORWARD, parseError, status);
    if (gl == NULL) {
        dataerrln("FAIL: createInstance(Greek-Latin) returned NULL - %s", u_errorName(status));
        return;
    }

    UChar sigma = 0x3C3;
    UChar upsilon = 0x3C5;
    UChar nu = 0x3BD;

    UChar alpha = 0x3B1;




    
    UnicodeString syn;
    syn.append(sigma).append(upsilon).append(nu);
    expect(*gl, syn, "syn");

    
    UnicodeString sayn;
    sayn.append(sigma).append(alpha).append(upsilon).append(nu);
    expect(*gl, sayn, "saun");

    
    UnicodeString rules(
                "$alpha   = \\u03B1;"
                "$nu      = \\u03BD;"
                "$sigma   = \\u03C3;"
                "$ypsilon = \\u03C5;"
                "$vowel   = [aeiouAEIOU$alpha$ypsilon];"
                "s <>           $sigma;"
                "a <>           $alpha;"
                "u <>  $vowel { $ypsilon;"
                "y <>           $ypsilon;"
                "n <>           $nu;",
                "");
    Transliterator *mini = Transliterator::createFromRules("mini", rules, UTRANS_REVERSE, parseError, status);
    if (U_FAILURE(status)) { errln("FAIL: Transliterator constructor failed"); return; }
    expect(*mini, syn, "syn");
    expect(*mini, sayn, "saun");
    delete mini;
    mini = NULL;

#if !UCONFIG_NO_FORMATTING
    
    Locale el("el");
    DateFormatSymbols syms(el, status);
    if (U_FAILURE(status)) { errln("FAIL: Transliterator constructor failed"); return; }
    int32_t i, count;
    const UnicodeString* data = syms.getMonths(count);
    for (i=0; i<count; ++i) {
        if (data[i].length() == 0) {
            continue;
        }
        UnicodeString out(data[i]);
        gl->transliterate(out);
        UBool ok = TRUE;
        if (data[i].length() >= 2 && out.length() >= 2 &&
            u_isupper(data[i].charAt(0)) && u_islower(data[i].charAt(1))) {
            if (!(u_isupper(out.charAt(0)) && u_islower(out.charAt(1)))) {
                ok = FALSE;
            }
        }
        if (ok) {
            logln(prettify(data[i] + " -> " + out));
        } else {
            errln(UnicodeString("FAIL: ") + prettify(data[i] + " -> " + out));
        }
    }
#endif

    delete gl;
}




void TransliteratorTest::TestJ243(void) {
    UErrorCode ec = U_ZERO_ERROR;

    
    
    Transliterator *hex =
        Transliterator::createInstance("Hex-Any", UTRANS_FORWARD, ec);
    if (assertSuccess("getInstance", ec)) {
        expect(*hex, UnicodeString("\\u0041+\\U00000042,U+0043uU+0044z", ""), "A+B,CuDz");
    }
    delete hex;











}




void TransliteratorTest::TestJ329(void) {
    
    struct { UBool containsErrors; const char* rule; } DATA[] = {
        { FALSE, "a > b; c > d" },
        { TRUE,  "a > b; no operator; c > d" },
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    for (int32_t i=0; i<DATA_length; ++i) {
        UErrorCode status = U_ZERO_ERROR;
        UParseError parseError;
        Transliterator *rbt = Transliterator::createFromRules("<ID>",
                                    DATA[i].rule,
                                    UTRANS_FORWARD,
                                    parseError,
                                    status);
        UBool gotError = U_FAILURE(status);
        UnicodeString desc(DATA[i].rule);
        desc.append(gotError ? " -> error" : " -> no error");
        if (gotError) {
            desc = desc + ", ParseError code=" + u_errorName(status) +
                " line=" + parseError.line +
                " offset=" + parseError.offset +
                " context=" + parseError.preContext;
        }
        if (gotError == DATA[i].containsErrors) {
            logln(UnicodeString("Ok:   ") + desc);
        } else {
            errln(UnicodeString("FAIL: ") + desc);
        }
        delete rbt;
    }
}




void TransliteratorTest::TestSegments(void) {
    
    
    UnicodeString DATA[] = {
        "([a-z]) '.' ([0-9]) > $2 '-' $1",
        "abc.123.xyz.456",
        "ab1-c23.xy4-z56",

        
        "(([a-z])([0-9])) > $1 '.' $2 '.' $3;",
        "a1 b2",
        "a1.a.1 b2.b.2",
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA)/sizeof(*DATA));

    for (int32_t i=0; i<DATA_length; i+=3) {
        logln("Pattern: " + prettify(DATA[i]));
        UParseError parseError;
        UErrorCode status = U_ZERO_ERROR;
        Transliterator *t = Transliterator::createFromRules("ID", DATA[i], UTRANS_FORWARD, parseError, status);
        if (U_FAILURE(status)) {
            errln("FAIL: RBT constructor");
        } else {
            expect(*t, DATA[i+1], DATA[i+2]);
        }
        delete t;
    }
}




void TransliteratorTest::TestCursorOffset(void) {
    
    
    UnicodeString DATA[] = {
        "pre {alpha} post > | @ ALPHA ;"
        "eALPHA > beta ;"
        "pre {beta} post > BETA @@ | ;"
        "post > xyz",

        "prealphapost prebetapost",

        "prbetaxyz preBETApost",
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA)/sizeof(*DATA));

    for (int32_t i=0; i<DATA_length; i+=3) {
        logln("Pattern: " + prettify(DATA[i]));
        UParseError parseError;
        UErrorCode status = U_ZERO_ERROR;
        Transliterator *t = Transliterator::createFromRules("<ID>", DATA[i], UTRANS_FORWARD, parseError, status);
        if (U_FAILURE(status)) {
            errln("FAIL: RBT constructor");
        } else {
            expect(*t, DATA[i+1], DATA[i+2]);
        }
        delete t;
    }
}





void TransliteratorTest::TestArbitraryVariableValues(void) {
    
    
    UnicodeString DATA[] = {
        "$abe = ab;"
        "$pat = x[yY]z;"
        "$ll  = 'a-z';"
        "$llZ = [$ll];"
        "$llY = [$ll$pat];"
        "$emp = ;"

        "$abe > ABE;"
        "$pat > END;"
        "$llZ > 1;"
        "$llY > 2;"
        "7$emp 8 > 9;"
        "",

        "ab xYzxyz stY78",
        "ABE ENDEND 1129",
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA)/sizeof(*DATA));

    for (int32_t i=0; i<DATA_length; i+=3) {
        logln("Pattern: " + prettify(DATA[i]));
        UParseError parseError;
        UErrorCode status = U_ZERO_ERROR;
        Transliterator *t = Transliterator::createFromRules("<ID>", DATA[i], UTRANS_FORWARD, parseError, status);
        if (U_FAILURE(status)) {
            errln("FAIL: RBT constructor");
        } else {
            expect(*t, DATA[i+1], DATA[i+2]);
        }
        delete t;
    }
}





void TransliteratorTest::TestPositionHandling(void) {
    
    
    const char* DATA[] = {
        "a{t} > SS ; {t}b > UU ; {t} > TT ;",
        "xtat txtb", 
        "xTTaSS TTxUUb",

        "a{t} > SS ; {t}b > UU ; {t} > TT ; a > A ; b > B ;",
        "xtat txtb", 
        "xtaSS TTxUUb",

        "a{t} > SS ; {t}b > UU ; {t} > TT ; a > A ; b > B ;",
        "xtat txtb", 
        "xtaTT TTxTTb",
    };

    
    
    int32_t POS[] = {
        0, 9, 0, 9,
        2, 9, 3, 8,
        3, 8, 3, 8,
    };

    int32_t n = (int32_t)(sizeof(DATA) / sizeof(DATA[0])) / 3;
    for (int32_t i=0; i<n; i++) {
        UErrorCode status = U_ZERO_ERROR;
        UParseError parseError;
        Transliterator *t = Transliterator::createFromRules("<ID>",
                                DATA[3*i], UTRANS_FORWARD, parseError, status);
        if (U_FAILURE(status)) {
            delete t;
            errln("FAIL: RBT constructor");
            return;
        }
        UTransPosition pos;
        pos.contextStart= POS[4*i];
        pos.contextLimit = POS[4*i+1];
        pos.start = POS[4*i+2];
        pos.limit = POS[4*i+3];
        UnicodeString rsource(DATA[3*i+1]);
        t->transliterate(rsource, pos, status);
        if (U_FAILURE(status)) {
            delete t;
            errln("FAIL: transliterate");
            return;
        }
        t->finishTransliteration(rsource, pos);
        expectAux(DATA[3*i],
                  DATA[3*i+1],
                  rsource,
                  DATA[3*i+2]);
        delete t;
    }
}




void TransliteratorTest::TestHiraganaKatakana(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* hk = Transliterator::createInstance("Hiragana-Katakana", UTRANS_FORWARD, parseError, status);
    Transliterator* kh = Transliterator::createInstance("Katakana-Hiragana", UTRANS_FORWARD, parseError, status);
    if (hk == 0 || kh == 0) {
        dataerrln("FAIL: createInstance failed - %s", u_errorName(status));
        delete hk;
        delete kh;
        return;
    }

    
    
    const char* DATA[] = {
        "both",
        "\\u3042\\u3090\\u3099\\u3092\\u3050",
        "\\u30A2\\u30F8\\u30F2\\u30B0",

        "kh",
        "\\u307C\\u3051\\u3060\\u3042\\u3093\\u30FC",
        "\\u30DC\\u30F6\\u30C0\\u30FC\\u30F3\\u30FC",
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    for (int32_t i=0; i<DATA_length; i+=3) {
        UnicodeString h = CharsToUnicodeString(DATA[i+1]);
        UnicodeString k = CharsToUnicodeString(DATA[i+2]);
        switch (*DATA[i]) {
        case 0x68: 
            expect(*hk, h, k);
            break;
        case 0x6B: 
            expect(*kh, k, h);
            break;
        case 0x62: 
            expect(*hk, h, k);
            expect(*kh, k, h);
            break;
        }
    }
    delete hk;
    delete kh;
}




void TransliteratorTest::TestCopyJ476(void) {
    
    
    
    Transliterator *t2 = 0;
    {
        UParseError parseError;
        UErrorCode status = U_ZERO_ERROR;
        Transliterator *t1 = Transliterator::createFromRules("t1",
            "a>A;b>B;'foo'+>'bar'", UTRANS_FORWARD, parseError, status);
        if (U_FAILURE(status)) {
            errln("FAIL: RBT constructor");
            return;
        }
        t2 = t1->clone(); 
        expect(*t1, "abcfoofoo", "ABcbar");
        delete t1;
    }
    expect(*t2, "abcfoofoo", "ABcbar");
    delete t2;
}





void TransliteratorTest::TestInterIndic(void) {
    UnicodeString ID("Devanagari-Gujarati", "");
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    Transliterator* dg = Transliterator::createInstance(ID, UTRANS_FORWARD, parseError, status);
    if (dg == 0) {
        dataerrln("FAIL: createInstance(" + ID + ") returned NULL - " + u_errorName(status));
        return;
    }
    UnicodeString id = dg->getID();
    if (id != ID) {
        errln("FAIL: createInstance(" + ID + ")->getID() => " + id);
    }
    UnicodeString dev = CharsToUnicodeString("\\u0901\\u090B\\u0925");
    UnicodeString guj = CharsToUnicodeString("\\u0A81\\u0A8B\\u0AA5");
    expect(*dg, dev, guj);
    delete dg;
}




void TransliteratorTest::TestFilterIDs(void) {
    
    
    const char* DATA[] = {
        "[aeiou]Any-Hex", 
        "[aeiou]Hex-Any", 
        "quizzical",      
        "q\\u0075\\u0069zz\\u0069c\\u0061l", 
        
        "[aeiou]Any-Hex;[^5]Hex-Any",
        "[^5]Any-Hex;[aeiou]Hex-Any",
        "quizzical",
        "q\\u0075izzical",
        
        "[abc]Null",
        "[abc]Null",
        "xyz",
        "xyz",
    };
    enum { DATA_length = sizeof(DATA) / sizeof(DATA[0]) };

    for (int i=0; i<DATA_length; i+=4) {
        UnicodeString ID(DATA[i], "");
        UnicodeString uID(DATA[i+1], "");
        UnicodeString data2(DATA[i+2], "");
        UnicodeString data3(DATA[i+3], "");
        UParseError parseError;
        UErrorCode status = U_ZERO_ERROR;
        Transliterator *t = Transliterator::createInstance(ID, UTRANS_FORWARD, parseError, status);
        if (t == 0) {
            errln("FAIL: createInstance(" + ID + ") returned NULL");
            return;
        }
        expect(*t, data2, data3);

        
        if (ID != t->getID()) {
            errln("FAIL: createInstance(" + ID + ").getID() => " +
                  t->getID());
        }

        
        Transliterator *u = t->createInverse(status);
        if (u == 0) {
            errln("FAIL: " + ID + ".createInverse() returned NULL");
        } else if (u->getID() != uID) {
            errln("FAIL: " + ID + ".createInverse().getID() => " +
                  u->getID() + ", expected " + uID);
        }

        delete t;
        delete u;
    }
}




void TransliteratorTest::TestCaseMap(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* toUpper =
        Transliterator::createInstance("Any-Upper[^xyzXYZ]", UTRANS_FORWARD, parseError, status);
    Transliterator* toLower =
        Transliterator::createInstance("Any-Lower[^xyzXYZ]", UTRANS_FORWARD, parseError, status);
    Transliterator* toTitle =
        Transliterator::createInstance("Any-Title[^xyzXYZ]", UTRANS_FORWARD, parseError, status);
    if (toUpper==0 || toLower==0 || toTitle==0) {
        errln("FAIL: createInstance returned NULL");
        delete toUpper;
        delete toLower;
        delete toTitle;
        return;
    }

    expect(*toUpper, "The quick brown fox jumped over the lazy dogs.",
           "THE QUICK BROWN FOx JUMPED OVER THE LAzy DOGS.");
    expect(*toLower, "The quIck brown fOX jUMPED OVER THE LAzY dogs.",
           "the quick brown foX jumped over the lazY dogs.");
    expect(*toTitle, "the quick brown foX can't jump over the laZy dogs.",
           "The Quick Brown FoX Can't Jump Over The LaZy Dogs.");

    delete toUpper;
    delete toLower;
    delete toTitle;
}




void TransliteratorTest::TestNameMap(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* uni2name =
        Transliterator::createInstance("Any-Name[^abc]", UTRANS_FORWARD, parseError, status);
    Transliterator* name2uni =
        Transliterator::createInstance("Name-Any", UTRANS_FORWARD, parseError, status);
    if (uni2name==0 || name2uni==0) {
        errln("FAIL: createInstance returned NULL");
        delete uni2name;
        delete name2uni;
        return;
    }

    
    expect(*uni2name, CharsToUnicodeString("\\u00A0abc\\u4E01\\u00B5\\u0A81\\uFFFD\\u0004\\u0009\\u0081\\uFFFF"),
           CharsToUnicodeString("\\\\N{NO-BREAK SPACE}abc\\\\N{CJK UNIFIED IDEOGRAPH-4E01}\\\\N{MICRO SIGN}\\\\N{GUJARATI SIGN CANDRABINDU}\\\\N{REPLACEMENT CHARACTER}\\\\N{<control-0004>}\\\\N{<control-0009>}\\\\N{<control-0081>}\\\\N{<noncharacter-FFFF>}"));
    expect(*name2uni, UNICODE_STRING_SIMPLE("{\\N { NO-BREAK SPACE}abc\\N{  CJK UNIFIED  IDEOGRAPH-4E01  }\\N{x\\N{MICRO SIGN}\\N{GUJARATI SIGN CANDRABINDU}\\N{REPLACEMENT CHARACTER}\\N{<control-0004>}\\N{<control-0009>}\\N{<control-0081>}\\N{<noncharacter-FFFF>}\\N{<control-0004>}\\N{"),
           CharsToUnicodeString("{\\u00A0abc\\u4E01\\\\N{x\\u00B5\\u0A81\\uFFFD\\u0004\\u0009\\u0081\\uFFFF\\u0004\\\\N{"));

    delete uni2name;
    delete name2uni;

    
    Transliterator* t =
        Transliterator::createInstance("Any-Name;Name-Any", UTRANS_FORWARD, parseError, status);
    if (t==0) {
        errln("FAIL: createInstance returned NULL");
        delete t;
        return;
    }

    
    UnicodeString s = CharsToUnicodeString("{\\u00A0abc\\u4E01\\\\N{x\\u00B5\\u0A81\\uFFFD\\u0004\\u0009\\u0081\\uFFFF\\u0004\\\\N{");
    expect(*t, s, s);
    delete t;
}




void TransliteratorTest::TestLiberalizedID(void) {
    
    
    
    
    
    const char* DATA[] = {
        "latin-greek", NULL , "case insensitivity",
        "  Null  ", "Null", "whitespace",
        " Latin[a-z]-Greek  ", "[a-z]Latin-Greek", "inline filter",
        "  null  ; latin-greek  ", NULL , "compound whitespace",
    };
    const int32_t DATA_length = sizeof(DATA)/sizeof(DATA[0]);
    UParseError parseError;
    UErrorCode status= U_ZERO_ERROR;
    for (int32_t i=0; i<DATA_length; i+=3) {
        Transliterator *t = Transliterator::createInstance(DATA[i], UTRANS_FORWARD, parseError, status);
        if (t == 0) {
            dataerrln(UnicodeString("FAIL: ") + DATA[i+2] +
                  " cannot create ID \"" + DATA[i] + "\" - " + u_errorName(status));
        } else {
            UnicodeString exp;
            if (DATA[i+1]) {
                exp = UnicodeString(DATA[i+1], "");
            }
            
            
            if (exp.length() == 0 || exp == t->getID()) {
                logln(UnicodeString("Ok: ") + DATA[i+2] +
                      " create ID \"" + DATA[i] + "\" => \"" +
                      exp + "\"");
            } else {
                errln(UnicodeString("FAIL: ") + DATA[i+2] +
                      " create ID \"" + DATA[i] + "\" => \"" +
                      t->getID() + "\", exp \"" + exp + "\"");
            }
            delete t;
        }
    }
}


void TransliteratorTest::TestCreateInstance(){
    const char* FORWARD = "F";
    const char* REVERSE = "R";
    const char* DATA[] = {
        
        
        
        "Latin-Hangul", REVERSE, "Hangul-Latin", 

        
        "InvalidSource-InvalidTarget", FORWARD, "",
        "InvalidSource-InvalidTarget", REVERSE, "",
        "Hex-Any;InvalidSource-InvalidTarget", FORWARD, "",
        "Hex-Any;InvalidSource-InvalidTarget", REVERSE, "",
        "InvalidSource-InvalidTarget;Hex-Any", FORWARD, "",
        "InvalidSource-InvalidTarget;Hex-Any", REVERSE, "",

        NULL
    };

    for (int32_t i=0; DATA[i]; i+=3) {
        UParseError err;
        UErrorCode ec = U_ZERO_ERROR;
        UnicodeString id(DATA[i]);
        UTransDirection dir = (DATA[i+1]==FORWARD)?
            UTRANS_FORWARD:UTRANS_REVERSE;
        UnicodeString expID(DATA[i+2]);
        Transliterator* t =
            Transliterator::createInstance(id,dir,err,ec);
        UnicodeString newID;
        if (t) {
            newID = t->getID();
        }
        UBool ok = (newID == expID);
        if (!t) {
            newID = u_errorName(ec);
        }
        if (ok) {
            logln((UnicodeString)"Ok: createInstance(" +
                  id + "," + DATA[i+1] + ") => " + newID);
        } else {
            dataerrln((UnicodeString)"FAIL: createInstance(" +
                  id + "," + DATA[i+1] + ") => " + newID +
                  ", expected " + expID);
        }
        delete t;
    }
}




void TransliteratorTest::TestNormalizationTransliterator() {
    
    
    const char* CANON[] = {
        
        "cat",                "cat",                "cat"               ,
        "\\u00e0ardvark",      "a\\u0300ardvark",     "\\u00e0ardvark"    ,

        "\\u1e0a",             "D\\u0307",            "\\u1e0a"            , 
        "D\\u0307",            "D\\u0307",            "\\u1e0a"            , 

        "\\u1e0c\\u0307",       "D\\u0323\\u0307",      "\\u1e0c\\u0307"      , 
        "\\u1e0a\\u0323",       "D\\u0323\\u0307",      "\\u1e0c\\u0307"      , 
        "D\\u0307\\u0323",      "D\\u0323\\u0307",      "\\u1e0c\\u0307"      , 

        "\\u1e10\\u0307\\u0323", "D\\u0327\\u0323\\u0307","\\u1e10\\u0323\\u0307", 
        "D\\u0307\\u0328\\u0323","D\\u0328\\u0323\\u0307","\\u1e0c\\u0328\\u0307", 

        "\\u1E14",             "E\\u0304\\u0300",      "\\u1E14"            , 
        "\\u0112\\u0300",       "E\\u0304\\u0300",      "\\u1E14"            , 
        "\\u00c8\\u0304",       "E\\u0300\\u0304",      "\\u00c8\\u0304"      , 

        "\\u212b",             "A\\u030a",            "\\u00c5"            , 
        "\\u00c5",             "A\\u030a",            "\\u00c5"            , 

        "\\u00fdffin",         "y\\u0301ffin",        "\\u00fdffin"        ,    
        "\\u00fd\\uFB03n",      "y\\u0301\\uFB03n",     "\\u00fd\\uFB03n"     , 

        "Henry IV",           "Henry IV",           "Henry IV"          ,
        "Henry \\u2163",       "Henry \\u2163",       "Henry \\u2163"      ,

        "\\u30AC",             "\\u30AB\\u3099",       "\\u30AC"            , 
        "\\u30AB\\u3099",       "\\u30AB\\u3099",       "\\u30AC"            , 
        "\\uFF76\\uFF9E",       "\\uFF76\\uFF9E",       "\\uFF76\\uFF9E"      , 
        "\\u30AB\\uFF9E",       "\\u30AB\\uFF9E",       "\\u30AB\\uFF9E"      , 
        "\\uFF76\\u3099",       "\\uFF76\\u3099",       "\\uFF76\\u3099"      , 

        "A\\u0300\\u0316",      "A\\u0316\\u0300",      "\\u00C0\\u0316"      ,
        0 
    };

    const char* COMPAT[] = {
        
        "\\uFB4f",             "\\u05D0\\u05DC",       "\\u05D0\\u05DC"     , 

        "\\u00fdffin",         "y\\u0301ffin",        "\\u00fdffin"        ,    
        "\\u00fd\\uFB03n",      "y\\u0301ffin",        "\\u00fdffin"        , 

        "Henry IV",           "Henry IV",           "Henry IV"          ,
        "Henry \\u2163",       "Henry IV",           "Henry IV"          ,

        "\\u30AC",             "\\u30AB\\u3099",       "\\u30AC"            , 
        "\\u30AB\\u3099",       "\\u30AB\\u3099",       "\\u30AC"            , 

        "\\uFF76\\u3099",       "\\u30AB\\u3099",       "\\u30AC"            , 
        0 
    };

    int32_t i;
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* NFD = Transliterator::createInstance("NFD", UTRANS_FORWARD, parseError, status);
    Transliterator* NFC = Transliterator::createInstance("NFC", UTRANS_FORWARD, parseError, status);
    if (!NFD || !NFC) {
        dataerrln("FAIL: createInstance failed: %s", u_errorName(status));
        delete NFD;
        delete NFC;
        return;
    }
    for (i=0; CANON[i]; i+=3) {
        UnicodeString in = CharsToUnicodeString(CANON[i]);
        UnicodeString expd = CharsToUnicodeString(CANON[i+1]);
        UnicodeString expc = CharsToUnicodeString(CANON[i+2]);
        expect(*NFD, in, expd);
        expect(*NFC, in, expc);
    }
    delete NFD;
    delete NFC;

    Transliterator* NFKD = Transliterator::createInstance("NFKD", UTRANS_FORWARD, parseError, status);
    Transliterator* NFKC = Transliterator::createInstance("NFKC", UTRANS_FORWARD, parseError, status);
    if (!NFKD || !NFKC) {
        dataerrln("FAIL: createInstance failed");
        delete NFKD;
        delete NFKC;
        return;
    }
    for (i=0; COMPAT[i]; i+=3) {
        UnicodeString in = CharsToUnicodeString(COMPAT[i]);
        UnicodeString expkd = CharsToUnicodeString(COMPAT[i+1]);
        UnicodeString expkc = CharsToUnicodeString(COMPAT[i+2]);
        expect(*NFKD, in, expkd);
        expect(*NFKC, in, expkc);
    }
    delete NFKD;
    delete NFKC;

    UParseError pe;
    status = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance("NFD; [x]Remove",
                                                       UTRANS_FORWARD,
                                                       pe, status);
    if (t == 0) {
        errln("FAIL: createInstance failed");
    }
    expect(*t, CharsToUnicodeString("\\u010dx"),
           CharsToUnicodeString("c\\u030C"));
    delete t;
}




void TransliteratorTest::TestCompoundRBT(void) {
    
    
    
    UnicodeString rule("::Hex-Any;\n"
                       "::Any-Lower;\n"
                       "a > '.A.';\n"
                       "b > '.B.';\n"
                       "::[^t]Any-Upper;", "");
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createFromRules("Test", rule, UTRANS_FORWARD, parseError, status);
    if (t == 0) {
        errln("FAIL: createFromRules failed");
        return;
    }
    expect(*t, UNICODE_STRING_SIMPLE("\\u0043at in the hat, bat on the mat"),
           "C.A.t IN tHE H.A.t, .B..A.t ON tHE M.A.t");
    UnicodeString r;
    t->toRules(r, TRUE);
    if (r == rule) {
        logln((UnicodeString)"OK: toRules() => " + r);
    } else {
        errln((UnicodeString)"FAIL: toRules() => " + r +
              ", expected " + rule);
    }
    delete t;

    
    t = Transliterator::createInstance("Greek-Latin; Latin-Cyrillic", UTRANS_FORWARD, parseError, status);
    if (t == 0) {
        dataerrln("FAIL: createInstance failed - %s", u_errorName(status));
        return;
    }
    UnicodeString exp("::Greek-Latin;\n::Latin-Cyrillic;");
    t->toRules(r, TRUE);
    if (r != exp) {
        errln((UnicodeString)"FAIL: toRules() => " + r +
              ", expected " + exp);
    } else {
        logln((UnicodeString)"OK: toRules() => " + r);
    }
    delete t;

    
    t = Transliterator::createFromRules("Test", r, UTRANS_FORWARD, parseError, status);
    if (t == 0) {
        errln("FAIL: createFromRules #2 failed");
        return;
    } else {
        logln((UnicodeString)"OK: createFromRules(" + r + ") succeeded");
    }

    
    t->toRules(r, TRUE);
    if (r != exp) {
        errln((UnicodeString)"FAIL: toRules() => " + r +
              ", expected " + exp);
    } else {
        logln((UnicodeString)"OK: toRules() => " + r);
    }

    delete t;

    
    
    UnicodeString id("Upper(Lower);(NFKC)", "");
    t = Transliterator::createInstance(id, UTRANS_FORWARD, parseError, status);
    if (t == 0) {
        errln("FAIL: createInstance #2 failed");
        return;
    }
    if (t->getID() == id) {
        logln((UnicodeString)"OK: created " + id);
    } else {
        errln((UnicodeString)"FAIL: createInstance(" + id +
              ").getID() => " + t->getID());
    }

    Transliterator *u = t->createInverse(status);
    if (u == 0) {
        errln("FAIL: createInverse failed");
        delete t;
        return;
    }
    exp = "NFKC();Lower(Upper)";
    if (u->getID() == exp) {
        logln((UnicodeString)"OK: createInverse(" + id + ") => " +
              u->getID());
    } else {
        errln((UnicodeString)"FAIL: createInverse(" + id + ") => " +
              u->getID());
    }
    delete t;
    delete u;
}

















void TransliteratorTest::TestCompoundFilter(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance
        ("Greek-Latin; Latin-Greek; Lower", UTRANS_FORWARD, parseError, status);
    if (t == 0) {
        dataerrln("FAIL: createInstance failed - %s", u_errorName(status));
        return;
    }
    t->adoptFilter(new UnicodeSet("[^A]", status));
    if (U_FAILURE(status)) {
        errln("FAIL: UnicodeSet ct failed");
        delete t;
        return;
    }
    
    
    expect(*t,
           CharsToUnicodeString("BA\\u039A\\u0391"),
           CharsToUnicodeString("\\u03b2A\\u03ba\\u03b1"));
    delete t;
}

void TransliteratorTest::TestRemove(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance("Remove[abc]", UTRANS_FORWARD, parseError, status);
    if (t == 0) {
        errln("FAIL: createInstance failed");
        return;
    }
    
    expect(*t, "Able bodied baker's cats", "Ale odied ker's ts");
    
    
    
    Transliterator* t2 = t->clone();
    expect(*t2, "Able bodied baker's cats", "Ale odied ker's ts");
    
    delete t;
    delete t2;
}

void TransliteratorTest::TestToRules(void) {
    const char* RBT = "rbt";
    const char* SET = "set";
    static const char* DATA[] = {
        RBT,
        "$a=\\u4E61; [$a] > A;",
        "[\\u4E61] > A;",

        RBT,
        "$white=[[:Zs:][:Zl:]]; $white{a} > A;",
        "[[:Zs:][:Zl:]]{a} > A;",

        SET,
        "[[:Zs:][:Zl:]]",
        "[[:Zs:][:Zl:]]",

        SET,
        "[:Ps:]",
        "[:Ps:]",

        SET,
        "[:L:]",
        "[:L:]",

        SET,
        "[[:L:]-[A]]",
        "[[:L:]-[A]]",

        SET,
        "[~[:Lu:][:Ll:]]",
        "[~[:Lu:][:Ll:]]",

        SET,
        "[~[a-z]]",
        "[~[a-z]]",

        RBT,
        "$white=[:Zs:]; $black=[^$white]; $black{a} > A;",
        "[^[:Zs:]]{a} > A;",

        RBT,
        "$a=[:Zs:]; $b=[[a-z]-$a]; $b{a} > A;",
        "[[a-z]-[:Zs:]]{a} > A;",

        RBT,
        "$a=[:Zs:]; $b=[$a&[a-z]]; $b{a} > A;",
        "[[:Zs:]&[a-z]]{a} > A;",

        RBT,
        "$a=[:Zs:]; $b=[x$a]; $b{a} > A;",
        "[x[:Zs:]]{a} > A;",

        RBT,
        "$accentMinus = [ [\\u0300-\\u0345] & [:M:] - [\\u0338]] ;"
        "$macron = \\u0304 ;"
        "$evowel = [aeiouyAEIOUY] ;"
        "$iotasub = \\u0345 ;" 
        "($evowel $macron $accentMinus *) i > | $1 $iotasub ;",
        "([AEIOUYaeiouy]\\u0304[[\\u0300-\\u0345]&[:M:]-[\\u0338]]*)i > | $1 \\u0345;",

        RBT,
        "([AEIOUYaeiouy]\\u0304[[:M:]-[\\u0304\\u0345]]*)i > | $1 \\u0345;",
        "([AEIOUYaeiouy]\\u0304[[:M:]-[\\u0304\\u0345]]*)i > | $1 \\u0345;",
    };
    static const int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    for (int32_t d=0; d < DATA_length; d+=3) {
        if (DATA[d] == RBT) {
            
            UParseError parseError;
            UErrorCode status = U_ZERO_ERROR;
            Transliterator *t = Transliterator::createFromRules("ID",
                                                                UnicodeString(DATA[d+1], -1, US_INV), UTRANS_FORWARD, parseError, status);
            if (t == 0) {
                dataerrln("FAIL: createFromRules failed - %s", u_errorName(status));
                return;
            }
            UnicodeString rules, escapedRules;
            t->toRules(rules, FALSE);
            t->toRules(escapedRules, TRUE);
            UnicodeString expRules = CharsToUnicodeString(DATA[d+2]);
            UnicodeString expEscapedRules(DATA[d+2], -1, US_INV);
            if (rules == expRules) {
                logln((UnicodeString)"Ok: " + UnicodeString(DATA[d+1], -1, US_INV) +
                      " => " + rules);
            } else {
                errln((UnicodeString)"FAIL: " + UnicodeString(DATA[d+1], -1, US_INV) +
                      " => " + rules + ", exp " + expRules);
            }
            if (escapedRules == expEscapedRules) {
                logln((UnicodeString)"Ok: " + UnicodeString(DATA[d+1], -1, US_INV) +
                      " => " + escapedRules);
            } else {
                errln((UnicodeString)"FAIL: " + UnicodeString(DATA[d+1], -1, US_INV) +
                      " => " + escapedRules + ", exp " + expEscapedRules);
            }
            delete t;
            
        } else {
            
            UErrorCode status = U_ZERO_ERROR;
            UnicodeString pat(DATA[d+1], -1, US_INV);
            UnicodeString expToPat(DATA[d+2], -1, US_INV);
            UnicodeSet set(pat, status);
            if (U_FAILURE(status)) {
                errln("FAIL: UnicodeSet ct failed");
                return;
            }
            
            UnicodeString toPat;
            set.toPattern(toPat);
            if (expToPat == toPat) {
                logln((UnicodeString)"Ok: " + pat +
                      " => " + toPat);
            } else {
                errln((UnicodeString)"FAIL: " + pat +
                      " => " + prettify(toPat, TRUE) +
                      ", exp " + prettify(pat, TRUE));
            }
        }
    }
}

void TransliteratorTest::TestContext() {
    UTransPosition pos = {0, 2, 0, 1}; 
    expect("de > x; {d}e > y;",
           "de",
           "ye",
           &pos);

    expect("ab{c} > z;",
           "xadabdabcy",
           "xadabdabzy");
}

void TransliteratorTest::TestSupplemental() { 

    expect(CharsToUnicodeString("$a=\\U00010300; $s=[\\U00010300-\\U00010323];"
                                "a > $a; $s > i;"),
           CharsToUnicodeString("ab\\U0001030Fx"),
           CharsToUnicodeString("\\U00010300bix"));

    expect(CharsToUnicodeString("$a=[a-z\\U00010300-\\U00010323];"
                                "$b=[A-Z\\U00010400-\\U0001044D];"
                                "($a)($b) > $2 $1;"),
           CharsToUnicodeString("aB\\U00010300\\U00010400c\\U00010401\\U00010301D"),
           CharsToUnicodeString("Ba\\U00010400\\U00010300\\U00010401cD\\U00010301"));

    

    
    
    

    
    
    expect(CharsToUnicodeString("$a=[a\\U00010300-\\U00010323];"
                                "$a {x} > | @ \\U00010400;"
                                "{$a} [^\\u0000-\\uFFFF] > y;"),
           CharsToUnicodeString("kax\\U00010300xm"),
           CharsToUnicodeString("ky\\U00010400y\\U00010400m"));

    expectT("Any-Name",
           CharsToUnicodeString("\\U00010330\\U000E0061\\u00A0"),
           UNICODE_STRING_SIMPLE("\\N{GOTHIC LETTER AHSA}\\N{TAG LATIN SMALL LETTER A}\\N{NO-BREAK SPACE}"));

    expectT("Any-Hex/Unicode",
           CharsToUnicodeString("\\U00010330\\U0010FF00\\U000E0061\\u00A0"),
           UNICODE_STRING_SIMPLE("U+10330U+10FF00U+E0061U+00A0"));

    expectT("Any-Hex/C",
           CharsToUnicodeString("\\U00010330\\U0010FF00\\U000E0061\\u00A0"),
           UNICODE_STRING_SIMPLE("\\U00010330\\U0010FF00\\U000E0061\\u00A0"));

    expectT("Any-Hex/Perl",
           CharsToUnicodeString("\\U00010330\\U0010FF00\\U000E0061\\u00A0"),
           UNICODE_STRING_SIMPLE("\\x{10330}\\x{10FF00}\\x{E0061}\\x{A0}"));

    expectT("Any-Hex/Java",
           CharsToUnicodeString("\\U00010330\\U0010FF00\\U000E0061\\u00A0"),
           UNICODE_STRING_SIMPLE("\\uD800\\uDF30\\uDBFF\\uDF00\\uDB40\\uDC61\\u00A0"));

    expectT("Any-Hex/XML",
           CharsToUnicodeString("\\U00010330\\U0010FF00\\U000E0061\\u00A0"),
           "&#x10330;&#x10FF00;&#xE0061;&#xA0;");

    expectT("Any-Hex/XML10",
           CharsToUnicodeString("\\U00010330\\U0010FF00\\U000E0061\\u00A0"),
           "&#66352;&#1113856;&#917601;&#160;");

    expectT(UNICODE_STRING_SIMPLE("[\\U000E0000-\\U000E0FFF] Remove"),
           CharsToUnicodeString("\\U00010330\\U0010FF00\\U000E0061\\u00A0"),
           CharsToUnicodeString("\\U00010330\\U0010FF00\\u00A0"));
}

void TransliteratorTest::TestQuantifier() { 

    
    expect("a+ {b} > | @@ c; A > a; (a+ c) > '(' $1 ')';",
           "AAAAAb",
           "aaa(aac)");

    
    expect("{b} a+ > c @@ |; (a+) > '(' $1 ')';",
           "baaaaa",
           "caa(aaa)");

    
    expect("{(b)} a+ > $1 @@ |; (a+) > '(' $1 ')';",
           "baaaaa",
           "baa(aaa)");

    
    UTransPosition pos = {0, 5, 3, 5};
    expect("a+ {b} > | @@ c; x > y; (a+ c) > '(' $1 ')';",
           "xxxab",
           "xxx(ac)",
           &pos);

    
    UTransPosition pos2 = {0, 4, 0, 2};
    expect("{b} a+ > c @@ |; x > y; a > A;",
           "baxx",
           "caxx",
           &pos2);

    
    expect("{b} a+ > c @@ |; x > y; a > A;",
           "baxx",
           "cayy");

    expect("(ab)? c > d;",
           "c abc ababc",
           "d d abd");
    
    
    
    expect("(ab)+ {x} > '(' $1 ')';",
           "x abx ababxy",
           "x ab(ab) abab(ab)y");

    expect("b+ > x;",
           "ac abc abbc abbbc",
           "ac axc axc axc");

    expect("[abc]+ > x;",
           "qac abrc abbcs abtbbc",
           "qx xrx xs xtx");

    expect("q{(ab)+} > x;",
           "qa qab qaba qababc qaba",
           "qa qx qxa qxc qxa");

    expect("q(ab)* > x;",
           "qa qab qaba qababc",
           "xa x xa xc");

    
    
    expect("q(ab)* > '(' $1 ')';",
           "qa qab qaba qababc",
           "()a (ab) (ab)a (ab)c");

    
    
    expect("'ab'+ > x;",
           "bb ab ababb",
           "bb x xb");

    
    
    expect("$var = ab; $var+ > x;",
           "bb ab ababb",
           "bb x xb");
}

class TestTrans : public Transliterator {
public:
    TestTrans(const UnicodeString& id) : Transliterator(id, 0) {
    }
    virtual Transliterator* clone(void) const {
        return new TestTrans(getID());
    }
    virtual void handleTransliterate(Replaceable& , UTransPosition& offsets,
        UBool ) const
    {
        offsets.start = offsets.limit;
    }
    virtual UClassID getDynamicClassID() const;
    static UClassID U_EXPORT2 getStaticClassID();
};
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TestTrans)




void TransliteratorTest::TestSTV(void) {
    int32_t ns = Transliterator::countAvailableSources();
    if (ns < 0 || ns > 255) {
        errln((UnicodeString)"FAIL: Bad source count: " + ns);
        return;
    }
    int32_t i, j;
    for (i=0; i<ns; ++i) {
        UnicodeString source;
        Transliterator::getAvailableSource(i, source);
        logln((UnicodeString)"" + i + ": " + source);
        if (source.length() == 0) {
            errln("FAIL: empty source");
            continue;
        }
        int32_t nt = Transliterator::countAvailableTargets(source);
        if (nt < 0 || nt > 255) {
            errln((UnicodeString)"FAIL: Bad target count: " + nt);
            continue;
        }
        for (int32_t j=0; j<nt; ++j) {
            UnicodeString target;
            Transliterator::getAvailableTarget(j, source, target);
            logln((UnicodeString)" " + j + ": " + target);
            if (target.length() == 0) {
                errln("FAIL: empty target");
                continue;
            }
            int32_t nv = Transliterator::countAvailableVariants(source, target);
            if (nv < 0 || nv > 255) {
                errln((UnicodeString)"FAIL: Bad variant count: " + nv);
                continue;
            }
            for (int32_t k=0; k<nv; ++k) {
                UnicodeString variant;
                Transliterator::getAvailableVariant(k, source, target, variant);
                if (variant.length() == 0) { 
                    logln((UnicodeString)"  " + k + ": <empty>");
                } else {
                    logln((UnicodeString)"  " + k + ": " + variant);
                }
            }
        }
    }

    
    const char* IDS[] = { "Fieruwer", "Seoridf-Sweorie", "Oewoir-Oweri/Vsie" };
    const char* FULL_IDS[] = { "Any-Fieruwer", "Seoridf-Sweorie", "Oewoir-Oweri/Vsie" };
    const char* SOURCES[] = { NULL, "Seoridf", "Oewoir" };
    for (i=0; i<3; ++i) {
        Transliterator *t = new TestTrans(IDS[i]);
        if (t == 0) {
            errln("FAIL: out of memory");
            return;
        }
        if (t->getID() != IDS[i]) {
            errln((UnicodeString)"FAIL: ID mismatch for " + IDS[i]);
            delete t;
            return;
        }
        Transliterator::registerInstance(t);
        UErrorCode status = U_ZERO_ERROR;
        t = Transliterator::createInstance(IDS[i], UTRANS_FORWARD, status);
        if (t == NULL) {
            errln((UnicodeString)"FAIL: Registration/creation failed for ID " +
                  IDS[i]);
        } else {
            logln((UnicodeString)"Ok: Registration/creation succeeded for ID " +
                  IDS[i]);
            delete t;
        }
        Transliterator::unregister(IDS[i]);
        t = Transliterator::createInstance(IDS[i], UTRANS_FORWARD, status);
        if (t != NULL) {
            errln((UnicodeString)"FAIL: Unregistration failed for ID " +
                  IDS[i]);
            delete t;
        }
    }

    
    int32_t n = Transliterator::countAvailableIDs();
    for (i=0; i<n; ++i) {
        UnicodeString id = Transliterator::getAvailableID(i);
        for (j=0; j<3; ++j) {
            if (id.caseCompare(FULL_IDS[j],0)==0) {
                errln((UnicodeString)"FAIL: unregister(" + id + ") failed");
            }
        }
    }
    n = Transliterator::countAvailableTargets("Any");
    for (i=0; i<n; ++i) {
        UnicodeString t;
        Transliterator::getAvailableTarget(i, "Any", t);
        if (t.caseCompare(IDS[0],0)==0) {
            errln((UnicodeString)"FAIL: unregister(Any-" + t + ") failed");
        }
    }
    n = Transliterator::countAvailableSources();
    for (i=0; i<n; ++i) {
        UnicodeString s;
        Transliterator::getAvailableSource(i, s);
        for (j=0; j<3; ++j) {
            if (SOURCES[j] == NULL) continue;
            if (s.caseCompare(SOURCES[j],0)==0) {
                errln((UnicodeString)"FAIL: unregister(" + s + "-*) failed");
            }
        }
    }
}




void TransliteratorTest::TestCompoundInverse(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance
        ("Greek-Latin; Title()", UTRANS_REVERSE,parseError, status);
    if (t == 0) {
        dataerrln("FAIL: createInstance - %s", u_errorName(status));
        return;
    }
    UnicodeString exp("(Title);Latin-Greek");
    if (t->getID() == exp) {
        logln("Ok: inverse of \"Greek-Latin; Title()\" is \"" +
              t->getID());
    } else {
        errln("FAIL: inverse of \"Greek-Latin; Title()\" is \"" +
              t->getID() + "\", expected \"" + exp + "\"");
    }
    delete t;
}




void TransliteratorTest::TestNFDChainRBT() {
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator* t = Transliterator::createFromRules(
                               "TEST", "::NFD; aa > Q; a > q;",
                               UTRANS_FORWARD, pe, ec);
    if (t == NULL || U_FAILURE(ec)) {
        dataerrln("FAIL: Transliterator::createFromRules failed with %s", u_errorName(ec));
        return;
    }
    expect(*t, "aa", "Q");
    delete t;

    





















































































}




void TransliteratorTest::TestNullInverse() {
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance("Null", UTRANS_FORWARD, pe, ec);
    if (t == 0 || U_FAILURE(ec)) {
        errln("FAIL: createInstance");
        return;
    }
    Transliterator *u = t->createInverse(ec);
    if (u == 0 || U_FAILURE(ec)) {
        errln("FAIL: createInverse");
        delete t;
        return;
    }
    if (u->getID() != "Null") {
        errln("FAIL: Inverse of Null should be Null");
    }
    delete t;
    delete u;
}




void TransliteratorTest::TestAliasInverseID() {
    UnicodeString ID("Latin-Hangul", ""); 
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance(ID, UTRANS_FORWARD, pe, ec);
    if (t == 0 || U_FAILURE(ec)) {
        dataerrln("FAIL: createInstance - %s", u_errorName(ec));
        return;
    }
    Transliterator *u = t->createInverse(ec);
    if (u == 0 || U_FAILURE(ec)) {
        errln("FAIL: createInverse");
        delete t;
        return;
    }
    UnicodeString exp = "Hangul-Latin";
    UnicodeString got = u->getID();
    if (got != exp) {
        errln((UnicodeString)"FAIL: Inverse of " + ID + " is " + got +
              ", expected " + exp);
    }
    delete t;
    delete u;
}




void TransliteratorTest::TestCompoundInverseID() {
    UnicodeString ID = "Latin-Jamo;NFC(NFD)";
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance(ID, UTRANS_FORWARD, pe, ec);
    if (t == 0 || U_FAILURE(ec)) {
        dataerrln("FAIL: createInstance - %s", u_errorName(ec));
        return;
    }
    Transliterator *u = t->createInverse(ec);
    if (u == 0 || U_FAILURE(ec)) {
        errln("FAIL: createInverse");
        delete t;
        return;
    }
    UnicodeString exp = "NFD(NFC);Jamo-Latin";
    UnicodeString got = u->getID();
    if (got != exp) {
        errln((UnicodeString)"FAIL: Inverse of " + ID + " is " + got +
              ", expected " + exp);
    }
    delete t;
    delete u;
}





void TransliteratorTest::TestUndefinedVariable() {
    UnicodeString rule = "$initial } a <> \\u1161;";
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createFromRules("<ID>", rule, UTRANS_FORWARD, pe, ec);
    delete t;
    if (U_FAILURE(ec)) {
        logln((UnicodeString)"OK: Got exception for " + rule + ", as expected: " +
              u_errorName(ec));
        return;
    }
    errln((UnicodeString)"Fail: bogus rule " + rule + " compiled with error " +
          u_errorName(ec));
}




void TransliteratorTest::TestEmptyContext() {
    expect(" { a } > b;", "xay a ", "xby b ");
}




void TransliteratorTest::TestCompoundFilterID(void) {
    static const char* DATA[] = {
        

        

        
        
        

        "[abc]; [abc]", NULL, NULL, NULL, 
        "Latin-Greek; [abc];", NULL, NULL, NULL, 
        "[b]; Latin-Greek; Upper; ([xyz])", "F", "abc", "a\\u0392c",
        "[b]; (Lower); Latin-Greek; Upper(); ([\\u0392])", "R", "\\u0391\\u0392\\u0393", "\\u0391b\\u0393",
        "#\n::[b]; ::Latin-Greek; ::Upper; ::([xyz]);", "F", "abc", "a\\u0392c",
        "#\n::[b]; ::(Lower); ::Latin-Greek; ::Upper(); ::([\\u0392]);", "R", "\\u0391\\u0392\\u0393", "\\u0391b\\u0393",
        NULL,
    };

    for (int32_t i=0; DATA[i]; i+=4) {
        UnicodeString id = CharsToUnicodeString(DATA[i]);
        UTransDirection direction = (DATA[i+1] != NULL && DATA[i+1][0] == 'R') ?
            UTRANS_REVERSE : UTRANS_FORWARD;
        UnicodeString source;
        UnicodeString exp;
        if (DATA[i+2] != NULL) {
            source = CharsToUnicodeString(DATA[i+2]);
            exp = CharsToUnicodeString(DATA[i+3]);
        }
        UBool expOk = (DATA[i+1] != NULL);
        Transliterator* t = NULL;
        UParseError pe;
        UErrorCode ec = U_ZERO_ERROR;
        if (id.charAt(0) == 0x23) {
            t = Transliterator::createFromRules("ID", id, direction, pe, ec);
        } else {
            t = Transliterator::createInstance(id, direction, pe, ec);
        }
        UBool ok = (t != NULL && U_SUCCESS(ec));
        UnicodeString transID;
        if (t!=0) {
            transID = t->getID();
        }
        else {
            transID = UnicodeString("NULL", "");
        }
        if (ok == expOk) {
            logln((UnicodeString)"Ok: " + id + " => " + transID + ", " +
                  u_errorName(ec));
            if (source.length() != 0) {
                expect(*t, source, exp);
            }
            delete t;
        } else {
            dataerrln((UnicodeString)"FAIL: " + id + " => " + transID + ", " +
                  u_errorName(ec));
        }
    }
}




void TransliteratorTest::TestPropertySet() {
    expect(UNICODE_STRING_SIMPLE("a>A; \\p{Lu}>x; \\p{ANY}>y;"), "abcDEF", "Ayyxxx");
    expect("(.+)>'[' $1 ']';", " a stitch \n in time \r saves 9",
           "[ a stitch ]\n[ in time ]\r[ saves 9]");
}




void TransliteratorTest::TestNewEngine() {
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance("Latin-Hiragana", UTRANS_FORWARD, pe, ec);
    if (t == 0 || U_FAILURE(ec)) {
        dataerrln("FAIL: createInstance Latin-Hiragana - %s", u_errorName(ec));
        return;
    }
    
    expect(*t, CharsToUnicodeString("a\\u3042\\u30A2"),
           CharsToUnicodeString("\\u3042\\u3042\\u30A2"));

    delete t;

#if 1
    
    
    
    Transliterator *a =
        Transliterator::createFromRules("a_to_A", "a > A;", UTRANS_FORWARD, pe, ec);
    Transliterator *A =
        Transliterator::createFromRules("A_to_b", "A > b;", UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        delete a;
        delete A;
        return;
    }

    Transliterator* array[3];
    array[0] = a;
    array[1] = Transliterator::createInstance("NFD", UTRANS_FORWARD, pe, ec);
    array[2] = A;
    if (U_FAILURE(ec)) {
        errln("FAIL: createInstance NFD");
        delete a;
        delete A;
        delete array[1];
        return;
    }

    t = new CompoundTransliterator(array, 3, new UnicodeSet("[:Ll:]", ec));
    if (U_FAILURE(ec)) {
        errln("FAIL: UnicodeSet constructor");
        delete a;
        delete A;
        delete array[1];
        delete t;
        return;
    }

    expect(*t, "aAaA", "bAbA");

    assertTrue("countElements", t->countElements() == 3);
    assertEquals("getElement(0)", t->getElement(0, ec).getID(), "a_to_A");
    assertEquals("getElement(1)", t->getElement(1, ec).getID(), "NFD");
    assertEquals("getElement(2)", t->getElement(2, ec).getID(), "A_to_b");
    assertSuccess("getElement", ec);

    delete a;
    delete A;
    delete array[1];
    delete t;
#endif

    expect("$smooth = x; $macron = q; [:^L:] { ([aeiouyAEIOUY] $macron?) } [^aeiouyAEIOUY$smooth$macron] > | $1 $smooth ;",
           "a",
           "ax");

    UnicodeString gr = CharsToUnicodeString(
        "$ddot = \\u0308 ;"
        "$lcgvowel = [\\u03b1\\u03b5\\u03b7\\u03b9\\u03bf\\u03c5\\u03c9] ;"
        "$rough = \\u0314 ;"
        "($lcgvowel+ $ddot?) $rough > h | $1 ;"
        "\\u03b1 <> a ;"
        "$rough <> h ;");

    expect(gr, CharsToUnicodeString("\\u03B1\\u0314"), "ha");
}





void TransliteratorTest::TestQuantifiedSegment(void) {
    
    expect("([abc]+) > x $1 x;", "cba", "xcbax");

    
    expect("([abc])+ > x $1 x;", "cba", "xax");

    
    expect("([abc])+ { q > x $1 x;", "cbaq", "cbaxax");

    
    expect("{q} ([a-d])+ > '(' $1 ')';", "ddqcba", "dd(a)cba");

    
    
    UnicodeString r("([a-c]){q} > x $1 x;");
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator* t = Transliterator::createFromRules("ID", r, UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: createFromRules");
        delete t;
        return;
    }
    UnicodeString rr;
    t->toRules(rr, TRUE);
    if (r != rr) {
        errln((UnicodeString)"FAIL: \"" + r + "\" x toRules() => \"" + rr + "\"");
    } else {
        logln((UnicodeString)"Ok: \"" + r + "\" x toRules() => \"" + rr + "\"");
    }
    delete t;

    
    
    r = "([a-c])+{q} > x $1 x;";
    t = Transliterator::createFromRules("ID", r, UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: createFromRules");
        delete t;
        return;
    }
    t->toRules(rr, TRUE);
    if (r != rr) {
        errln((UnicodeString)"FAIL: \"" + r + "\" x toRules() => \"" + rr + "\"");
    } else {
        logln((UnicodeString)"Ok: \"" + r + "\" x toRules() => \"" + rr + "\"");
    }
    delete t;
}




void TransliteratorTest::TestDevanagariLatinRT(){
    const int MAX_LEN= 52;
    const char* const source[MAX_LEN] = {
        "bh\\u0101rata",
        "kra",
        "k\\u1E63a",
        "khra",
        "gra",
        "\\u1E45ra",
        "cra",
        "chra",
        "j\\u00F1a",
        "jhra",
        "\\u00F1ra",
        "\\u1E6Dya",
        "\\u1E6Dhra",
        "\\u1E0Dya",
      
        "\\u1E0Dhya",
        "\\u1E5Bhra",
        "\\u1E47ra",
        "tta",
        "thra",
        "dda",
        "dhra",
        "nna",
        "pra",
        "phra",
        "bra",
        "bhra",
        "mra",
        "\\u1E49ra",
      
        "yra",
        "\\u1E8Fra",
      
        "vra",
        "\\u015Bra",
        "\\u1E63ra",
        "sra",
        "hma",
        "\\u1E6D\\u1E6Da",
        "\\u1E6D\\u1E6Dha",
        "\\u1E6Dh\\u1E6Dha",
        "\\u1E0D\\u1E0Da",
        "\\u1E0D\\u1E0Dha",
        "\\u1E6Dya",
        "\\u1E6Dhya",
        "\\u1E0Dya",
        "\\u1E0Dhya",
        
        
        
        
        "hya",
        "\\u015Br\\u0325",
        "\\u015Bca",
        "\\u0115",
        "san\\u0304j\\u012Bb s\\u0113nagupta",
        "\\u0101nand vaddir\\u0101ju",    
        "\\u0101",
        "a"
    };
    const char* const expected[MAX_LEN] = {
        "\\u092D\\u093E\\u0930\\u0924",   
        "\\u0915\\u094D\\u0930",          
        "\\u0915\\u094D\\u0937",          
        "\\u0916\\u094D\\u0930",          
        "\\u0917\\u094D\\u0930",          
        "\\u0919\\u094D\\u0930",          
        "\\u091A\\u094D\\u0930",          
        "\\u091B\\u094D\\u0930",          
        "\\u091C\\u094D\\u091E",          
        "\\u091D\\u094D\\u0930",          
        "\\u091E\\u094D\\u0930",          
        "\\u091F\\u094D\\u092F",          
        "\\u0920\\u094D\\u0930",          
        "\\u0921\\u094D\\u092F",          
      
        "\\u0922\\u094D\\u092F",          
        "\\u0922\\u093C\\u094D\\u0930",   
        "\\u0923\\u094D\\u0930",          
        "\\u0924\\u094D\\u0924",          
        "\\u0925\\u094D\\u0930",          
        "\\u0926\\u094D\\u0926",          
        "\\u0927\\u094D\\u0930",          
        "\\u0928\\u094D\\u0928",          
        "\\u092A\\u094D\\u0930",          
        "\\u092B\\u094D\\u0930",          
        "\\u092C\\u094D\\u0930",          
        "\\u092D\\u094D\\u0930",          
        "\\u092E\\u094D\\u0930",          
        "\\u0929\\u094D\\u0930",          
      
        "\\u092F\\u094D\\u0930",          
        "\\u092F\\u093C\\u094D\\u0930",   
      
        "\\u0935\\u094D\\u0930",          
        "\\u0936\\u094D\\u0930",          
        "\\u0937\\u094D\\u0930",          
        "\\u0938\\u094D\\u0930",          
        "\\u0939\\u094d\\u092E",          
        "\\u091F\\u094D\\u091F",          
        "\\u091F\\u094D\\u0920",          
        "\\u0920\\u094D\\u0920",          
        "\\u0921\\u094D\\u0921",          
        "\\u0921\\u094D\\u0922",          
        "\\u091F\\u094D\\u092F",          
        "\\u0920\\u094D\\u092F",          
        "\\u0921\\u094D\\u092F",          
        "\\u0922\\u094D\\u092F",          
     
        "\\u0939\\u094D\\u092F",          
        "\\u0936\\u0943",                 
        "\\u0936\\u094D\\u091A",          
        "\\u090d",                        
        "\\u0938\\u0902\\u091C\\u0940\\u092C\\u094D \\u0938\\u0947\\u0928\\u0917\\u0941\\u092A\\u094D\\u0924",
        "\\u0906\\u0928\\u0902\\u0926\\u094D \\u0935\\u0926\\u094D\\u0926\\u093F\\u0930\\u093E\\u091C\\u0941",    
        "\\u0906",
        "\\u0905",
    };
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    UnicodeString message;
    Transliterator* latinToDev=Transliterator::createInstance("Latin-Devanagari", UTRANS_FORWARD, parseError, status);
    Transliterator* devToLatin=Transliterator::createInstance("Devanagari-Latin", UTRANS_FORWARD, parseError, status);
    if(U_FAILURE(status)){
        dataerrln("FAIL: construction " +   UnicodeString(" Error: ") + u_errorName(status));
        dataerrln("PreContext: " + prettify(parseError.preContext) + " PostContext: " + prettify( parseError.postContext) );
        return;
    }
    UnicodeString gotResult;
    for(int i= 0; i<MAX_LEN; i++){
        gotResult = source[i];
        expect(*latinToDev,CharsToUnicodeString(source[i]),CharsToUnicodeString(expected[i]));
        expect(*devToLatin,CharsToUnicodeString(expected[i]),CharsToUnicodeString(source[i]));
    }
    delete latinToDev;
    delete devToLatin;
}

void TransliteratorTest::TestTeluguLatinRT(){
    const int MAX_LEN=10;
    const char* const source[MAX_LEN] = {   
        "raghur\\u0101m vi\\u015Bvan\\u0101dha",                         
        "\\u0101nand vaddir\\u0101ju",                                   
        "r\\u0101j\\u012Bv ka\\u015Barab\\u0101da",                      
        "san\\u0304j\\u012Bv ka\\u015Barab\\u0101da",                    
        "san\\u0304j\\u012Bb sen'gupta",                                 
        "amar\\u0113ndra hanum\\u0101nula",                              
        "ravi kum\\u0101r vi\\u015Bvan\\u0101dha",                       
        "\\u0101ditya kandr\\u0113gula",                                 
        "\\u015Br\\u012Bdhar ka\\u1E47\\u1E6Dama\\u015Be\\u1E6D\\u1E6Di",
        "m\\u0101dhav de\\u015Be\\u1E6D\\u1E6Di"                         
    };

    const char* const expected[MAX_LEN] = {
        "\\u0c30\\u0c18\\u0c41\\u0c30\\u0c3e\\u0c2e\\u0c4d \\u0c35\\u0c3f\\u0c36\\u0c4d\\u0c35\\u0c28\\u0c3e\\u0c27",     
        "\\u0c06\\u0c28\\u0c02\\u0c26\\u0c4d \\u0C35\\u0C26\\u0C4D\\u0C26\\u0C3F\\u0C30\\u0C3E\\u0C1C\\u0C41",     
        "\\u0c30\\u0c3e\\u0c1c\\u0c40\\u0c35\\u0c4d \\u0c15\\u0c36\\u0c30\\u0c2c\\u0c3e\\u0c26",
        "\\u0c38\\u0c02\\u0c1c\\u0c40\\u0c35\\u0c4d \\u0c15\\u0c36\\u0c30\\u0c2c\\u0c3e\\u0c26",
        "\\u0c38\\u0c02\\u0c1c\\u0c40\\u0c2c\\u0c4d \\u0c38\\u0c46\\u0c28\\u0c4d\\u0c17\\u0c41\\u0c2a\\u0c4d\\u0c24",
        "\\u0c05\\u0c2e\\u0c30\\u0c47\\u0c02\\u0c26\\u0c4d\\u0c30 \\u0c39\\u0c28\\u0c41\\u0c2e\\u0c3e\\u0c28\\u0c41\\u0c32",
        "\\u0c30\\u0c35\\u0c3f \\u0c15\\u0c41\\u0c2e\\u0c3e\\u0c30\\u0c4d \\u0c35\\u0c3f\\u0c36\\u0c4d\\u0c35\\u0c28\\u0c3e\\u0c27",
        "\\u0c06\\u0c26\\u0c3f\\u0c24\\u0c4d\\u0c2f \\u0C15\\u0C02\\u0C26\\u0C4D\\u0C30\\u0C47\\u0C17\\u0C41\\u0c32",
        "\\u0c36\\u0c4d\\u0c30\\u0c40\\u0C27\\u0C30\\u0C4D \\u0c15\\u0c02\\u0c1f\\u0c2e\\u0c36\\u0c46\\u0c1f\\u0c4d\\u0c1f\\u0c3f",
        "\\u0c2e\\u0c3e\\u0c27\\u0c35\\u0c4d \\u0c26\\u0c46\\u0c36\\u0c46\\u0c1f\\u0c4d\\u0c1f\\u0c3f",
    };

    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    UnicodeString message;
    Transliterator* latinToDev=Transliterator::createInstance("Latin-Telugu", UTRANS_FORWARD, parseError, status);
    Transliterator* devToLatin=Transliterator::createInstance("Telugu-Latin", UTRANS_FORWARD, parseError, status);
    if(U_FAILURE(status)){
        dataerrln("FAIL: construction " +   UnicodeString(" Error: ") + u_errorName(status));
        dataerrln("PreContext: " + prettify(parseError.preContext) + " PostContext: " + prettify( parseError.postContext) );
        return;
    }
    UnicodeString gotResult;
    for(int i= 0; i<MAX_LEN; i++){
        gotResult = source[i];
        expect(*latinToDev,CharsToUnicodeString(source[i]),CharsToUnicodeString(expected[i]));
        expect(*devToLatin,CharsToUnicodeString(expected[i]),CharsToUnicodeString(source[i]));
    }
    delete latinToDev;
    delete devToLatin;
}

void TransliteratorTest::TestSanskritLatinRT(){
    const int MAX_LEN =16;
    const char* const source[MAX_LEN] = {
        "rmk\\u1E63\\u0113t",
        "\\u015Br\\u012Bmad",
        "bhagavadg\\u012Bt\\u0101",
        "adhy\\u0101ya",
        "arjuna",
        "vi\\u1E63\\u0101da",
        "y\\u014Dga",
        "dhr\\u0325tar\\u0101\\u1E63\\u1E6Dra",
        "uv\\u0101cr\\u0325",
        "dharmak\\u1E63\\u0113tr\\u0113",
        "kuruk\\u1E63\\u0113tr\\u0113",
        "samav\\u0113t\\u0101",
        "yuyutsava\\u1E25",
        "m\\u0101mak\\u0101\\u1E25",
    
        "kimakurvata",
        "san\\u0304java",
    };
    const char* const expected[MAX_LEN] = {
        "\\u0930\\u094D\\u092E\\u094D\\u0915\\u094D\\u0937\\u0947\\u0924\\u094D",
        "\\u0936\\u094d\\u0930\\u0940\\u092e\\u0926\\u094d",
        "\\u092d\\u0917\\u0935\\u0926\\u094d\\u0917\\u0940\\u0924\\u093e",
        "\\u0905\\u0927\\u094d\\u092f\\u093e\\u092f",
        "\\u0905\\u0930\\u094d\\u091c\\u0941\\u0928",
        "\\u0935\\u093f\\u0937\\u093e\\u0926",
        "\\u092f\\u094b\\u0917",
        "\\u0927\\u0943\\u0924\\u0930\\u093e\\u0937\\u094d\\u091f\\u094d\\u0930",
        "\\u0909\\u0935\\u093E\\u091A\\u0943",
        "\\u0927\\u0930\\u094d\\u092e\\u0915\\u094d\\u0937\\u0947\\u0924\\u094d\\u0930\\u0947",
        "\\u0915\\u0941\\u0930\\u0941\\u0915\\u094d\\u0937\\u0947\\u0924\\u094d\\u0930\\u0947",
        "\\u0938\\u092e\\u0935\\u0947\\u0924\\u093e",
        "\\u092f\\u0941\\u092f\\u0941\\u0924\\u094d\\u0938\\u0935\\u0903",
        "\\u092e\\u093e\\u092e\\u0915\\u093e\\u0903",
    
        "\\u0915\\u093f\\u092e\\u0915\\u0941\\u0930\\u094d\\u0935\\u0924",
        "\\u0938\\u0902\\u091c\\u0935",
    };
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    UnicodeString message;
    Transliterator* latinToDev=Transliterator::createInstance("Latin-Devanagari", UTRANS_FORWARD, parseError, status);
    Transliterator* devToLatin=Transliterator::createInstance("Devanagari-Latin", UTRANS_FORWARD, parseError, status);
    if(U_FAILURE(status)){
        dataerrln("FAIL: construction " +   UnicodeString(" Error: ") + u_errorName(status));
        dataerrln("PreContext: " + prettify(parseError.preContext) + " PostContext: " + prettify( parseError.postContext) );
        return;
    }
    UnicodeString gotResult;
    for(int i= 0; i<MAX_LEN; i++){
        gotResult = source[i];
        expect(*latinToDev,CharsToUnicodeString(source[i]),CharsToUnicodeString(expected[i]));
        expect(*devToLatin,CharsToUnicodeString(expected[i]),CharsToUnicodeString(source[i]));
    }
    delete latinToDev;
    delete devToLatin;
}


void TransliteratorTest::TestCompoundLatinRT(){
    const char* const source[] = {
        "rmk\\u1E63\\u0113t",
        "\\u015Br\\u012Bmad",
        "bhagavadg\\u012Bt\\u0101",
        "adhy\\u0101ya",
        "arjuna",
        "vi\\u1E63\\u0101da",
        "y\\u014Dga",
        "dhr\\u0325tar\\u0101\\u1E63\\u1E6Dra",
        "uv\\u0101cr\\u0325",
        "dharmak\\u1E63\\u0113tr\\u0113",
        "kuruk\\u1E63\\u0113tr\\u0113",
        "samav\\u0113t\\u0101",
        "yuyutsava\\u1E25",
        "m\\u0101mak\\u0101\\u1E25",
     
        "kimakurvata",
        "san\\u0304java"
    };
    const int MAX_LEN = sizeof(source)/sizeof(source[0]);
    const char* const expected[MAX_LEN] = {
        "\\u0930\\u094D\\u092E\\u094D\\u0915\\u094D\\u0937\\u0947\\u0924\\u094D",
        "\\u0936\\u094d\\u0930\\u0940\\u092e\\u0926\\u094d",
        "\\u092d\\u0917\\u0935\\u0926\\u094d\\u0917\\u0940\\u0924\\u093e",
        "\\u0905\\u0927\\u094d\\u092f\\u093e\\u092f",
        "\\u0905\\u0930\\u094d\\u091c\\u0941\\u0928",
        "\\u0935\\u093f\\u0937\\u093e\\u0926",
        "\\u092f\\u094b\\u0917",
        "\\u0927\\u0943\\u0924\\u0930\\u093e\\u0937\\u094d\\u091f\\u094d\\u0930",
        "\\u0909\\u0935\\u093E\\u091A\\u0943",
        "\\u0927\\u0930\\u094d\\u092e\\u0915\\u094d\\u0937\\u0947\\u0924\\u094d\\u0930\\u0947",
        "\\u0915\\u0941\\u0930\\u0941\\u0915\\u094d\\u0937\\u0947\\u0924\\u094d\\u0930\\u0947",
        "\\u0938\\u092e\\u0935\\u0947\\u0924\\u093e",
        "\\u092f\\u0941\\u092f\\u0941\\u0924\\u094d\\u0938\\u0935\\u0903",
        "\\u092e\\u093e\\u092e\\u0915\\u093e\\u0903",
    
        "\\u0915\\u093f\\u092e\\u0915\\u0941\\u0930\\u094d\\u0935\\u0924",
        "\\u0938\\u0902\\u091c\\u0935"
    };
    if(MAX_LEN != sizeof(expected)/sizeof(expected[0])) {
        errln("error in TestCompoundLatinRT: source[] and expected[] have different lengths!");
        return;
    }

    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    UnicodeString message;
    Transliterator* devToLatinToDev  =Transliterator::createInstance("Devanagari-Latin;Latin-Devanagari", UTRANS_FORWARD, parseError, status);
    Transliterator* latinToDevToLatin=Transliterator::createInstance("Latin-Devanagari;Devanagari-Latin", UTRANS_FORWARD, parseError, status);
    Transliterator* devToTelToDev    =Transliterator::createInstance("Devanagari-Telugu;Telugu-Devanagari", UTRANS_FORWARD, parseError, status);
    Transliterator* latinToTelToLatin=Transliterator::createInstance("Latin-Telugu;Telugu-Latin", UTRANS_FORWARD, parseError, status);

    if(U_FAILURE(status)){
        dataerrln("FAIL: construction " +   UnicodeString(" Error: ") + u_errorName(status));
        dataerrln("PreContext: " + prettify(parseError.preContext) + " PostContext: " + prettify( parseError.postContext) );
        return;
    }
    UnicodeString gotResult;
    for(int i= 0; i<MAX_LEN; i++){
        gotResult = source[i];
        expect(*devToLatinToDev,CharsToUnicodeString(expected[i]),CharsToUnicodeString(expected[i]));
        expect(*latinToDevToLatin,CharsToUnicodeString(source[i]),CharsToUnicodeString(source[i]));
        expect(*latinToTelToLatin,CharsToUnicodeString(source[i]),CharsToUnicodeString(source[i]));

    }
    delete(latinToDevToLatin);
    delete(devToLatinToDev);  
    delete(devToTelToDev);    
    delete(latinToTelToLatin);
}




void TransliteratorTest::TestGurmukhiDevanagari(){
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    UnicodeSet vowel(UnicodeString("[\\u0905-\\u090A \\u090F\\u0910\\u0913\\u0914 \\u093e-\\u0942\\u0947\\u0948\\u094B\\u094C\\u094D]", -1, US_INV).unescape(), status);
    UnicodeSet non_vowel(UnicodeString("[\\u0915-\\u0928\\u092A-\\u0930]", -1, US_INV).unescape(), status);
    UParseError parseError;

    UnicodeSetIterator vIter(vowel);
    UnicodeSetIterator nvIter(non_vowel);
    Transliterator* trans = Transliterator::createInstance("Devanagari-Gurmukhi",UTRANS_FORWARD, parseError, status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating transliterator %s", u_errorName(status));
      delete trans;
      return;
    }
    UnicodeString src (" \\u0902", -1, US_INV);
    UnicodeString expected(" \\u0A02", -1, US_INV);
    src = src.unescape();
    expected= expected.unescape();

    while(vIter.next()){
        src.setCharAt(0,(UChar) vIter.getCodepoint());
        expected.setCharAt(0,(UChar) (vIter.getCodepoint()+0x0100));
        expect(*trans,src,expected);
    }
    
    expected.setCharAt(1,0x0A70);
    while(nvIter.next()){
        
        src.setCharAt(0,(UChar)nvIter.getCodepoint());
        expected.setCharAt(0,(UChar) (nvIter.getCodepoint()+0x0100));
        expect(*trans,src,expected);
    }
    delete trans;
}



void TransliteratorTest::TestLocaleInstantiation(void) {
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance("ru_RU-Latin", UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        dataerrln("FAIL: createInstance(ru_RU-Latin) - %s", u_errorName(ec));
        delete t;
        return;
    }
    expect(*t, CharsToUnicodeString("\\u0430"), "a");
    delete t;
    
    t = Transliterator::createInstance("en-el", UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: createInstance(en-el)");
        delete t;
        return;
    }
    expect(*t, "a", CharsToUnicodeString("\\u03B1"));
    delete t;
}
        



void TransliteratorTest::TestTitleAccents(void) {
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createInstance("Title", UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: createInstance(Title)");
        delete t;
        return;
    }
    expect(*t, CharsToUnicodeString("a\\u0300b can't abe"), CharsToUnicodeString("A\\u0300b Can't Abe"));
    delete t;
}




void TransliteratorTest::TestLocaleResource() {
    const char* DATA[] = {
        
        
        "Latin-el",              "b",               "\\u03bc\\u03c0",
        "Latin-Greek",           "b",               "\\u03B2",
        "Greek-Latin/UNGEGN",    "\\u03B2",         "v",
        "el-Latin",              "\\u03B2",         "v",
        "Greek-Latin",           "\\u03B2",         "b",
    };
    const int32_t DATA_length = sizeof(DATA) / sizeof(DATA[0]);
    for (int32_t i=0; i<DATA_length; i+=3) {
        UParseError pe;
        UErrorCode ec = U_ZERO_ERROR;
        Transliterator *t = Transliterator::createInstance(DATA[i], UTRANS_FORWARD, pe, ec);
        if (U_FAILURE(ec)) {
            dataerrln((UnicodeString)"FAIL: createInstance(" + DATA[i] + ") - " + u_errorName(ec));
            delete t;
            continue;
        }
        expect(*t, CharsToUnicodeString(DATA[i+1]),
               CharsToUnicodeString(DATA[i+2]));
        delete t;
    }
}




void TransliteratorTest::TestParseError() {
    static const char* rule =
        "a > b;\n"
        "# more stuff\n"
        "d << b;";
    UErrorCode ec = U_ZERO_ERROR;
    UParseError pe;
    Transliterator *t = Transliterator::createFromRules("ID", rule, UTRANS_FORWARD, pe, ec);
    delete t;
    if (U_FAILURE(ec)) {
        UnicodeString err(pe.preContext);
        err.append((UChar)124).append(pe.postContext);
        if (err.indexOf("d << b") >= 0) {
            logln("Ok: " + err);
        } else {
            errln("FAIL: " + err);
        }
    }
    else {
        errln("FAIL: no syntax error");
    }
    static const char* maskingRule =
        "a>x;\n"
        "# more stuff\n"
        "ab>y;";
    ec = U_ZERO_ERROR;
    delete Transliterator::createFromRules("ID", maskingRule, UTRANS_FORWARD, pe, ec);
    if (ec != U_RULE_MASK_ERROR) {
        errln("FAIL: returned %s instead of U_RULE_MASK_ERROR", u_errorName(ec));
    }
    else if (UnicodeString("a > x;") != UnicodeString(pe.preContext)) {
        errln("FAIL: did not get expected precontext");
    }
    else if (UnicodeString("ab > y;") != UnicodeString(pe.postContext)) {
        errln("FAIL: did not get expected postcontext");
    }
}




void TransliteratorTest::TestOutputSet() {
    UnicodeString rule = "$set = [a-cm-n]; b > $set;";
    UErrorCode ec = U_ZERO_ERROR;
    UParseError pe;
    Transliterator *t = Transliterator::createFromRules("ID", rule, UTRANS_FORWARD, pe, ec);
    delete t;
    if (U_FAILURE(ec)) {
        UnicodeString err(pe.preContext);
        err.append((UChar)124).append(pe.postContext);
        logln("Ok: " + err);
        return;
    }
    errln("FAIL: No syntax error");
}        





void TransliteratorTest::TestVariableRange() {
    UnicodeString rule = "use variable range 0x70 0x72; a > A; b > B; q > Q;";
    UErrorCode ec = U_ZERO_ERROR;
    UParseError pe;
    Transliterator *t = Transliterator::createFromRules("ID", rule, UTRANS_FORWARD, pe, ec);
    delete t;
    if (U_FAILURE(ec)) {
        UnicodeString err(pe.preContext);
        err.append((UChar)124).append(pe.postContext);
        logln("Ok: " + err);
        return;
    }
    errln("FAIL: No syntax error");
}




void TransliteratorTest::TestInvalidPostContext() {
    UnicodeString rule = "a}b{c>d;";
    UErrorCode ec = U_ZERO_ERROR;
    UParseError pe;
    Transliterator *t = Transliterator::createFromRules("ID", rule, UTRANS_FORWARD, pe, ec);
    delete t;
    if (U_FAILURE(ec)) {
        UnicodeString err(pe.preContext);
        err.append((UChar)124).append(pe.postContext);
        if (err.indexOf("a}b{c") >= 0) {
            logln("Ok: " + err);
        } else {
            errln("FAIL: " + err);
        }
        return;
    }
    errln("FAIL: No syntax error");
}




void TransliteratorTest::TestIDForms() {
    const char* DATA[] = {
        "NFC", NULL, "NFD",
        "nfd", NULL, "NFC", 
        "Any-NFKD", NULL, "Any-NFKC",
        "Null", NULL, "Null",
        "-nfkc", "nfkc", "NFKD",
        "-nfkc/", "nfkc", "NFKD",
        "Latin-Greek/UNGEGN", NULL, "Greek-Latin/UNGEGN",
        "Greek/UNGEGN-Latin", "Greek-Latin/UNGEGN", "Latin-Greek/UNGEGN",
        "Bengali-Devanagari/", "Bengali-Devanagari", "Devanagari-Bengali",
        "Source-", NULL, NULL,
        "Source/Variant-", NULL, NULL,
        "Source-/Variant", NULL, NULL,
        "/Variant", NULL, NULL,
        "/Variant-", NULL, NULL,
        "-/Variant", NULL, NULL,
        "-/", NULL, NULL,
        "-", NULL, NULL,
        "/", NULL, NULL,
    };
    const int32_t DATA_length = sizeof(DATA)/sizeof(DATA[0]);
    
    for (int32_t i=0; i<DATA_length; i+=3) {
        const char* ID = DATA[i];
        const char* expID = DATA[i+1];
        const char* expInvID = DATA[i+2];
        UBool expValid = (expInvID != NULL);
        if (expID == NULL) {
            expID = ID;
        }
        UParseError pe;
        UErrorCode ec = U_ZERO_ERROR;
        Transliterator *t =
            Transliterator::createInstance(ID, UTRANS_FORWARD, pe, ec);
        if (U_FAILURE(ec)) {
            if (!expValid) {
                logln((UnicodeString)"Ok: getInstance(" + ID +") => " + u_errorName(ec));
            } else {
                dataerrln((UnicodeString)"FAIL: Couldn't create " + ID + " - " + u_errorName(ec));
            }
            delete t;
            continue;
        }
        Transliterator *u = t->createInverse(ec);
        if (U_FAILURE(ec)) {
            errln((UnicodeString)"FAIL: Couldn't create inverse of " + ID);
            delete t;
            delete u;
            continue;
        }
        if (t->getID() == expID &&
            u->getID() == expInvID) {
            logln((UnicodeString)"Ok: " + ID + ".getInverse() => " + expInvID);
        } else {
            errln((UnicodeString)"FAIL: getInstance(" + ID + ") => " +
                  t->getID() + " x getInverse() => " + u->getID() +
                  ", expected " + expInvID);
        }
        delete t;
        delete u;
    }
}

static const UChar SPACE[]   = {32,0};
static const UChar NEWLINE[] = {10,0};
static const UChar RETURN[]  = {13,0};
static const UChar EMPTY[]   = {0};

void TransliteratorTest::checkRules(const UnicodeString& label, Transliterator& t2,
                                    const UnicodeString& testRulesForward) {
    UnicodeString rules2; t2.toRules(rules2, TRUE);
    
    rules2.findAndReplace(SPACE, EMPTY);
    rules2.findAndReplace(NEWLINE, EMPTY);
    rules2.findAndReplace(RETURN, EMPTY);

    UnicodeString testRules(testRulesForward); testRules.findAndReplace(SPACE, EMPTY);
    
    if (rules2 != testRules) {
        errln(label);
        logln((UnicodeString)"GENERATED RULES: " + rules2);
        logln((UnicodeString)"SHOULD BE:       " + testRulesForward);
    }
}




void TransliteratorTest::TestToRulesMark() {
    const char* testRules = 
        "::[[:Latin:][:Mark:]];"
        "::NFKD (NFC);"
        "::Lower (Lower);"
        "a <> \\u03B1;" 
        "::NFKC (NFD);"
        "::Upper (Lower);"
        "::Lower ();"
        "::([[:Greek:][:Mark:]]);"
        ;
    const char* testRulesForward = 
        "::[[:Latin:][:Mark:]];"
        "::NFKD(NFC);"
        "::Lower(Lower);"
        "a > \\u03B1;"
        "::NFKC(NFD);"
        "::Upper (Lower);"
        "::Lower ();"
        ;
    const char* testRulesBackward = 
        "::[[:Greek:][:Mark:]];"
        "::Lower (Upper);"
        "::NFD(NFKC);"
        "\\u03B1 > a;"
        "::Lower(Lower);"
        "::NFC(NFKD);"
        ;
    UnicodeString source = CharsToUnicodeString("\\u00E1"); 
    UnicodeString target = CharsToUnicodeString("\\u03AC"); 
    
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t2 = Transliterator::createFromRules("source-target", UnicodeString(testRules, -1, US_INV), UTRANS_FORWARD, pe, ec);
    Transliterator *t3 = Transliterator::createFromRules("target-source", UnicodeString(testRules, -1, US_INV), UTRANS_REVERSE, pe, ec);

    if (U_FAILURE(ec)) {
        delete t2;
        delete t3;
        dataerrln((UnicodeString)"FAIL: createFromRules => " + u_errorName(ec));
        return;
    }
    
    expect(*t2, source, target);
    expect(*t3, target, source);
    
    checkRules("Failed toRules FORWARD", *t2, UnicodeString(testRulesForward, -1, US_INV));
    checkRules("Failed toRules BACKWARD", *t3, UnicodeString(testRulesBackward, -1, US_INV));

    delete t2;
    delete t3;
}




void TransliteratorTest::TestEscape() {
    UParseError pe;
    UErrorCode ec;
    Transliterator *t;

    ec = U_ZERO_ERROR;
    t = Transliterator::createInstance("Hex-Any", UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: createInstance");
    } else {
        expect(*t,
               UNICODE_STRING_SIMPLE("\\x{40}\\U00000031&#x32;&#81;"),
               "@12Q");
    }
    delete t;

    ec = U_ZERO_ERROR;
    t = Transliterator::createInstance("Any-Hex/C", UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: createInstance");
    } else {
        expect(*t,
               CharsToUnicodeString("A\\U0010BEEF\\uFEED"),
               UNICODE_STRING_SIMPLE("\\u0041\\U0010BEEF\\uFEED"));
    }
    delete t;

    ec = U_ZERO_ERROR;
    t = Transliterator::createInstance("Any-Hex/Java", UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: createInstance");
    } else {
        expect(*t,
               CharsToUnicodeString("A\\U0010BEEF\\uFEED"),
               UNICODE_STRING_SIMPLE("\\u0041\\uDBEF\\uDEEF\\uFEED"));
    }
    delete t;

    ec = U_ZERO_ERROR;
    t = Transliterator::createInstance("Any-Hex/Perl", UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: createInstance");
    } else {
        expect(*t,
               CharsToUnicodeString("A\\U0010BEEF\\uFEED"),
               UNICODE_STRING_SIMPLE("\\x{41}\\x{10BEEF}\\x{FEED}"));
    }
    delete t;
}


void TransliteratorTest::TestAnchorMasking(){
    UnicodeString rule ("^a > Q; a > q;");
    UErrorCode status= U_ZERO_ERROR;
    UParseError parseError;

    Transliterator* t = Transliterator::createFromRules("ID", rule, UTRANS_FORWARD,parseError,status);
    if(U_FAILURE(status)){
        errln(UnicodeString("FAIL: ") + "ID" +
              ".createFromRules() => bad rules" +
              
              ", line " + parseError.line +
              ", offset " + parseError.offset +
              ", context " + prettify(parseError.preContext, TRUE) +
              ", rules: " + prettify(rule, TRUE));
    }
    delete t;
}




void TransliteratorTest::TestDisplayName() {
#if UCONFIG_NO_FORMATTING
    logln("Skipping, UCONFIG_NO_FORMATTING is set\n");
    return;
#else
    static const char* DATA[] = {
        
        
        
        
        
        "Any-Hex", "Any to Hex Escape", "Hex Escape to Any",
        
        
        "Any-Hex/Perl", "Any to Hex Escape/Perl", "Hex Escape to Any/Perl",
        
        
        "NFC", "Any to NFC", "Any to NFD",
    };

    int32_t DATA_length = sizeof(DATA) / sizeof(DATA[0]);
    
    Locale US("en", "US");
    
    for (int32_t i=0; i<DATA_length; i+=3) {
        UnicodeString name;
        Transliterator::getDisplayName(DATA[i], US, name);
        if (name != DATA[i+1]) {
            dataerrln((UnicodeString)"FAIL: " + DATA[i] + ".getDisplayName() => " +
                  name + ", expected " + DATA[i+1]);
        } else {
            logln((UnicodeString)"Ok: " + DATA[i] + ".getDisplayName() => " + name);
        }
        UErrorCode ec = U_ZERO_ERROR;
        UParseError pe;
        Transliterator *t = Transliterator::createInstance(DATA[i], UTRANS_REVERSE, pe, ec);
        if (U_FAILURE(ec)) {
            delete t;
            dataerrln("FAIL: createInstance failed - %s", u_errorName(ec));
            continue;
        }
        name = Transliterator::getDisplayName(t->getID(), US, name);
        if (name != DATA[i+2]) {
            dataerrln((UnicodeString)"FAIL: " + t->getID() + ".getDisplayName() => " +
                  name + ", expected " + DATA[i+2]);
        } else {
            logln((UnicodeString)"Ok: " + t->getID() + ".getDisplayName() => " + name);
        }
        delete t;
    }
#endif
}

void TransliteratorTest::TestSpecialCases(void) {
    const UnicodeString registerRules[] = {
        "Any-Dev1", "x > X; y > Y;",
        "Any-Dev2", "XY > Z",
        "Greek-Latin/FAKE", 
            CharsToUnicodeString
            ("[^[:L:][:M:]] { \\u03bc\\u03c0 > b ; \\u03bc\\u03c0 } [^[:L:][:M:]] > b ; [^[:L:][:M:]] { [\\u039c\\u03bc][\\u03a0\\u03c0] > B ; [\\u039c\\u03bc][\\u03a0\\u03c0] } [^[:L:][:M:]] > B ;"),
        "" 
    };

    const UnicodeString testCases[] = {
        
        
        "NFD" , CharsToUnicodeString("a\\u0300 \\u00E0 \\u1100\\u1161 \\uFF76\\uFF9E\\u03D3"), "",
        "NFC" , CharsToUnicodeString("a\\u0300 \\u00E0 \\u1100\\u1161 \\uFF76\\uFF9E\\u03D3"), "",
        "NFKD", CharsToUnicodeString("a\\u0300 \\u00E0 \\u1100\\u1161 \\uFF76\\uFF9E\\u03D3"), "",
        "NFKC", CharsToUnicodeString("a\\u0300 \\u00E0 \\u1100\\u1161 \\uFF76\\uFF9E\\u03D3"), "",

        
        "Greek-Latin/UNGEGN", CharsToUnicodeString("(\\u03BC\\u03C0)"), "(b)",
        "Greek-Latin/FAKE", CharsToUnicodeString("(\\u03BC\\u03C0)"), "(b)",
    
        
        "nfd;Dev1;Dev2;nfc", "xy", "Z",

        
        "Title", CharsToUnicodeString("ab'cD ffi\\u0131I\\u0130 \\u01C7\\u01C8\\u01C9 ") + DESERET_dee + DESERET_DEE, 
                 CharsToUnicodeString("Ab'cd Ffi\\u0131ii\\u0307 \\u01C8\\u01C9\\u01C9 ") + DESERET_DEE + DESERET_dee, 
                 
        
        



        "Upper", CharsToUnicodeString("ab'cD \\uFB00i\\u0131I\\u0130 \\u01C7\\u01C8\\u01C9 ") + DESERET_dee + DESERET_DEE, 
                 CharsToUnicodeString("AB'CD FFIII\\u0130 \\u01C7\\u01C7\\u01C7 ") + DESERET_DEE + DESERET_DEE,
        "Lower", CharsToUnicodeString("ab'cD \\uFB00i\\u0131I\\u0130 \\u01C7\\u01C8\\u01C9 ") + DESERET_dee + DESERET_DEE, 
                 CharsToUnicodeString("ab'cd \\uFB00i\\u0131ii\\u0307 \\u01C9\\u01C9\\u01C9 ") + DESERET_dee + DESERET_dee,
    
        "Upper", CharsToUnicodeString("ab'cD \\uFB00i\\u0131I\\u0130 \\u01C7\\u01C8\\u01C9 ") + DESERET_dee + DESERET_DEE, "",
        "Lower", CharsToUnicodeString("ab'cD \\uFB00i\\u0131I\\u0130 \\u01C7\\u01C8\\u01C9 ") + DESERET_dee + DESERET_DEE, "",

         
        "Greek-Latin/UNGEGN",  CharsToUnicodeString("\\u03C3 \\u03C3\\u03C2 \\u03C2\\u03C3"), 
                               CharsToUnicodeString("s ss s\\u0331s\\u0331") ,
        "Latin-Greek/UNGEGN",  CharsToUnicodeString("s ss s\\u0331s\\u0331"), 
                               CharsToUnicodeString("\\u03C3 \\u03C3\\u03C2 \\u03C2\\u03C3") ,
        "Greek-Latin",  CharsToUnicodeString("\\u03C3 \\u03C3\\u03C2 \\u03C2\\u03C3"), 
                        CharsToUnicodeString("s ss s\\u0331s\\u0331") ,
        "Latin-Greek",  CharsToUnicodeString("s ss s\\u0331s\\u0331"), 
                        CharsToUnicodeString("\\u03C3 \\u03C3\\u03C2 \\u03C2\\u03C3"),
        
        
        
        
        "Upper", CharsToUnicodeString("tat\\u02B9\\u00E2na"),
                 CharsToUnicodeString("TAT\\u02B9\\u00C2NA"),
        "Lower", CharsToUnicodeString("TAT\\u02B9\\u00C2NA"),
                 CharsToUnicodeString("tat\\u02B9\\u00E2na"),
        "Title", CharsToUnicodeString("tat\\u02B9\\u00E2na"),
                 CharsToUnicodeString("Tat\\u02B9\\u00E2na"),

        "" 
    };

    UParseError pos;
    int32_t i;
    for (i = 0; registerRules[i].length()!=0; i+=2) {
        UErrorCode status = U_ZERO_ERROR;

        Transliterator *t = Transliterator::createFromRules(registerRules[0+i], 
            registerRules[i+1], UTRANS_FORWARD, pos, status);
        if (U_FAILURE(status)) {
            dataerrln("Fails: Unable to create the transliterator from rules. - %s", u_errorName(status));
        } else {
            Transliterator::registerInstance(t);
        }
    }
    for (i = 0; testCases[i].length()!=0; i+=3) {
        UErrorCode ec = U_ZERO_ERROR;
        UParseError pe;
        const UnicodeString& name = testCases[i];
        Transliterator *t = Transliterator::createInstance(name, UTRANS_FORWARD, pe, ec);
        if (U_FAILURE(ec)) {
            dataerrln((UnicodeString)"FAIL: Couldn't create " + name + " - " + u_errorName(ec));
            delete t;
            continue;
        }
        const UnicodeString& id = t->getID();
        const UnicodeString& source = testCases[i+1];
        UnicodeString target;

        
        
        if (testCases[i+2].length() > 0) {
            target = testCases[i+2];
        } else if (0==id.caseCompare("NFD", U_FOLD_CASE_DEFAULT)) {
            Normalizer::normalize(source, UNORM_NFD, 0, target, ec);
        } else if (0==id.caseCompare("NFC", U_FOLD_CASE_DEFAULT)) {
            Normalizer::normalize(source, UNORM_NFC, 0, target, ec);
        } else if (0==id.caseCompare("NFKD", U_FOLD_CASE_DEFAULT)) {
            Normalizer::normalize(source, UNORM_NFKD, 0, target, ec);
        } else if (0==id.caseCompare("NFKC", U_FOLD_CASE_DEFAULT)) {
            Normalizer::normalize(source, UNORM_NFKC, 0, target, ec);
        } else if (0==id.caseCompare("Lower", U_FOLD_CASE_DEFAULT)) {
            target = source;
            target.toLower(Locale::getUS());
        } else if (0==id.caseCompare("Upper", U_FOLD_CASE_DEFAULT)) {
            target = source;
            target.toUpper(Locale::getUS());
        }
        if (U_FAILURE(ec)) {
            errln((UnicodeString)"FAIL: Internal error normalizing " + source);
            continue;
        }

        expect(*t, source, target);
        delete t;
    }
    for (i = 0; registerRules[i].length()!=0; i+=2) {
        Transliterator::unregister(registerRules[i]);
    }
}

char* Char32ToEscapedChars(UChar32 ch, char* buffer) {
    if (ch <= 0xFFFF) {
        sprintf(buffer, "\\u%04x", (int)ch);
    } else {
        sprintf(buffer, "\\U%08x", (int)ch);
    }
    return buffer;
}

void TransliteratorTest::TestSurrogateCasing (void) {
    
    
    char buffer[20];
    UChar buffer2[20];
    UChar32 dee;
    U16_GET(DESERET_dee,0, 0, DESERET_dee.length(), dee);
    UnicodeString DEE(u_totitle(dee));
    if (DEE != DESERET_DEE) {
        err("Fails titlecase of surrogates");
        err(Char32ToEscapedChars(dee, buffer)); 
        err(", ");
        errln(Char32ToEscapedChars(DEE.char32At(0), buffer));
    }
        
    UnicodeString deeDEETest=DESERET_dee + DESERET_DEE;
    UnicodeString deedeeTest = DESERET_dee + DESERET_dee;
    UnicodeString DEEDEETest = DESERET_DEE + DESERET_DEE;
    UErrorCode status= U_ZERO_ERROR;

    u_strToUpper(buffer2, 20, deeDEETest.getBuffer(), deeDEETest.length(), NULL, &status);
    if (U_FAILURE(status) || (UnicodeString(buffer2)!= DEEDEETest)) {
        errln("Fails: Can't uppercase surrogates.");
    }
        
    status= U_ZERO_ERROR;
    u_strToLower(buffer2, 20, deeDEETest.getBuffer(), deeDEETest.length(), NULL, &status);
    if (U_FAILURE(status) || (UnicodeString(buffer2)!= deedeeTest)) {
        errln("Fails: Can't lowercase surrogates.");
    }
}

static void _trans(Transliterator& t, const UnicodeString& src,
                   UnicodeString& result) {
    result = src;
    t.transliterate(result);
}

static void _trans(const UnicodeString& id, const UnicodeString& src,
                   UnicodeString& result, UErrorCode ec) {
    UParseError pe;
    Transliterator *t = Transliterator::createInstance(id, UTRANS_FORWARD, pe, ec);
    if (U_SUCCESS(ec)) {
        _trans(*t, src, result);
    }
    delete t;
}

static UnicodeString _findMatch(const UnicodeString& source,
                                       const UnicodeString* pairs) {
    UnicodeString empty;
    for (int32_t i=0; pairs[i].length() > 0; i+=2) {
        if (0==source.caseCompare(pairs[i], U_FOLD_CASE_DEFAULT)) {
            return pairs[i+1];
        }
    }
    return empty;
}



void TransliteratorTest::TestIncrementalProgress(void) {
    UErrorCode ec = U_ZERO_ERROR;
    UnicodeString latinTest = "The Quick Brown Fox.";
    UnicodeString devaTest;
    _trans("Latin-Devanagari", latinTest, devaTest, ec);
    UnicodeString kataTest;
    _trans("Latin-Katakana", latinTest, kataTest, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: Internal error");
        return;
    }
    const UnicodeString tests[] = {
        "Any", latinTest,
        "Latin", latinTest,
        "Halfwidth", latinTest,
        "Devanagari", devaTest,
        "Katakana", kataTest,
        "" 
    };

    UnicodeString test("The Quick Brown Fox Jumped Over The Lazy Dog.");
    int32_t i = 0, j=0, k=0;
    int32_t sources = Transliterator::countAvailableSources();
    for (i = 0; i < sources; i++) {
        UnicodeString source;
        Transliterator::getAvailableSource(i, source);
        UnicodeString test = _findMatch(source, tests);
        if (test.length() == 0) {
            logln((UnicodeString)"Skipping " + source + "-X");
            continue;
        }
        int32_t targets = Transliterator::countAvailableTargets(source);
        for (j = 0; j < targets; j++) {
            UnicodeString target;
            Transliterator::getAvailableTarget(j, source, target);
            int32_t variants = Transliterator::countAvailableVariants(source, target);
            for (k =0; k< variants; k++) {
                UnicodeString variant;
                UParseError err;
                UErrorCode status = U_ZERO_ERROR;

                Transliterator::getAvailableVariant(k, source, target, variant);
                UnicodeString id = source + "-" + target + "/" + variant;
                
                Transliterator *t = Transliterator::createInstance(id, UTRANS_FORWARD, err, status);
                if (U_FAILURE(status)) {
                    dataerrln((UnicodeString)"FAIL: Could not create " + id);
                    delete t;
                    continue;
                }
                status = U_ZERO_ERROR;
                CheckIncrementalAux(t, test);

                UnicodeString rev;
                _trans(*t, test, rev);
                Transliterator *inv = t->createInverse(status);
                if (U_FAILURE(status)) {
#if UCONFIG_NO_BREAK_ITERATION
                    
                    if (id.compare((UnicodeString)"Latin-Thai/") != 0)
#endif
                        errln((UnicodeString)"FAIL: Could not create inverse of " + id);

                    delete t;
                    delete inv;
                    continue;
                }
                CheckIncrementalAux(inv, rev);
                delete t;
                delete inv;
            }
        }
    }
}

void TransliteratorTest::CheckIncrementalAux(const Transliterator* t, 
                                                      const UnicodeString& input) {
    UErrorCode ec = U_ZERO_ERROR;
    UTransPosition pos;
    UnicodeString test = input;

    pos.contextStart = 0;
    pos.contextLimit = input.length();
    pos.start = 0;
    pos.limit = input.length();

    t->transliterate(test, pos, ec);
    if (U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: transliterate() error " + u_errorName(ec));
        return;
    }
    UBool gotError = FALSE;
    (void)gotError;    

    

    if (pos.start == 0 && pos.limit != 0 && t->getID() != "Hex-Any/Unicode") {
        errln((UnicodeString)"No Progress, " +
              t->getID() + ": " + formatInput(test, input, pos));
        gotError = TRUE;
    } else {
        logln((UnicodeString)"PASS Progress, " +
              t->getID() + ": " + formatInput(test, input, pos));
    }
    t->finishTransliteration(test, pos);
    if (pos.start != pos.limit) {
        errln((UnicodeString)"Incomplete, " +
              t->getID() + ": " + formatInput(test, input, pos));
        gotError = TRUE;
    }
}

void TransliteratorTest::TestFunction() {
    
    
    
    UnicodeString rule =
        "([:Lu:]) > $1 '(' &Lower( $1 ) '=' &Hex( &Any-Lower( $1 ) ) ')';";
    
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createFromRules("Test", rule, UTRANS_FORWARD, pe, ec);
    if (t == NULL) {
        dataerrln("FAIL: createFromRules failed - %s", u_errorName(ec));
        return;
    }
    
    UnicodeString r;
    t->toRules(r, TRUE);
    if (r == rule) {
        logln((UnicodeString)"OK: toRules() => " + r);
    } else {
        errln((UnicodeString)"FAIL: toRules() => " + r +
              ", expected " + rule);
    }
    
    expect(*t, "The Quick Brown Fox",
           UNICODE_STRING_SIMPLE("T(t=\\u0074)he Q(q=\\u0071)uick B(b=\\u0062)rown F(f=\\u0066)ox"));

    delete t;
}

void TransliteratorTest::TestInvalidBackRef(void) {
    UnicodeString rule =  ". > $1;";
    UnicodeString rule2 =CharsToUnicodeString("(.) <> &hex/unicode($1) &name($1); . > $1; [{}] >\\u0020;");
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator *t = Transliterator::createFromRules("Test", rule, UTRANS_FORWARD, pe, ec);
    Transliterator *t2 = Transliterator::createFromRules("Test2", rule2, UTRANS_FORWARD, pe, ec);

    if (t != NULL) {
        errln("FAIL: createFromRules should have returned NULL");
        delete t;
    }

    if (t2 != NULL) {
        errln("FAIL: createFromRules should have returned NULL");
        delete t2;
    }

    if (U_SUCCESS(ec)) {
        errln("FAIL: Ok: . > $1; => no error");
    } else {
        logln((UnicodeString)"Ok: . > $1; => " + u_errorName(ec));
    }
}

void TransliteratorTest::TestMulticharStringSet() {
    
    const char* rule =
        "       [{aa}]       > x;"
        "         a          > y;"
        "       [b{bc}]      > z;"
        "[{gd}] { e          > q;"
        "         e } [{fg}] > r;" ;
        
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;
    Transliterator* t = Transliterator::createFromRules("Test", rule, UTRANS_FORWARD, pe, ec);
    if (t == NULL || U_FAILURE(ec)) {
        delete t;
        errln("FAIL: createFromRules failed");
        return;
    }
        
    expect(*t, "a aa ab bc d gd de gde gdefg ddefg",
           "y x yz z d gd de gdq gdqfg ddrfg");
    delete t;

    
    
    rule =
        "    [a {ab} {abc}]    > x;"
        "           b          > y;"
        "           c          > z;"
        " q [t {st} {rst}] { e > p;" ;
        
    t = Transliterator::createFromRules("Test", rule, UTRANS_FORWARD, pe, ec);
    if (t == NULL || U_FAILURE(ec)) {
        delete t;
        errln("FAIL: createFromRules failed");
        return;
    }
        
    expect(*t, "a ab abc qte qste qrste",
           "x x x qtp qstp qrstp");
    delete t;
}




Transliterator* _TUFF[4];
UnicodeString* _TUFID[4];

static Transliterator* U_EXPORT2 _TUFFactory(const UnicodeString& ,
                                   Transliterator::Token context) {
    return _TUFF[context.integer]->clone();
}

static void _TUFReg(const UnicodeString& ID, Transliterator* t, int32_t n) {
    _TUFF[n] = t;
    _TUFID[n] = new UnicodeString(ID);
    Transliterator::registerFactory(ID, _TUFFactory, Transliterator::integerToken(n));
}

static void _TUFUnreg(int32_t n) {
    if (_TUFF[n] != NULL) {
        Transliterator::unregister(*_TUFID[n]);
        delete _TUFF[n];
        delete _TUFID[n];
    }
}








void TransliteratorTest::TestUserFunction() {
 
    Transliterator* t;
    UParseError pe;
    UErrorCode ec = U_ZERO_ERROR;

    
    int32_t i;
    for (i=0; i<4; ++i) {
        _TUFF[i] = NULL;
    }

    
    t = Transliterator::createFromRules("gif",
                                        UNICODE_STRING_SIMPLE("'\\'u(..)(..) > '<img src=\"http://www.unicode.org/gifs/24/' $1 '/U' $1$2 '.gif\">';"),
                                        UTRANS_FORWARD, pe, ec);
    if (t == NULL || U_FAILURE(ec)) {
        dataerrln((UnicodeString)"FAIL: createFromRules gif " + u_errorName(ec));
        return;
    }
    _TUFReg("Any-gif", t, 0);

    t = Transliterator::createFromRules("RemoveCurly",
                                        UNICODE_STRING_SIMPLE("[\\{\\}] > ; '\\N' > ;"),
                                        UTRANS_FORWARD, pe, ec);
    if (t == NULL || U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: createFromRules RemoveCurly " + u_errorName(ec));
        goto FAIL;
    }
    expect(*t, UNICODE_STRING_SIMPLE("\\N{name}"), "name");
    _TUFReg("Any-RemoveCurly", t, 1);

    logln("Trying &hex");
    t = Transliterator::createFromRules("hex2",
                                        "(.) > &hex($1);",
                                        UTRANS_FORWARD, pe, ec);
    if (t == NULL || U_FAILURE(ec)) {
        errln("FAIL: createFromRules");
        goto FAIL;
    }
    logln("Registering");
    _TUFReg("Any-hex2", t, 2);
    t = Transliterator::createInstance("Any-hex2", UTRANS_FORWARD, ec);
    if (t == NULL || U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: createInstance Any-hex2 " + u_errorName(ec));
        goto FAIL;
    }
    expect(*t, "abc", UNICODE_STRING_SIMPLE("\\u0061\\u0062\\u0063"));
    delete t;

    logln("Trying &gif");
    t = Transliterator::createFromRules("gif2",
                                        "(.) > &Gif(&Hex2($1));",
                                        UTRANS_FORWARD, pe, ec);
    if (t == NULL || U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: createFromRules gif2 " + u_errorName(ec));
        goto FAIL;
    }
    logln("Registering");
    _TUFReg("Any-gif2", t, 3);
    t = Transliterator::createInstance("Any-gif2", UTRANS_FORWARD, ec);
    if (t == NULL || U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: createInstance Any-gif2 " + u_errorName(ec));
        goto FAIL;
    }
    expect(*t, "ab", "<img src=\"http://www.unicode.org/gifs/24/00/U0061.gif\">"
           "<img src=\"http://www.unicode.org/gifs/24/00/U0062.gif\">");
    delete t;

    
    t = Transliterator::createFromRules("test",
                                        "(.) > &Hex($1) ' ' &RemoveCurly(&Name($1)) ' ';",
                                        UTRANS_FORWARD, pe, ec);
    if (t == NULL || U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: createFromRules test " + u_errorName(ec));
        goto FAIL;
    }
    expect(*t, "abc",
           UNICODE_STRING_SIMPLE("\\u0061 LATIN SMALL LETTER A \\u0062 LATIN SMALL LETTER B \\u0063 LATIN SMALL LETTER C "));
    delete t;

 FAIL:
    for (i=0; i<4; ++i) {
        _TUFUnreg(i);
    }
}




void TransliteratorTest::TestAnyX(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* anyLatin =
        Transliterator::createInstance("Any-Latin", UTRANS_FORWARD, parseError, status);
    if (anyLatin==0) {
        dataerrln("FAIL: createInstance returned NULL - %s", u_errorName(status));
        delete anyLatin;
        return;
    }

    expect(*anyLatin,
           CharsToUnicodeString("greek:\\u03B1\\u03B2\\u03BA\\u0391\\u0392\\u039A hiragana:\\u3042\\u3076\\u304F cyrillic:\\u0430\\u0431\\u0446"),
           CharsToUnicodeString("greek:abkABK hiragana:abuku cyrillic:abc"));

    delete anyLatin;
}




void TransliteratorTest::TestAny(void) {
    UErrorCode status = U_ZERO_ERROR;
    
    
    UnicodeSet alphabetic("[:alphabetic:]", status);
    if (U_FAILURE(status)) {
        dataerrln("Failure: file %s, line %d, status = %s", __FILE__, __LINE__, u_errorName(status));
        return;
    }
    alphabetic.freeze();

    UnicodeString testString;
    for (int32_t i = 0; i < USCRIPT_CODE_LIMIT; i++) {
        const char *scriptName = uscript_getShortName((UScriptCode)i);
        if (scriptName == NULL) {
            errln("Failure: file %s, line %d: Script Code %d is invalid, ", __FILE__, __LINE__, i);
            return;
        }

        UnicodeSet sample;
        sample.applyPropertyAlias("script", scriptName, status);
        if (U_FAILURE(status)) {
            errln("Failure: file %s, line %d, status = %s", __FILE__, __LINE__, u_errorName(status));
            return;
        }
        sample.retainAll(alphabetic);
        for (int32_t count=0; count<5; count++) {
            UChar32 c = sample.charAt(count);
            if (c == -1) {
                break;
            }
            testString.append(c);
        }
    }

    UParseError parseError;
    Transliterator* anyLatin =
        Transliterator::createInstance("Any-Latin", UTRANS_FORWARD, parseError, status);
    if (U_FAILURE(status)) {
        dataerrln("Failure: file %s, line %d, status = %s", __FILE__, __LINE__, u_errorName(status));
        return;
    }

    logln(UnicodeString("Sample set for Any-Latin: ") + testString);
    anyLatin->transliterate(testString);
    logln(UnicodeString("Sample result for Any-Latin: ") + testString);
    delete anyLatin;
}






void TransliteratorTest::TestSourceTargetSet() {
    UErrorCode ec = U_ZERO_ERROR;

    
    const char* r =
        "a > b; "
        "r [x{lu}] > q;";

    
    UnicodeSet expSrc("[arx{lu}]", ec);

    
    UnicodeSet expTrg("[bq]", ec);

    UParseError pe;
    Transliterator* t = Transliterator::createFromRules("test", r, UTRANS_FORWARD, pe, ec);

    if (U_FAILURE(ec)) {
        delete t;
        errln("FAIL: Couldn't set up test");
        return;
    }

    UnicodeSet src; t->getSourceSet(src);
    UnicodeSet trg; t->getTargetSet(trg);

    if (src == expSrc && trg == expTrg) {
        UnicodeString a, b;
        logln((UnicodeString)"Ok: " +
              r + " => source = " + src.toPattern(a, TRUE) +
              ", target = " + trg.toPattern(b, TRUE));
    } else {
        UnicodeString a, b, c, d;
        errln((UnicodeString)"FAIL: " +
              r + " => source = " + src.toPattern(a, TRUE) +
              ", expected " + expSrc.toPattern(b, TRUE) +
              "; target = " + trg.toPattern(c, TRUE) +
              ", expected " + expTrg.toPattern(d, TRUE));
    }

    delete t;
}




void TransliteratorTest::TestPatternWhiteSpace() {
    
    const char* r = "a > \\u200E b;";
    
    UErrorCode ec = U_ZERO_ERROR;
    UParseError pe;
    Transliterator* t = Transliterator::createFromRules("test", CharsToUnicodeString(r), UTRANS_FORWARD, pe, ec);
    
    if (U_FAILURE(ec)) {
        errln("FAIL: Couldn't set up test");
    } else {
        expect(*t, "a", "b");
    }
    delete t;
    
    
    ec = U_ZERO_ERROR;
    UnicodeSet set(CharsToUnicodeString("[a \\u200E]"), ec);
    
    if (U_FAILURE(ec)) {
        errln("FAIL: Couldn't set up test");
    } else {
        if (set.contains(0x200E)) {
            errln("FAIL: U+200E not being ignored by UnicodeSet");
        }
    }
}



void TransliteratorTest::TestAllCodepoints(){
    UScriptCode code= USCRIPT_INVALID_CODE;
    char id[256]={'\0'};
    char abbr[256]={'\0'};
    char newId[256]={'\0'};
    char newAbbrId[256]={'\0'};
    char oldId[256]={'\0'};
    char oldAbbrId[256]={'\0'};

    UErrorCode status =U_ZERO_ERROR;
    UParseError pe;
    
    for(uint32_t i = 0; i<=0x10ffff; i++){
        code =  uscript_getScript(i,&status);
        if(code == USCRIPT_INVALID_CODE){
            dataerrln("uscript_getScript for codepoint \\U%08X failed.", i);
        }
        const char* myId = uscript_getName(code);
        if(!myId) {
          dataerrln("Valid script code returned NULL name. Check your data!");
          return;
        }
        uprv_strcpy(id,myId);
        uprv_strcpy(abbr,uscript_getShortName(code));

        uprv_strcpy(newId,"[:");
        uprv_strcat(newId,id);
        uprv_strcat(newId,":];NFD");

        uprv_strcpy(newAbbrId,"[:");
        uprv_strcat(newAbbrId,abbr);
        uprv_strcat(newAbbrId,":];NFD");

        if(uprv_strcmp(newId,oldId)!=0){
            Transliterator* t = Transliterator::createInstance(newId,UTRANS_FORWARD,pe,status);
            if(t==NULL || U_FAILURE(status)){
                dataerrln((UnicodeString)"FAIL: Could not create " + id + " - " + u_errorName(status));
            }
            delete t;
        }
        if(uprv_strcmp(newAbbrId,oldAbbrId)!=0){
            Transliterator* t = Transliterator::createInstance(newAbbrId,UTRANS_FORWARD,pe,status);
            if(t==NULL || U_FAILURE(status)){
                dataerrln((UnicodeString)"FAIL: Could not create " + id + " - " + u_errorName(status));
            }
            delete t;
        }
        uprv_strcpy(oldId,newId);
        uprv_strcpy(oldAbbrId, newAbbrId);

    }

} 

#define TEST_TRANSLIT_ID(id, cls) { \
  UErrorCode ec = U_ZERO_ERROR; \
  Transliterator* t = Transliterator::createInstance(id, UTRANS_FORWARD, ec); \
  if (U_FAILURE(ec)) { \
    dataerrln("FAIL: Couldn't create %s - %s", id, u_errorName(ec)); \
  } else { \
    if (t->getDynamicClassID() != cls::getStaticClassID()) { \
      errln("FAIL: " #cls " dynamic and static class ID mismatch"); \
    } \
    /* *t = *t; */ /*can't do this: coverage test for assignment op*/ \
  } \
  delete t; \
}

#define TEST_TRANSLIT_RULE(rule, cls) { \
  UErrorCode ec = U_ZERO_ERROR; \
  UParseError pe; \
  Transliterator* t = Transliterator::createFromRules("_", rule, UTRANS_FORWARD, pe, ec); \
  if (U_FAILURE(ec)) { \
    errln("FAIL: Couldn't create " rule); \
  } else { \
    if (t->getDynamicClassID() != cls ::getStaticClassID()) { \
      errln("FAIL: " #cls " dynamic and static class ID mismatch"); \
    } \
    /* *t = *t; */ /*can't do this: coverage test for assignment op*/ \
  } \
  delete t; \
}

void TransliteratorTest::TestBoilerplate() {
    TEST_TRANSLIT_ID("Any-Latin", AnyTransliterator);
    TEST_TRANSLIT_ID("Any-Hex", EscapeTransliterator);
    TEST_TRANSLIT_ID("Hex-Any", UnescapeTransliterator);
    TEST_TRANSLIT_ID("Lower", LowercaseTransliterator);
    TEST_TRANSLIT_ID("Upper", UppercaseTransliterator);
    TEST_TRANSLIT_ID("Title", TitlecaseTransliterator);
    TEST_TRANSLIT_ID("Null", NullTransliterator);
    TEST_TRANSLIT_ID("Remove", RemoveTransliterator);
    TEST_TRANSLIT_ID("Any-Name", UnicodeNameTransliterator);
    TEST_TRANSLIT_ID("Name-Any", NameUnicodeTransliterator);
    TEST_TRANSLIT_ID("NFD", NormalizationTransliterator);
    TEST_TRANSLIT_ID("Latin-Greek", CompoundTransliterator);
    TEST_TRANSLIT_RULE("a>b;", RuleBasedTransliterator);
}

void TransliteratorTest::TestAlternateSyntax() {
    
    
    
    
    expect(CharsToUnicodeString("a \\u2192 x; b \\u2190 y; c \\u2194 z"),
           "abc",
           "xbz");
    expect(CharsToUnicodeString("([:^ASCII:]) \\u2192 \\u2206Name($1);"),
           CharsToUnicodeString("<=\\u2190; >=\\u2192; <>=\\u2194; &=\\u2206"),
           UNICODE_STRING_SIMPLE("<=\\N{LEFTWARDS ARROW}; >=\\N{RIGHTWARDS ARROW}; <>=\\N{LEFT RIGHT ARROW}; &=\\N{INCREMENT}"));
}

static const char* BEGIN_END_RULES[] = {
    
    "abc > xy;"
    "aba > z;",

    








    "", 

    






    "", 

    






    "", 

    
    "abc > xy;"
    "::Null;"
    "aba > z;",

    
    "::Upper;"
    "ABC > xy;"
    "AB > x;"
    "C > z;"
    "::Upper;"
    "XYZ > p;"
    "XY > q;"
    "Z > r;"
    "::Upper;",

    
    "$ws = [[:Separator:][\\u0009-\\u000C]$];"
    "$delim = [\\-$ws];"
    "$ws $delim* > ' ';"
    "'-' $delim* > '-';",

    
    "::Null;"
    "$ws = [[:Separator:][\\u0009-\\u000C]$];"
    "$delim = [\\-$ws];"
    "$ws $delim* > ' ';"
    "'-' $delim* > '-';",

    
    "$ws = [[:Separator:][\\u0009-\\u000C]$];"
    "$delim = [\\-$ws];"
    "$ws $delim* > ' ';"
    "'-' $delim* > '-';"
    "::Null;",

    
    "$ws = [[:Separator:][\\u0009-\\u000C]$];"
    "$delim = [\\-$ws];"
    "::Null;"
    "$ws $delim* > ' ';"
    "'-' $delim* > '-';",

    








    "", 

    








    "", 

    
















    "", 

    
    "$ws = [[:Separator:][\\u0009-\\u000C]$];"
    "$delim = [\\-$ws];"
    "$ab = [ab];"
    "::Null;"
    "$ws $delim* > ' ';"
    "'-' $delim* > '-';"
    "::Null;"
    "$ab { ' ' } $ab > '-';"
    "c { ' ' > ;"
    "::Null;"
    "'a-a' > a\\%|a;",

    










    "", 

    
    "::[abc];"
    "abc > xy;"
    "::Null;"
    "aba > yz;"
    "::Upper;",

    











    "", 

    
    "::[abc];"
    "abc <> xy;"
    "::Null;"
    "aba <> yz;"
    "::Upper(Lower);"
    "::([XYZ]);"
};


























static const char* BEGIN_END_TEST_CASES[] = {
    
    BEGIN_END_RULES[0],  "abc ababc aba",        "xy zbc z",



    BEGIN_END_RULES[4],  "abc ababc aba",        "xy abxy z",
    BEGIN_END_RULES[5],  "abccabaacababcbc",     "PXAARXQBR",

    BEGIN_END_RULES[6],  "e   e - e---e-  e",    "e e e-e-e",
    BEGIN_END_RULES[7],  "e   e - e---e-  e",    "e e e-e-e",
    BEGIN_END_RULES[8],  "e   e - e---e-  e",    "e e e-e-e",
    BEGIN_END_RULES[9],  "e   e - e---e-  e",    "e e e-e-e",





    BEGIN_END_RULES[13], "e   e - e---e-  e",    "e e e-e-e",
    BEGIN_END_RULES[13], "a    a    a    a",     "a%a%a%a",
    BEGIN_END_RULES[13], "a a-b c b a",          "a%a-b cb-a",


    BEGIN_END_RULES[15], "abc xy ababc xyz aba", "XY xy ABXY xyz YZ",

    BEGIN_END_RULES[17], "abc xy ababc xyz aba", "XY xy ABXY xyz YZ"
};
static const int32_t BEGIN_END_TEST_CASES_length = (int32_t)(sizeof(BEGIN_END_TEST_CASES) / sizeof(BEGIN_END_TEST_CASES[0]));

void TransliteratorTest::TestBeginEnd() {
    
    int32_t i = 0;
    for (i = 0; i < BEGIN_END_TEST_CASES_length; i += 3) {
        expect((UnicodeString)"Test case #" + (i / 3),
               UnicodeString(BEGIN_END_TEST_CASES[i], -1, US_INV),
               UnicodeString(BEGIN_END_TEST_CASES[i + 1], -1, US_INV),
               UnicodeString(BEGIN_END_TEST_CASES[i + 2], -1, US_INV));
    }

    
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* reversed  = Transliterator::createFromRules("Reversed", UnicodeString(BEGIN_END_RULES[17]),
            UTRANS_REVERSE, parseError, status);
    if (reversed == 0 || U_FAILURE(status)) {
        reportParseError(UnicodeString("FAIL: Couldn't create reversed transliterator"), parseError, status);
    } else {
        expect(*reversed, UnicodeString("xy XY XYZ yz YZ"), UnicodeString("xy abc xaba yz aba"));
    }
    delete reversed;

    
    













}

void TransliteratorTest::TestBeginEndToRules() {
    
    
    
    
    
    for (int32_t i = 0; i < BEGIN_END_TEST_CASES_length; i += 3) {
        UParseError parseError;
        UErrorCode status = U_ZERO_ERROR;
        Transliterator* t = Transliterator::createFromRules("--", UnicodeString(BEGIN_END_TEST_CASES[i], -1, US_INV),
                UTRANS_FORWARD, parseError, status);
        if (U_FAILURE(status)) {
            reportParseError(UnicodeString("FAIL: Couldn't create transliterator"), parseError, status);
        } else {
            UnicodeString rules;
            t->toRules(rules, TRUE);
            Transliterator* t2 = Transliterator::createFromRules((UnicodeString)"Test case #" + (i / 3), rules,
                    UTRANS_FORWARD, parseError, status);
            if (U_FAILURE(status)) {
                reportParseError(UnicodeString("FAIL: Couldn't create transliterator from generated rules"),
                        parseError, status);
                delete t;
            } else {
                expect(*t2,
                       UnicodeString(BEGIN_END_TEST_CASES[i + 1], -1, US_INV),
                       UnicodeString(BEGIN_END_TEST_CASES[i + 2], -1, US_INV));
                delete t;
                delete t2;
            }
        }
    }

    
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* reversed = Transliterator::createFromRules("Reversed", UnicodeString(BEGIN_END_RULES[17]),
            UTRANS_REVERSE, parseError, status);
    if (U_FAILURE(status)) {
        reportParseError(UnicodeString("FAIL: Couldn't create reversed transliterator"), parseError, status);
    } else {
        UnicodeString rules;
        reversed->toRules(rules, FALSE);
        Transliterator* reversed2 = Transliterator::createFromRules("Reversed", rules, UTRANS_FORWARD,
                parseError, status);
        if (U_FAILURE(status)) {
            reportParseError(UnicodeString("FAIL: Couldn't create reversed transliterator from generated rules"),
                    parseError, status);
            delete reversed;
        } else {
            expect(*reversed2,
                   UnicodeString("xy XY XYZ yz YZ"),
                   UnicodeString("xy abc xaba yz aba"));
            delete reversed;
            delete reversed2;
        }
    }
}

void TransliteratorTest::TestRegisterAlias() {
    UnicodeString longID("Lower;[aeiou]Upper");
    UnicodeString shortID("Any-CapVowels");
    UnicodeString reallyShortID("CapVowels");

    Transliterator::registerAlias(shortID, longID);

    UErrorCode err = U_ZERO_ERROR;
    Transliterator* t1 = Transliterator::createInstance(longID, UTRANS_FORWARD, err);
    if (U_FAILURE(err)) {
        errln("Failed to instantiate transliterator with long ID");
        Transliterator::unregister(shortID);
        return;
    }
    Transliterator* t2 = Transliterator::createInstance(reallyShortID, UTRANS_FORWARD, err);
    if (U_FAILURE(err)) {
        errln("Failed to instantiate transliterator with short ID");
        delete t1;
        Transliterator::unregister(shortID);
        return;
    }

    if (t1->getID() != longID)
        errln("Transliterator instantiated with long ID doesn't have long ID");
    if (t2->getID() != reallyShortID)
        errln("Transliterator instantiated with short ID doesn't have short ID");

    UnicodeString rules1;
    UnicodeString rules2;

    t1->toRules(rules1, TRUE);
    t2->toRules(rules2, TRUE);
    if (rules1 != rules2)
        errln("Alias transliterators aren't the same");

    delete t1;
    delete t2;
    Transliterator::unregister(shortID);

    t1 = Transliterator::createInstance(shortID, UTRANS_FORWARD, err);
    if (U_SUCCESS(err)) {
        errln("Instantiation with short ID succeeded after short ID was unregistered");
        delete t1;
    }

    
    
    UnicodeString realID("Latin-Greek");
    UnicodeString fakeID("Latin-dlgkjdflkjdl");
    Transliterator::registerAlias(fakeID, realID);

    err = U_ZERO_ERROR;
    t1 = Transliterator::createInstance(realID, UTRANS_FORWARD, err);
    if (U_FAILURE(err)) {
        dataerrln("Failed to instantiate transliterator with real ID - %s", u_errorName(err));
        Transliterator::unregister(realID);
        return;
    }
    t2 = Transliterator::createInstance(fakeID, UTRANS_FORWARD, err);
    if (U_FAILURE(err)) {
        errln("Failed to instantiate transliterator with fake ID");
        delete t1;
        Transliterator::unregister(realID);
        return;
    }

    t1->toRules(rules1, TRUE);
    t2->toRules(rules2, TRUE);
    if (rules1 != rules2)
        errln("Alias transliterators aren't the same");

    delete t1;
    delete t2;
    Transliterator::unregister(fakeID);
}

void TransliteratorTest::TestRuleStripping() {
    



    static const UChar rule[] = {
        0x0023,0x0020,0x000D,0x000A,
        0xE001,0x003E,0x0C01,0x003B,0x0020,0x0023,0x0020,0x0053,0x0049,0x0047,0x004E,0
    };
    static const UChar expectedRule[] = {
        0xE001,0x003E,0x0C01,0x003B,0
    };
    UChar result[sizeof(rule)/sizeof(rule[0])];
    UErrorCode status = U_ZERO_ERROR;
    int32_t len = utrans_stripRules(rule, (int32_t)(sizeof(rule)/sizeof(rule[0])), result, &status);
    if (len != u_strlen(expectedRule)) {
        errln("utrans_stripRules return len = %d", len);
    }
    if (u_strncmp(expectedRule, result, len) != 0) {
        errln("utrans_stripRules did not return expected string");
    }
}




void TransliteratorTest::TestHalfwidthFullwidth(void) {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* hf = Transliterator::createInstance("Halfwidth-Fullwidth", UTRANS_FORWARD, parseError, status);
    Transliterator* fh = Transliterator::createInstance("Fullwidth-Halfwidth", UTRANS_FORWARD, parseError, status);
    if (hf == 0 || fh == 0) {
        dataerrln("FAIL: createInstance failed - %s", u_errorName(status));
        delete hf;
        delete fh;
        return;
    }

    
    
    
    
    
    const char* DATA[] = {
        "both",
        "\\uFFE9\\uFFEA\\uFFEB\\uFFEC\\u0061\\uFF71\\u00AF\\u0020",
        "\\u2190\\u2191\\u2192\\u2193\\uFF41\\u30A2\\uFFE3\\u3000",
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    for (int32_t i=0; i<DATA_length; i+=3) {
        UnicodeString h = CharsToUnicodeString(DATA[i+1]);
        UnicodeString f = CharsToUnicodeString(DATA[i+2]);
        switch (*DATA[i]) {
        case 0x68: 
            expect(*hf, h, f);
            break;
        case 0x66: 
            expect(*fh, f, h);
            break;
        case 0x62: 
            expect(*hf, h, f);
            expect(*fh, f, h);
            break;
        }
    }
    delete hf;
    delete fh;
}


    




void TransliteratorTest::TestThai(void) {
#if !UCONFIG_NO_BREAK_ITERATION
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* tr = Transliterator::createInstance("Any-Latin", UTRANS_FORWARD, parseError, status);
    if (tr == 0) {
        dataerrln("FAIL: createInstance failed - %s", u_errorName(status));
        return;
    }
    if (U_FAILURE(status)) {
        errln("FAIL: createInstance failed with %s", u_errorName(status));
        return;
    }
    const char *thaiText = 
        "\\u0e42\\u0e14\\u0e22\\u0e1e\\u0e37\\u0e49\\u0e19\\u0e10\\u0e32\\u0e19\\u0e41\\u0e25\\u0e49\\u0e27, \\u0e04\\u0e2d"
        "\\u0e21\\u0e1e\\u0e34\\u0e27\\u0e40\\u0e15\\u0e2d\\u0e23\\u0e4c\\u0e08\\u0e30\\u0e40\\u0e01\\u0e35\\u0e48\\u0e22"
        "\\u0e27\\u0e02\\u0e49\\u0e2d\\u0e07\\u0e01\\u0e31\\u0e1a\\u0e40\\u0e23\\u0e37\\u0e48\\u0e2d\\u0e07\\u0e02\\u0e2d"
        "\\u0e07\\u0e15\\u0e31\\u0e27\\u0e40\\u0e25\\u0e02. \\u0e04\\u0e2d\\u0e21\\u0e1e\\u0e34\\u0e27\\u0e40\\u0e15\\u0e2d"
        "\\u0e23\\u0e4c\\u0e08\\u0e31\\u0e14\\u0e40\\u0e01\\u0e47\\u0e1a\\u0e15\\u0e31\\u0e27\\u0e2d\\u0e31\\u0e01\\u0e29"
        "\\u0e23\\u0e41\\u0e25\\u0e30\\u0e2d\\u0e31\\u0e01\\u0e02\\u0e23\\u0e30\\u0e2d\\u0e37\\u0e48\\u0e19\\u0e46 \\u0e42"
        "\\u0e14\\u0e22\\u0e01\\u0e32\\u0e23\\u0e01\\u0e33\\u0e2b\\u0e19\\u0e14\\u0e2b\\u0e21\\u0e32\\u0e22\\u0e40\\u0e25"
        "\\u0e02\\u0e43\\u0e2b\\u0e49\\u0e2a\\u0e33\\u0e2b\\u0e23\\u0e31\\u0e1a\\u0e41\\u0e15\\u0e48\\u0e25\\u0e30\\u0e15"
        "\\u0e31\\u0e27. \\u0e01\\u0e48\\u0e2d\\u0e19\\u0e2b\\u0e19\\u0e49\\u0e32\\u0e17\\u0e35\\u0e48\\u0e4a Unicode \\u0e08"
        "\\u0e30\\u0e16\\u0e39\\u0e01\\u0e2a\\u0e23\\u0e49\\u0e32\\u0e07\\u0e02\\u0e36\\u0e49\\u0e19, \\u0e44\\u0e14\\u0e49"
        "\\u0e21\\u0e35\\u0e23\\u0e30\\u0e1a\\u0e1a encoding \\u0e2d\\u0e22\\u0e39\\u0e48\\u0e2b\\u0e25\\u0e32\\u0e22\\u0e23"
        "\\u0e49\\u0e2d\\u0e22\\u0e23\\u0e30\\u0e1a\\u0e1a\\u0e2a\\u0e33\\u0e2b\\u0e23\\u0e31\\u0e1a\\u0e01\\u0e32\\u0e23"
        "\\u0e01\\u0e33\\u0e2b\\u0e19\\u0e14\\u0e2b\\u0e21\\u0e32\\u0e22\\u0e40\\u0e25\\u0e02\\u0e40\\u0e2b\\u0e25\\u0e48"
        "\\u0e32\\u0e19\\u0e35\\u0e49. \\u0e44\\u0e21\\u0e48\\u0e21\\u0e35 encoding \\u0e43\\u0e14\\u0e17\\u0e35\\u0e48"
        "\\u0e21\\u0e35\\u0e08\\u0e33\\u0e19\\u0e27\\u0e19\\u0e15\\u0e31\\u0e27\\u0e2d\\u0e31\\u0e01\\u0e02\\u0e23\\u0e30"
        "\\u0e21\\u0e32\\u0e01\\u0e40\\u0e1e\\u0e35\\u0e22\\u0e07\\u0e1e\\u0e2d: \\u0e22\\u0e01\\u0e15\\u0e31\\u0e27\\u0e2d"
        "\\u0e22\\u0e48\\u0e32\\u0e07\\u0e40\\u0e0a\\u0e48\\u0e19, \\u0e40\\u0e09\\u0e1e\\u0e32\\u0e30\\u0e43\\u0e19\\u0e01"
        "\\u0e25\\u0e38\\u0e48\\u0e21\\u0e2a\\u0e2b\\u0e20\\u0e32\\u0e1e\\u0e22\\u0e38\\u0e42\\u0e23\\u0e1b\\u0e40\\u0e1e"
        "\\u0e35\\u0e22\\u0e07\\u0e41\\u0e2b\\u0e48\\u0e07\\u0e40\\u0e14\\u0e35\\u0e22\\u0e27 \\u0e01\\u0e47\\u0e15\\u0e49"
        "\\u0e2d\\u0e07\\u0e01\\u0e32\\u0e23\\u0e2b\\u0e25\\u0e32\\u0e22 encoding \\u0e43\\u0e19\\u0e01\\u0e32\\u0e23\\u0e04"
        "\\u0e23\\u0e2d\\u0e1a\\u0e04\\u0e25\\u0e38\\u0e21\\u0e17\\u0e38\\u0e01\\u0e20\\u0e32\\u0e29\\u0e32\\u0e43\\u0e19"
        "\\u0e01\\u0e25\\u0e38\\u0e48\\u0e21. \\u0e2b\\u0e23\\u0e37\\u0e2d\\u0e41\\u0e21\\u0e49\\u0e41\\u0e15\\u0e48\\u0e43"
        "\\u0e19\\u0e20\\u0e32\\u0e29\\u0e32\\u0e40\\u0e14\\u0e35\\u0e48\\u0e22\\u0e27 \\u0e40\\u0e0a\\u0e48\\u0e19 \\u0e20"
        "\\u0e32\\u0e29\\u0e32\\u0e2d\\u0e31\\u0e07\\u0e01\\u0e24\\u0e29 \\u0e01\\u0e47\\u0e44\\u0e21\\u0e48\\u0e21\\u0e35"
        " encoding \\u0e43\\u0e14\\u0e17\\u0e35\\u0e48\\u0e40\\u0e1e\\u0e35\\u0e22\\u0e07\\u0e1e\\u0e2d\\u0e2a\\u0e33\\u0e2b"
        "\\u0e23\\u0e31\\u0e1a\\u0e17\\u0e38\\u0e01\\u0e15\\u0e31\\u0e27\\u0e2d\\u0e31\\u0e01\\u0e29\\u0e23, \\u0e40\\u0e04"
        "\\u0e23\\u0e37\\u0e48\\u0e2d\\u0e07\\u0e2b\\u0e21\\u0e32\\u0e22\\u0e27\\u0e23\\u0e23\\u0e04\\u0e15\\u0e2d\\u0e19"
        " \\u0e41\\u0e25\\u0e30\\u0e2a\\u0e31\\u0e0d\\u0e25\\u0e31\\u0e01\\u0e29\\u0e13\\u0e4c\\u0e17\\u0e32\\u0e07\\u0e40"
        "\\u0e17\\u0e04\\u0e19\\u0e34\\u0e04\\u0e17\\u0e35\\u0e48\\u0e43\\u0e0a\\u0e49\\u0e01\\u0e31\\u0e19\\u0e2d\\u0e22"
        "\\u0e39\\u0e48\\u0e17\\u0e31\\u0e48\\u0e27\\u0e44\\u0e1b.";
    
    const char *latinText =     
        "doy ph\\u1ee5\\u0304\\u0302n \\u1e6d\\u0304h\\u0101n l\\u00e6\\u0302w, khxmphiwtexr\\u0312 ca ke\\u012b\\u0300"
        "ywk\\u0304\\u0125xng k\\u1ea1b re\\u1ee5\\u0304\\u0300xng k\\u0304hxng t\\u1ea1wlek\\u0304h. khxmphiwtexr"
        "\\u0312 c\\u1ea1d k\\u0115b t\\u1ea1w x\\u1ea1ks\\u0304\\u02b9r l\\u00e6a x\\u1ea1kk\\u0304h ra x\\u1ee5\\u0304"
        "\\u0300n\\u00ab doy k\\u0101r k\\u1ea3h\\u0304nd h\\u0304m\\u0101ylek\\u0304h h\\u0304\\u0131\\u0302 s\\u0304"
        "\\u1ea3h\\u0304r\\u1ea1b t\\u00e6\\u0300la t\\u1ea1w. k\\u0300xn h\\u0304n\\u0302\\u0101 th\\u012b\\u0300\\u0301"
        " Unicode ca t\\u0304h\\u016bk s\\u0304r\\u0302\\u0101ng k\\u0304h\\u1ee5\\u0302n, d\\u1ecb\\u0302 m\\u012b "
        "rabb encoding xy\\u016b\\u0300 h\\u0304l\\u0101y r\\u0302xy rabb s\\u0304\\u1ea3h\\u0304r\\u1ea1b k\\u0101"
        "r k\\u1ea3h\\u0304nd h\\u0304m\\u0101ylek\\u0304h h\\u0304el\\u0300\\u0101 n\\u012b\\u0302. m\\u1ecb\\u0300m"
        "\\u012b encoding d\\u0131 th\\u012b\\u0300 m\\u012b c\\u1ea3nwn t\\u1ea1w x\\u1ea1kk\\u0304hra m\\u0101k p"
        "he\\u012byng phx: yk t\\u1ea1wx\\u1ef3\\u0101ng ch\\u00e8n, c\\u0304heph\\u0101a n\\u0131 kl\\u00f9m s\\u0304"
        "h\\u0304p\\u0323h\\u0101ph yurop phe\\u012byng h\\u0304\\u00e6\\u0300ng de\\u012byw k\\u0306 t\\u0302xngk\\u0101"
        "r h\\u0304l\\u0101y encoding n\\u0131 k\\u0101r khrxbkhlum thuk p\\u0323h\\u0101s\\u0304\\u02b9\\u0101 n\\u0131"
        " kl\\u00f9m. h\\u0304r\\u1ee5\\u0304x m\\u00e6\\u0302t\\u00e6\\u0300 n\\u0131 p\\u0323h\\u0101s\\u0304\\u02b9"
        "\\u0101 de\\u012b\\u0300yw ch\\u00e8n p\\u0323h\\u0101s\\u0304\\u02b9\\u0101 x\\u1ea1ngkvs\\u0304\\u02b9 k\\u0306"
        " m\\u1ecb\\u0300m\\u012b encoding d\\u0131 th\\u012b\\u0300 phe\\u012byng phx s\\u0304\\u1ea3h\\u0304r\\u1ea1"
        "b thuk t\\u1ea1w x\\u1ea1ks\\u0304\\u02b9r, kher\\u1ee5\\u0304\\u0300xngh\\u0304m\\u0101y wrrkh txn l\\u00e6"
        "a s\\u0304\\u1ea1\\u1ef5l\\u1ea1ks\\u0304\\u02b9\\u1e47\\u0312 th\\u0101ng thekhnikh th\\u012b\\u0300 ch\\u0131"
        "\\u0302 k\\u1ea1n xy\\u016b\\u0300 th\\u1ea1\\u0300wp\\u1ecb.";
    
    
    UnicodeString  xlitText(thaiText);
    xlitText = xlitText.unescape();
    tr->transliterate(xlitText);

    UnicodeString expectedText(latinText);
    expectedText = expectedText.unescape();
    expect(*tr, xlitText, expectedText);
    
    delete tr;
#endif
}





void TransliteratorTest::expectT(const UnicodeString& id,
                                 const UnicodeString& source,
                                 const UnicodeString& expectedResult) {
    UErrorCode ec = U_ZERO_ERROR;
    UParseError pe;
    Transliterator *t = Transliterator::createInstance(id, UTRANS_FORWARD, pe, ec);
    if (U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: Could not create " + id + " -  " + u_errorName(ec));
        delete t;
        return;
    }
    expect(*t, source, expectedResult);
    delete t;
}

void TransliteratorTest::reportParseError(const UnicodeString& message,
                                          const UParseError& parseError,
                                          const UErrorCode& status) {
    dataerrln(message +
          
          ", line " + parseError.line +
          ", offset " + parseError.offset +
          ", pre-context " + prettify(parseError.preContext, TRUE) +
          ", post-context " + prettify(parseError.postContext,TRUE) +
          ", Error: " + u_errorName(status));
}

void TransliteratorTest::expect(const UnicodeString& rules,
                                const UnicodeString& source,
                                const UnicodeString& expectedResult,
                                UTransPosition *pos) {
    expect("<ID>", rules, source, expectedResult, pos);
}

void TransliteratorTest::expect(const UnicodeString& id,
                                const UnicodeString& rules,
                                const UnicodeString& source,
                                const UnicodeString& expectedResult,
                                UTransPosition *pos) {
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    Transliterator* t = Transliterator::createFromRules(id, rules, UTRANS_FORWARD, parseError, status);
    if (U_FAILURE(status)) {
        reportParseError(UnicodeString("Couldn't create transliterator from ") + rules, parseError, status);
    } else {
        expect(*t, source, expectedResult, pos);
    }
    delete t;
}

void TransliteratorTest::expect(const Transliterator& t,
                                const UnicodeString& source,
                                const UnicodeString& expectedResult,
                                const Transliterator& reverseTransliterator) {
    expect(t, source, expectedResult);
    expect(reverseTransliterator, expectedResult, source);
}

void TransliteratorTest::expect(const Transliterator& t,
                                const UnicodeString& source,
                                const UnicodeString& expectedResult,
                                UTransPosition *pos) {
    if (pos == 0) {
        UnicodeString result(source);
        t.transliterate(result);
        expectAux(t.getID() + ":String", source, result, expectedResult);
    }
    UTransPosition index={0, 0, 0, 0};
    if (pos != 0) {
        index = *pos;
    }

    UnicodeString rsource(source);
    if (pos == 0) {
        t.transliterate(rsource);
    } else {
        
        t.finishTransliteration(rsource, *pos);
    }
    expectAux(t.getID() + ":Replaceable", source, rsource, expectedResult);

    
    
    UnicodeString log;
    rsource.remove();
    if (pos != 0) {
        rsource = source;
        formatInput(log, rsource, index);
        log.append(" -> ");
        UErrorCode status = U_ZERO_ERROR;
        t.transliterate(rsource, index, status);
        formatInput(log, rsource, index);
    } else {
        for (int32_t i=0; i<source.length(); ++i) {
            if (i != 0) {
                log.append(" + ");
            }
            log.append(source.charAt(i)).append(" -> ");
            UErrorCode status = U_ZERO_ERROR;
            t.transliterate(rsource, index, source.charAt(i), status);
            formatInput(log, rsource, index);
        }
    }
    
    
    
    
    t.finishTransliteration(rsource, index);
    log.append(" => ").append(rsource);

    expectAux(t.getID() + ":Keyboard", log,
              rsource == expectedResult,
              expectedResult);
}

    





UnicodeString& TransliteratorTest::formatInput(UnicodeString &appendTo,
                                               const UnicodeString& input,
                                               const UTransPosition& pos) {
    
    
    
    if (0 <= pos.contextStart &&
        pos.contextStart <= pos.start &&
        pos.start <= pos.limit &&
        pos.limit <= pos.contextLimit &&
        pos.contextLimit <= input.length()) {

        UnicodeString a, b, c, d, e;
        input.extractBetween(0, pos.contextStart, a);
        input.extractBetween(pos.contextStart, pos.start, b);
        input.extractBetween(pos.start, pos.limit, c);
        input.extractBetween(pos.limit, pos.contextLimit, d);
        input.extractBetween(pos.contextLimit, input.length(), e);
        appendTo.append(a).append((UChar)123).append(b).
            append((UChar)PIPE).append(c).append((UChar)PIPE).append(d).
            append((UChar)125).append(e);
    } else {
        appendTo.append((UnicodeString)"INVALID UTransPosition {cs=" +
                        pos.contextStart + ", s=" + pos.start + ", l=" +
                        pos.limit + ", cl=" + pos.contextLimit + "} on " +
                        input);
    }
    return appendTo;
}

void TransliteratorTest::expectAux(const UnicodeString& tag,
                                   const UnicodeString& source,
                                   const UnicodeString& result,
                                   const UnicodeString& expectedResult) {
    expectAux(tag, source + " -> " + result,
              result == expectedResult,
              expectedResult);
}

void TransliteratorTest::expectAux(const UnicodeString& tag,
                                   const UnicodeString& summary, UBool pass,
                                   const UnicodeString& expectedResult) {
    if (pass) {
        logln(UnicodeString("(")+tag+") " + prettify(summary));
    } else {
        dataerrln(UnicodeString("FAIL: (")+tag+") "
              + prettify(summary)
              + ", expected " + prettify(expectedResult));
    }
}

#endif 
