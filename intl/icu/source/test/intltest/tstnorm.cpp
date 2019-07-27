





#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/uchar.h"
#include "unicode/errorcode.h"
#include "unicode/normlzr.h"
#include "unicode/uniset.h"
#include "unicode/usetiter.h"
#include "unicode/schriter.h"
#include "unicode/utf16.h"
#include "cstring.h"
#include "normalizer2impl.h"
#include "tstnorm.h"

#define ARRAY_LENGTH(array) UPRV_LENGTHOF(array)

#define CASE(id,test) case id:                          \
                          name = #test;                 \
                          if (exec) {                   \
                              logln(#test "---");       \
                              logln((UnicodeString)""); \
                              test();                   \
                          }                             \
                          break

static UErrorCode status = U_ZERO_ERROR;

void BasicNormalizerTest::runIndexedTest(int32_t index, UBool exec,
                                         const char* &name, char* ) {
    switch (index) {
        CASE(0,TestDecomp);
        CASE(1,TestCompatDecomp);
        CASE(2,TestCanonCompose);
        CASE(3,TestCompatCompose);
        CASE(4,TestPrevious);
        CASE(5,TestHangulDecomp);
        CASE(6,TestHangulCompose);
        CASE(7,TestTibetan);
        CASE(8,TestCompositionExclusion);
        CASE(9,TestZeroIndex);
        CASE(10,TestVerisign);
        CASE(11,TestPreviousNext);
        CASE(12,TestNormalizerAPI);
        CASE(13,TestConcatenate);
        CASE(14,FindFoldFCDExceptions);
        CASE(15,TestCompare);
        CASE(16,TestSkippable);
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
        CASE(17,TestCustomComp);
        CASE(18,TestCustomFCC);
#endif
        CASE(19,TestFilteredNormalizer2Coverage);
        default: name = ""; break;
    }
}




static UnicodeString str(const char *input)
{
    UnicodeString str(input, ""); 
    return str.unescape();
}


BasicNormalizerTest::BasicNormalizerTest()
{
  
  

    canonTests[0][0] = str("cat");  canonTests[0][1] = str("cat"); canonTests[0][2] =  str("cat");

    canonTests[1][0] = str("\\u00e0ardvark");    canonTests[1][1] = str("a\\u0300ardvark");  canonTests[1][2] = str("\\u00e0ardvark"); 

    canonTests[2][0] = str("\\u1e0a"); canonTests[2][1] = str("D\\u0307"); canonTests[2][2] = str("\\u1e0a");                 

    canonTests[3][0] = str("D\\u0307");  canonTests[3][1] = str("D\\u0307"); canonTests[3][2] = str("\\u1e0a");            

    canonTests[4][0] = str("\\u1e0c\\u0307"); canonTests[4][1] = str("D\\u0323\\u0307");  canonTests[4][2] = str("\\u1e0c\\u0307");         

    canonTests[5][0] = str("\\u1e0a\\u0323"); canonTests[5][1] = str("D\\u0323\\u0307");  canonTests[5][2] = str("\\u1e0c\\u0307");        

    canonTests[6][0] = str("D\\u0307\\u0323"); canonTests[6][1] = str("D\\u0323\\u0307");  canonTests[6][2] = str("\\u1e0c\\u0307");         

    canonTests[7][0] = str("\\u1e10\\u0307\\u0323");  canonTests[7][1] = str("D\\u0327\\u0323\\u0307"); canonTests[7][2] = str("\\u1e10\\u0323\\u0307");     

    canonTests[8][0] = str("D\\u0307\\u0328\\u0323"); canonTests[8][1] = str("D\\u0328\\u0323\\u0307"); canonTests[8][2] = str("\\u1e0c\\u0328\\u0307");     

    canonTests[9][0] = str("\\u1E14"); canonTests[9][1] = str("E\\u0304\\u0300"); canonTests[9][2] = str("\\u1E14");         

    canonTests[10][0] = str("\\u0112\\u0300"); canonTests[10][1] = str("E\\u0304\\u0300");  canonTests[10][2] = str("\\u1E14");            

    canonTests[11][0] = str("\\u00c8\\u0304"); canonTests[11][1] = str("E\\u0300\\u0304");  canonTests[11][2] = str("\\u00c8\\u0304");         
  
    canonTests[12][0] = str("\\u212b"); canonTests[12][1] = str("A\\u030a"); canonTests[12][2] = str("\\u00c5");             

    canonTests[13][0] = str("\\u00c5");      canonTests[13][1] = str("A\\u030a");  canonTests[13][2] = str("\\u00c5");            
  
    canonTests[14][0] = str("\\u00C4ffin");  canonTests[14][1] = str("A\\u0308ffin");  canonTests[14][2] = str("\\u00C4ffin");

    canonTests[15][0] = str("\\u00C4\\uFB03n"); canonTests[15][1] = str("A\\u0308\\uFB03n"); canonTests[15][2] = str("\\u00C4\\uFB03n");
  
    canonTests[16][0] = str("Henry IV"); canonTests[16][1] = str("Henry IV"); canonTests[16][2] = str("Henry IV");

    canonTests[17][0] = str("Henry \\u2163");  canonTests[17][1] = str("Henry \\u2163");  canonTests[17][2] = str("Henry \\u2163");
  
    canonTests[18][0] = str("\\u30AC");  canonTests[18][1] = str("\\u30AB\\u3099");  canonTests[18][2] = str("\\u30AC");              

    canonTests[19][0] = str("\\u30AB\\u3099"); canonTests[19][1] = str("\\u30AB\\u3099");  canonTests[19][2] = str("\\u30AC");            

    canonTests[20][0] = str("\\uFF76\\uFF9E"); canonTests[20][1] = str("\\uFF76\\uFF9E");  canonTests[20][2] = str("\\uFF76\\uFF9E");       

    canonTests[21][0] = str("\\u30AB\\uFF9E"); canonTests[21][1] = str("\\u30AB\\uFF9E");  canonTests[21][2] = str("\\u30AB\\uFF9E");         

    canonTests[22][0] = str("\\uFF76\\u3099"); canonTests[22][1] = str("\\uFF76\\u3099");  canonTests[22][2] = str("\\uFF76\\u3099");         

    canonTests[23][0] = str("A\\u0300\\u0316"); canonTests[23][1] = str("A\\u0316\\u0300");  canonTests[23][2] = str("\\u00C0\\u0316");     

    
  
  compatTests[0][0] = str("cat"); compatTests[0][1] = str("cat"); compatTests[0][2] = str("cat") ;
  
  compatTests[1][0] = str("\\uFB4f");  compatTests[1][1] = str("\\u05D0\\u05DC"); compatTests[1][2] = str("\\u05D0\\u05DC");  
  
  compatTests[2][0] = str("\\u00C4ffin"); compatTests[2][1] = str("A\\u0308ffin"); compatTests[2][2] = str("\\u00C4ffin") ;

  compatTests[3][0] = str("\\u00C4\\uFB03n"); compatTests[3][1] = str("A\\u0308ffin"); compatTests[3][2] = str("\\u00C4ffin") ; 
  
  compatTests[4][0] = str("Henry IV"); compatTests[4][1] = str("Henry IV"); compatTests[4][2] = str("Henry IV") ;

  compatTests[5][0] = str("Henry \\u2163"); compatTests[5][1] = str("Henry IV");  compatTests[5][2] = str("Henry IV") ;
  
  compatTests[6][0] = str("\\u30AC"); compatTests[6][1] = str("\\u30AB\\u3099"); compatTests[6][2] = str("\\u30AC") ; 

  compatTests[7][0] = str("\\u30AB\\u3099"); compatTests[7][1] = str("\\u30AB\\u3099"); compatTests[7][2] = str("\\u30AC") ; 
  
  compatTests[8][0] = str("\\uFF76\\u3099"); compatTests[8][1] = str("\\u30AB\\u3099"); compatTests[8][2] = str("\\u30AC") ; 
  
  
  compatTests[9][0] = str("\\uFF76\\uFF9E"); compatTests[9][1] = str("\\u30AB\\u3099"); compatTests[9][2] = str("\\u30AC") ; 

  compatTests[10][0] = str("\\u30AB\\uFF9E"); compatTests[10][1] = str("\\u30AB\\u3099"); compatTests[10][2] = str("\\u30AC") ; 

  
  
  hangulCanon[0][0] = str("\\ud4db"); hangulCanon[0][1] = str("\\u1111\\u1171\\u11b6"); hangulCanon[0][2] = str("\\ud4db") ;

  hangulCanon[1][0] = str("\\u1111\\u1171\\u11b6"), hangulCanon[1][1] = str("\\u1111\\u1171\\u11b6"),   hangulCanon[1][2] = str("\\ud4db");
}

BasicNormalizerTest::~BasicNormalizerTest()
{
}

void BasicNormalizerTest::TestPrevious() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFD);
  
  logln("testing decomp...");
  uint32_t i;
  for (i = 0; i < ARRAY_LENGTH(canonTests); i++) {
    backAndForth(norm, canonTests[i][0]);
  }
  
  logln("testing compose...");
  norm->setMode(UNORM_NFC);
  for (i = 0; i < ARRAY_LENGTH(canonTests); i++) {
    backAndForth(norm, canonTests[i][0]);
  }

  delete norm;
}

void BasicNormalizerTest::TestDecomp() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFD);
  iterateTest(norm, canonTests, ARRAY_LENGTH(canonTests), 1);
  staticTest(UNORM_NFD, 0, canonTests, ARRAY_LENGTH(canonTests), 1);
  delete norm;
}

void BasicNormalizerTest::TestCompatDecomp() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFKD);
  iterateTest(norm, compatTests, ARRAY_LENGTH(compatTests), 1);
  
  staticTest(UNORM_NFKD, 0, 
         compatTests, ARRAY_LENGTH(compatTests), 1);
  delete norm;
}

void BasicNormalizerTest::TestCanonCompose() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFC);
  iterateTest(norm, canonTests, ARRAY_LENGTH(canonTests), 2);
  
  staticTest(UNORM_NFC, 0, canonTests,
         ARRAY_LENGTH(canonTests), 2);
  delete norm;
}

void BasicNormalizerTest::TestCompatCompose() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFKC);
  iterateTest(norm, compatTests, ARRAY_LENGTH(compatTests), 2);
  
  staticTest(UNORM_NFKC, 0, 
         compatTests, ARRAY_LENGTH(compatTests), 2);
  delete norm;
}




void BasicNormalizerTest::TestHangulCompose() 
{
  
  logln("Canonical composition...");
  staticTest(UNORM_NFC, 0,                    hangulCanon,  ARRAY_LENGTH(hangulCanon),  2);
  logln("Compatibility composition...");
  
  
  logln("Static composition...");
  Normalizer* norm = new Normalizer("", UNORM_NFC);
  iterateTest(norm, hangulCanon, ARRAY_LENGTH(hangulCanon), 2);
  norm->setMode(UNORM_NFKC);
  
  
  logln("Reverse iteration...");
  norm->setMode(UNORM_NFC);
  for (uint32_t i = 0; i < ARRAY_LENGTH(hangulCanon); i++) {
    backAndForth(norm, hangulCanon[i][0]);
  }
  delete norm;
}

void BasicNormalizerTest::TestHangulDecomp() 
{
  
  logln("Canonical decomposition...");
  staticTest(UNORM_NFD, 0,                     hangulCanon,  ARRAY_LENGTH(hangulCanon),  1);
  logln("Compatibility decomposition...");
  
  
  logln("Iterative decomposition...");
  Normalizer* norm = new Normalizer("", UNORM_NFD);
  iterateTest(norm, hangulCanon, ARRAY_LENGTH(hangulCanon), 1);
  norm->setMode(UNORM_NFKD);
  
  
  logln("Reverse iteration...");
  norm->setMode(UNORM_NFD);
  for (uint32_t i = 0; i < ARRAY_LENGTH(hangulCanon); i++) {
    backAndForth(norm, hangulCanon[i][0]);
  }
  delete norm;
}




void BasicNormalizerTest::TestTibetan(void) {
    UnicodeString decomp[1][3];
    decomp[0][0] = str("\\u0f77");
    decomp[0][1] = str("\\u0f77");
    decomp[0][2] = str("\\u0fb2\\u0f71\\u0f80");

    UnicodeString compose[1][3];
    compose[0][0] = str("\\u0fb2\\u0f71\\u0f80");
    compose[0][1] = str("\\u0fb2\\u0f71\\u0f80");
    compose[0][2] = str("\\u0fb2\\u0f71\\u0f80");

    staticTest(UNORM_NFD,         0, decomp, ARRAY_LENGTH(decomp), 1);
    staticTest(UNORM_NFKD,  0, decomp, ARRAY_LENGTH(decomp), 2);
    staticTest(UNORM_NFC,        0, compose, ARRAY_LENGTH(compose), 1);
    staticTest(UNORM_NFKC, 0, compose, ARRAY_LENGTH(compose), 2);
}





void BasicNormalizerTest::TestCompositionExclusion(void) {
    
    
    
    
    
    
    
    UnicodeString EXCLUDED = str(
        "\\u0340\\u0341\\u0343\\u0344\\u0374\\u037E\\u0387\\u0958"
        "\\u0959\\u095A\\u095B\\u095C\\u095D\\u095E\\u095F\\u09DC"
        "\\u09DD\\u09DF\\u0A33\\u0A36\\u0A59\\u0A5A\\u0A5B\\u0A5E"
        "\\u0B5C\\u0B5D\\u0F43\\u0F4D\\u0F52\\u0F57\\u0F5C\\u0F69"
        "\\u0F73\\u0F75\\u0F76\\u0F78\\u0F81\\u0F93\\u0F9D\\u0FA2"
        "\\u0FA7\\u0FAC\\u0FB9\\u1F71\\u1F73\\u1F75\\u1F77\\u1F79"
        "\\u1F7B\\u1F7D\\u1FBB\\u1FBE\\u1FC9\\u1FCB\\u1FD3\\u1FDB"
        "\\u1FE3\\u1FEB\\u1FEE\\u1FEF\\u1FF9\\u1FFB\\u1FFD\\u2000"
        "\\u2001\\u2126\\u212A\\u212B\\u2329\\u232A\\uF900\\uFA10"
        "\\uFA12\\uFA15\\uFA20\\uFA22\\uFA25\\uFA26\\uFA2A\\uFB1F"
        "\\uFB2A\\uFB2B\\uFB2C\\uFB2D\\uFB2E\\uFB2F\\uFB30\\uFB31"
        "\\uFB32\\uFB33\\uFB34\\uFB35\\uFB36\\uFB38\\uFB39\\uFB3A"
        "\\uFB3B\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46"
        "\\uFB47\\uFB48\\uFB49\\uFB4A\\uFB4B\\uFB4C\\uFB4D\\uFB4E"
        );
    for (int32_t i=0; i<EXCLUDED.length(); ++i) {
        UnicodeString a(EXCLUDED.charAt(i));
        UnicodeString b;
        UnicodeString c;
        Normalizer::normalize(a, UNORM_NFKD, 0, b, status);
        Normalizer::normalize(b, UNORM_NFC, 0, c, status);
        if (c == a) {
            errln("FAIL: " + hex(a) + " x DECOMP_COMPAT => " +
                  hex(b) + " x COMPOSE => " +
                  hex(c));
        } else if (verbose) {
            logln("Ok: " + hex(a) + " x DECOMP_COMPAT => " +
                  hex(b) + " x COMPOSE => " +
                  hex(c));                
        }
    }
}










void BasicNormalizerTest::TestZeroIndex(void) {
    const char* DATA[] = {
        
        
        "A\\u0316\\u0300", "\\u00C0\\u0316", "A\\u0316\\u0300",
        "A\\u0300\\u0316", "\\u00C0\\u0316", "A\\u0316\\u0300",
        "A\\u0327\\u0300", "\\u00C0\\u0327", "A\\u0327\\u0300",
        "c\\u0321\\u0327", "c\\u0321\\u0327", "c\\u0321\\u0327",
        "c\\u0327\\u0321", "\\u00E7\\u0321", "c\\u0327\\u0321",
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    for (int32_t i=0; i<DATA_length; i+=3) {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString a(DATA[i], "");
        a = a.unescape();
        UnicodeString b;
        Normalizer::normalize(a, UNORM_NFKC, 0, b, status);
        if (U_FAILURE(status)) {
            dataerrln("Error calling normalize UNORM_NFKC: %s", u_errorName(status));
        } else {
            UnicodeString exp(DATA[i+1], "");
            exp = exp.unescape();
            if (b == exp) {
                logln((UnicodeString)"Ok: " + hex(a) + " x COMPOSE_COMPAT => " + hex(b));
            } else {
                errln((UnicodeString)"FAIL: " + hex(a) + " x COMPOSE_COMPAT => " + hex(b) +
                      ", expect " + hex(exp));
            }
        }
        Normalizer::normalize(b, UNORM_NFD, 0, a, status);
        if (U_FAILURE(status)) {
            dataerrln("Error calling normalize UNORM_NFD: %s", u_errorName(status));
        } else {
            UnicodeString exp = UnicodeString(DATA[i+2], "").unescape();
            if (a == exp) {
                logln((UnicodeString)"Ok: " + hex(b) + " x DECOMP => " + hex(a));
            } else {
                errln((UnicodeString)"FAIL: " + hex(b) + " x DECOMP => " + hex(a) +
                      ", expect " + hex(exp));
            }
        }
    }
}




void BasicNormalizerTest::TestVerisign(void) {
    













































































    UnicodeString data[2][3];
    data[0][0] = str("\\u05B8\\u05B9\\u05B1\\u0591\\u05C3\\u05B0\\u05AC\\u059F");
    data[0][1] = str("\\u05B1\\u05B8\\u05B9\\u0591\\u05C3\\u05B0\\u05AC\\u059F");
    data[0][2] = str("");
    data[1][0] = str("\\u0592\\u05B7\\u05BC\\u05A5\\u05B0\\u05C0\\u05C4\\u05AD");
    data[1][1] = str("\\u05B0\\u05B7\\u05BC\\u05A5\\u0592\\u05C0\\u05AD\\u05C4");
    data[1][2] = str("");

    staticTest(UNORM_NFD, 0, data, ARRAY_LENGTH(data), 1);
    staticTest(UNORM_NFC, 0, data, ARRAY_LENGTH(data), 1);
}





UnicodeString BasicNormalizerTest::hex(UChar ch) {
    UnicodeString result;
    return appendHex(ch, 4, result);
}

UnicodeString BasicNormalizerTest::hex(const UnicodeString& s) {
    UnicodeString result;
    for (int i = 0; i < s.length(); ++i) {
        if (i != 0) result += (UChar)0x2c;
        appendHex(s[i], 4, result);
    }
    return result;
}


inline static void insert(UnicodeString& dest, int pos, UChar32 ch)
{
    dest.replace(pos, 0, ch);
}

void BasicNormalizerTest::backAndForth(Normalizer* iter, const UnicodeString& input)
{
    UChar32 ch;
    iter->setText(input, status);

    
    UnicodeString forward;
    for (ch = iter->first(); ch != iter->DONE; ch = iter->next()) {
        forward += ch;
    }

    
    UnicodeString reverse;
    for (ch = iter->last(); ch != iter->DONE; ch = iter->previous()) {
        insert(reverse, 0, ch);
    }
    
    if (forward != reverse) {
        errln("Forward/reverse mismatch for input " + hex(input)
              + ", forward: " + hex(forward) + ", backward: " + hex(reverse));
    }
}

void BasicNormalizerTest::staticTest(UNormalizationMode mode, int options,
                     UnicodeString tests[][3], int length,
                     int outCol)
{
    for (int i = 0; i < length; i++)
    {
        UnicodeString& input = tests[i][0];
        UnicodeString& expect = tests[i][outCol];
        
        logln("Normalizing '" + input + "' (" + hex(input) + ")" );
        
        UnicodeString output;
        Normalizer::normalize(input, mode, options, output, status);
        
        if (output != expect) {
            dataerrln(UnicodeString("ERROR: case ") + i + " normalized " + hex(input) + "\n"
                + "                expected " + hex(expect) + "\n"
                + "              static got " + hex(output) );
        }
    }
}

void BasicNormalizerTest::iterateTest(Normalizer* iter,
                                      UnicodeString tests[][3], int length,
                                      int outCol)
{
    for (int i = 0; i < length; i++)
    {
        UnicodeString& input = tests[i][0];
        UnicodeString& expect = tests[i][outCol];
        
        logln("Normalizing '" + input + "' (" + hex(input) + ")" );
        
        iter->setText(input, status);
        assertEqual(input, expect, iter, UnicodeString("ERROR: case ") + i + " ");
    }
}

void BasicNormalizerTest::assertEqual(const UnicodeString&    input,
                      const UnicodeString&    expected,
                      Normalizer*        iter,
                      const UnicodeString&    errPrefix)
{
    UnicodeString result;

    for (UChar32 ch = iter->first(); ch != iter->DONE; ch = iter->next()) {
        result += ch;
    }
    if (result != expected) {
        dataerrln(errPrefix + "normalized " + hex(input) + "\n"
            + "                expected " + hex(expected) + "\n"
            + "             iterate got " + hex(result) );
    }
}



class UChar32Iterator {
public:
    UChar32Iterator(const UChar32 *text, int32_t len, int32_t index) :
        s(text), length(len), i(index) {}

    UChar32 current() {
        if(i<length) {
            return s[i];
        } else {
            return 0xffff;
        }
    }

    UChar32 next() {
        if(i<length) {
            return s[i++];
        } else {
            return 0xffff;
        }
    }

    UChar32 previous() {
        if(i>0) {
            return s[--i];
        } else {
            return 0xffff;
        }
    }

    int32_t getIndex() {
        return i;
    }
private:
    const UChar32 *s;
    int32_t length, i;
};

void
BasicNormalizerTest::TestPreviousNext(const UChar *src, int32_t srcLength,
                                      const UChar32 *expect, int32_t expectLength,
                                      const int32_t *expectIndex, 
                                      int32_t srcMiddle, int32_t expectMiddle,
                                      const char *moves,
                                      UNormalizationMode mode,
                                      const char *name) {
    
    Normalizer iter(src, srcLength, mode);

    
    if(iter.getDynamicClassID() != Normalizer::getStaticClassID()) {
        errln("getStaticClassID != getDynamicClassID for Normalizer.");
    }

    UChar32Iterator iter32(expect, expectLength, expectMiddle);

    UChar32 c1, c2;
    char m;

    
    iter.setIndexOnly(srcMiddle);

    
    
    const char *move=moves;
    while((m=*move++)!=0) {
        if(m=='-') {
            c1=iter.previous();
            c2=iter32.previous();
        } else if(m=='0') {
            c1=iter.current();
            c2=iter32.current();
        } else  {
            c1=iter.next();
            c2=iter32.next();
        }

        
        if(c1!=c2) {
            
            char history[64];
            uprv_strcpy(history, moves);
            history[move-moves]=0;
            dataerrln("error: mismatch in Normalizer iteration (%s) at %s: "
                  "got c1=U+%04lx != expected c2=U+%04lx",
                  name, history, c1, c2);
            break;
        }

        
        if(iter.getIndex()!=expectIndex[iter32.getIndex()]) {
            
            char history[64];
            uprv_strcpy(history, moves);
            history[move-moves]=0;
            errln("error: index mismatch in Normalizer iteration (%s) at %s: "
                  "Normalizer index %ld expected %ld\n",
                  name, history, iter.getIndex(), expectIndex[iter32.getIndex()]);
            break;
        }
    }
}

void
BasicNormalizerTest::TestPreviousNext() {
    
    static const UChar src[]={
        U16_LEAD(0x2f999), U16_TRAIL(0x2f999),
        U16_LEAD(0x1d15f), U16_TRAIL(0x1d15f),
        0xc4,
        0x1ed0
    };
    static const UChar32 expect[]={
        0x831d,
        0x1d158, 0x1d165,
        0x41, 0x308,
        0x4f, 0x302, 0x301
    };

    
    static const int32_t expectIndex[]={
        0,
        2, 2,
        4, 4,
        5, 5, 5,
        6 
    };

    
    static const UChar src_j2911[]={
        U16_LEAD(0x2f999), U16_TRAIL(0x2f999),
        0xdd00, 0xd900, 
        0xc4,
        0x4f, 0x302, 0x301
    };
    static const UChar32 expect_j2911[]={
        0x831d,
        0xdd00, 0xd900, 
        0xc4,
        0x1ed0
    };

    
    static const int32_t expectIndex_j2911[]={
        0,
        2, 3,
        4,
        5,
        8 
    };

    
    
    enum {
        SRC_MIDDLE=4,
        EXPECT_MIDDLE=3,
        SRC_MIDDLE_2=2,
        EXPECT_MIDDLE_2=1
    };

    
    
    
    static const char *const moves="0+0+0--0-0-+++0--+++++++0--------";

    TestPreviousNext(src, UPRV_LENGTHOF(src),
                     expect, UPRV_LENGTHOF(expect),
                     expectIndex,
                     SRC_MIDDLE, EXPECT_MIDDLE,
                     moves, UNORM_NFD, "basic");

    TestPreviousNext(src_j2911, UPRV_LENGTHOF(src_j2911),
                     expect_j2911, UPRV_LENGTHOF(expect_j2911),
                     expectIndex_j2911,
                     SRC_MIDDLE, EXPECT_MIDDLE,
                     moves, UNORM_NFKC, "j2911");

    
    TestPreviousNext(src, UPRV_LENGTHOF(src),
                     expect, UPRV_LENGTHOF(expect),
                     expectIndex,
                     SRC_MIDDLE_2, EXPECT_MIDDLE_2,
                     moves, UNORM_NFD, "basic_2");

    TestPreviousNext(src_j2911, UPRV_LENGTHOF(src_j2911),
                     expect_j2911, UPRV_LENGTHOF(expect_j2911),
                     expectIndex_j2911,
                     SRC_MIDDLE_2, EXPECT_MIDDLE_2,
                     moves, UNORM_NFKC, "j2911_2");
}

void BasicNormalizerTest::TestConcatenate() {
    static const char *const
    cases[][4]={
        
        {
            "C",
            "re",
            "\\u0301sum\\u00e9",
            "r\\u00e9sum\\u00e9"
        },
        {
            "C",
            "a\\u1100",
            "\\u1161bcdefghijk",
            "a\\uac00bcdefghijk"
        },
        
        {
            "D",
            "\\u03B1\\u0345",
            "\\u0C4D\\U000110BA\\U0001D169",
            "\\u03B1\\U0001D169\\U000110BA\\u0C4D\\u0345"
        }
    };

    UnicodeString left, right, expect, result, r;
    UErrorCode errorCode;
    UNormalizationMode mode;
    int32_t i;

    
    for(i=0; i<(int32_t)(sizeof(cases)/sizeof(cases[0])); ++i) {
        switch(*cases[i][0]) {
        case 'C': mode=UNORM_NFC; break;
        case 'D': mode=UNORM_NFD; break;
        case 'c': mode=UNORM_NFKC; break;
        case 'd': mode=UNORM_NFKD; break;
        default: mode=UNORM_NONE; break;
        }

        left=UnicodeString(cases[i][1], "").unescape();
        right=UnicodeString(cases[i][2], "").unescape();
        expect=UnicodeString(cases[i][3], "").unescape();

        
        errorCode=U_ZERO_ERROR;

        r=Normalizer::concatenate(left, right, result, mode, 0, errorCode);
        if(U_FAILURE(errorCode) ||  result!=expect) {
            dataerrln("error in Normalizer::concatenate(), cases[] fails with "+
                UnicodeString(u_errorName(errorCode))+", result==expect: expected: "+
                hex(expect)+" =========> got: " + hex(result));
        }
    }

    

    
    result=r=expect=UnicodeString("zz", "");
    errorCode=U_UNEXPECTED_TOKEN;
    r=Normalizer::concatenate(left, right, result, mode, 0, errorCode);
    if(errorCode!=U_UNEXPECTED_TOKEN || result!=r || !result.isBogus()) {
        errln("error in Normalizer::concatenate(), violates UErrorCode protocol");
    }

    left.setToBogus();
    errorCode=U_ZERO_ERROR;
    r=Normalizer::concatenate(left, right, result, mode, 0, errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || result!=r || !result.isBogus()) {
        errln("error in Normalizer::concatenate(), does not detect left.isBogus()");
    }
}


static int32_t
ref_norm_compare(const UnicodeString &s1, const UnicodeString &s2, uint32_t options, UErrorCode &errorCode) {
    UnicodeString r1, r2, t1, t2;
    int32_t normOptions=(int32_t)(options>>UNORM_COMPARE_NORM_OPTIONS_SHIFT);

    if(options&U_COMPARE_IGNORE_CASE) {
        Normalizer::decompose(s1, FALSE, normOptions, r1, errorCode);
        Normalizer::decompose(s2, FALSE, normOptions, r2, errorCode);

        r1.foldCase(options);
        r2.foldCase(options);
    } else {
        r1=s1;
        r2=s2;
    }

    Normalizer::decompose(r1, FALSE, normOptions, t1, errorCode);
    Normalizer::decompose(r2, FALSE, normOptions, t2, errorCode);

    if(options&U_COMPARE_CODE_POINT_ORDER) {
        return t1.compareCodePointOrder(t2);
    } else {
        return t1.compare(t2);
    }
}


static int32_t
_norm_compare(const UnicodeString &s1, const UnicodeString &s2, uint32_t options, UErrorCode &errorCode) {
    int32_t normOptions=(int32_t)(options>>UNORM_COMPARE_NORM_OPTIONS_SHIFT);

    if( UNORM_YES==Normalizer::quickCheck(s1, UNORM_FCD, normOptions, errorCode) &&
        UNORM_YES==Normalizer::quickCheck(s2, UNORM_FCD, normOptions, errorCode)) {
        options|=UNORM_INPUT_IS_FCD;
    }

    return Normalizer::compare(s1, s2, options, errorCode);
}


static int32_t
ref_case_compare(const UnicodeString &s1, const UnicodeString &s2, uint32_t options) {
    UnicodeString t1, t2;

    t1=s1;
    t2=s2;

    t1.foldCase(options);
    t2.foldCase(options);

    if(options&U_COMPARE_CODE_POINT_ORDER) {
        return t1.compareCodePointOrder(t2);
    } else {
        return t1.compare(t2);
    }
}


static inline int32_t
_sign(int32_t value) {
    if(value==0) {
        return 0;
    } else {
        return (value>>31)|1;
    }
}

static const char *
_signString(int32_t value) {
    if(value<0) {
        return "<0";
    } else if(value==0) {
        return "=0";
    } else  {
        return ">0";
    }
}

void
BasicNormalizerTest::TestCompare() {
    
    
    

    
    
    static const char *const
    strings[]={
        
        
        "D\\u031B\\u0307\\u0323",
        "\\u1E0C\\u031B\\u0307",
        "D\\u031B\\u0323\\u0307",
        "d\\u031B\\u0323\\u0307",

        
        "\\u00E4",
        "a\\u0308",
        "A\\u0308",

        
        
        "\\u212B",
        "\\u00C5",
        "A\\u030A",
        "a\\u030A",

        
        "a\\u059A\\u0316\\u302A\\u032Fb",
        "a\\u302A\\u0316\\u032F\\u059Ab",
        "a\\u302A\\u0316\\u032F\\u059Ab",
        "A\\u059A\\u0316\\u302A\\u032Fb",

        
        
        "A\\u00df\\u00b5\\ufb03\\U0001040c\\u0131",
        "ass\\u03bcffi\\U00010434i",
        "\\u0061\\u0042\\u0131\\u03a3\\u00df\\ufb03\\ud93f\\udfff",
        "\\u0041\\u0062\\u0069\\u03c3\\u0073\\u0053\\u0046\\u0066\\u0049\\ud93f\\udfff",
        "\\u0041\\u0062\\u0131\\u03c3\\u0053\\u0073\\u0066\\u0046\\u0069\\ud93f\\udfff",
        "\\u0041\\u0062\\u0069\\u03c3\\u0073\\u0053\\u0046\\u0066\\u0049\\ud93f\\udffd",

        
        
        
        "\\ud800\\ud800\\udc01",
        "\\ud800\\udc00",

        
        
        "\\u20ac\\ud801",
        "\\u20ac\\ud800\\udc00",
        "\\ud800",
        "\\ud800\\uff61",
        "\\udfff",
        "\\uff61\\udfff",
        "\\uff61\\ud800\\udc02",
        "\\ud800\\udc02",
        "\\ud84d\\udc56",

        
        
        
        "\\uAD8B\\uAD8B\\uAD8B\\uAD8B"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "aaaaaaaaaaaaaaaaaazzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
        "\\uAD8B\\uAD8B\\uAD8B\\uAD8B"
        "d\\u031B\\u0307\\u0323",

        "\\u1100\\u116f\\u11aa\\uAD8B\\uAD8B\\u1100\\u116f\\u11aa"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "aaaaaaaaaaAAAAAAAAZZZZZZZZZZZZZZZZzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
        "\\u1100\\u116f\\u11aa\\uAD8B\\uAD8B\\u1100\\u116f\\u11aa"
        "\\u1E0C\\u031B\\u0307",

        
        
        
        "\\u0360\\u0345\\u0334",
        "\\u0360\\u03b9\\u0334",

        "\\u0360\\u1f80\\u0334",
        "\\u0360\\u03b1\\u0313\\u03b9\\u0334",

        "\\u0360\\u1ffc\\u0334",
        "\\u0360\\u03c9\\u03b9\\u0334",

        "a\\u0360\\u0345\\u0360\\u0345b",
        "a\\u0345\\u0360\\u0345\\u0360b",

        
        
        "\\u00cc",
        "\\u0069\\u0300",

        
        
        "\\u00e4\\u193b\\U0002f868",
        "\\u0061\\u193b\\u0308\\u36fc",

        
        
        ""
    };

    UnicodeString s[100]; 

    
    
    
    static const struct {
        uint32_t options;
        const char *name;
    } opt[]={
        { 0, "default" },
        { U_COMPARE_CODE_POINT_ORDER, "c.p. order" },
        { U_COMPARE_IGNORE_CASE, "ignore case" },
        { U_COMPARE_CODE_POINT_ORDER|U_COMPARE_IGNORE_CASE, "c.p. order & ignore case" },
        { U_COMPARE_IGNORE_CASE|U_FOLD_CASE_EXCLUDE_SPECIAL_I, "ignore case & special i" },
        { U_COMPARE_CODE_POINT_ORDER|U_COMPARE_IGNORE_CASE|U_FOLD_CASE_EXCLUDE_SPECIAL_I, "c.p. order & ignore case & special i" },
        { UNORM_UNICODE_3_2<<UNORM_COMPARE_NORM_OPTIONS_SHIFT, "Unicode 3.2" }
    };

    int32_t i, j, k, count=UPRV_LENGTHOF(strings);
    int32_t result, refResult;

    UErrorCode errorCode;

    
    for(i=0; i<count; ++i) {
        s[i]=UnicodeString(strings[i], "").unescape();
    }

    
    for(i=0; i<count; ++i) {
        for(j=i; j<count; ++j) {
            for(k=0; k<UPRV_LENGTHOF(opt); ++k) {
                
                errorCode=U_ZERO_ERROR;
                result=_norm_compare(s[i], s[j], opt[k].options, errorCode);
                refResult=ref_norm_compare(s[i], s[j], opt[k].options, errorCode);
                if(_sign(result)!=_sign(refResult)) {
                    errln("Normalizer::compare(%d, %d, %s)%s should be %s %s",
                        i, j, opt[k].name, _signString(result), _signString(refResult),
                        U_SUCCESS(errorCode) ? "" : u_errorName(errorCode));
                }

                
                if(opt[k].options&U_COMPARE_IGNORE_CASE) {
                    errorCode=U_ZERO_ERROR;
                    result=s[i].caseCompare(s[j], opt[k].options);
                    refResult=ref_case_compare(s[i], s[j], opt[k].options);
                    if(_sign(result)!=_sign(refResult)) {
                        errln("UniStr::caseCompare(%d, %d, %s)%s should be %s %s",
                            i, j, opt[k].name, _signString(result), _signString(refResult),
                            U_SUCCESS(errorCode) ? "" : u_errorName(errorCode));
                    }
                }
            }
        }
    }

    
    static const UChar iI[]={ 0x49, 0x69, 0x130, 0x131 };
    UnicodeSet iSet, set;

    UnicodeString s1, s2;

    const Normalizer2Impl *nfcImpl=Normalizer2Factory::getNFCImpl(errorCode);
    if(U_FAILURE(errorCode) || !nfcImpl->ensureCanonIterData(errorCode)) {
        dataerrln("Normalizer2Factory::getNFCImpl().ensureCanonIterData() failed: %s",
              u_errorName(errorCode));
        return;
    }

    
    for(i=0; i<UPRV_LENGTHOF(iI); ++i) {
        if(nfcImpl->getCanonStartSet(iI[i], iSet)) {
            set.addAll(iSet);
        }
    }

    
    const Normalizer2 *nfcNorm2=Normalizer2::getNFCInstance(errorCode);
    UnicodeSetIterator it(set);
    while(it.next() && !it.isString()) {
        UChar32 c=it.getCodepoint();
        if(!nfcNorm2->getDecomposition(c, s2)) {
            dataerrln("NFC.getDecomposition(i-composite U+%04lx) failed", (long)c);
            return;
        }

        s1.setTo(c);
        for(k=0; k<UPRV_LENGTHOF(opt); ++k) {
            
            errorCode=U_ZERO_ERROR;
            result=_norm_compare(s1, s2, opt[k].options, errorCode);
            refResult=ref_norm_compare(s1, s2, opt[k].options, errorCode);
            if(_sign(result)!=_sign(refResult)) {
                errln("Normalizer::compare(U+%04x with its NFD, %s)%s should be %s %s",
                    c, opt[k].name, _signString(result), _signString(refResult),
                    U_SUCCESS(errorCode) ? "" : u_errorName(errorCode));
            }

            
            if(opt[k].options&U_COMPARE_IGNORE_CASE) {
                errorCode=U_ZERO_ERROR;
                result=s1.caseCompare(s2, opt[k].options);
                refResult=ref_case_compare(s1, s2, opt[k].options);
                if(_sign(result)!=_sign(refResult)) {
                    errln("UniStr::caseCompare(U+%04x with its NFD, %s)%s should be %s %s",
                        c, opt[k].name, _signString(result), _signString(refResult),
                        U_SUCCESS(errorCode) ? "" : u_errorName(errorCode));
                }
            }
        }
    }

    
    if( nfcNorm2->getDecomposition(0x20, s2) ||
        nfcNorm2->getDecomposition(0x4e00, s2) ||
        nfcNorm2->getDecomposition(0x20002, s2)
    ) {
        errln("NFC.getDecomposition() returns TRUE for characters which do not have decompositions");
    }

    
    if( nfcNorm2->getRawDecomposition(0x20, s2) ||
        nfcNorm2->getRawDecomposition(0x4e00, s2) ||
        nfcNorm2->getRawDecomposition(0x20002, s2)
    ) {
        errln("NFC.getRawDecomposition() returns TRUE for characters which do not have decompositions");
    }

    
    if( nfcNorm2->composePair(0x20, 0x301)>=0 ||
        nfcNorm2->composePair(0x61, 0x305)>=0 ||
        nfcNorm2->composePair(0x1100, 0x1160)>=0 ||
        nfcNorm2->composePair(0xac00, 0x11a7)>=0
    ) {
        errln("NFC.composePair() incorrectly composes some pairs of characters");
    }

    
    UnicodeSet filter(UNICODE_STRING_SIMPLE("[^\\u00a0-\\u00ff]"), errorCode);
    FilteredNormalizer2 fn2(*nfcNorm2, filter);
    if( fn2.getDecomposition(0xe4, s1) || !fn2.getDecomposition(0x100, s2) ||
        s2.length()!=2 || s2[0]!=0x41 || s2[1]!=0x304
    ) {
        errln("FilteredNormalizer2(NFC, ^A0-FF).getDecomposition() failed");
    }

    
    if( fn2.getRawDecomposition(0xe4, s1) || !fn2.getRawDecomposition(0x100, s2) ||
        s2.length()!=2 || s2[0]!=0x41 || s2[1]!=0x304
    ) {
        errln("FilteredNormalizer2(NFC, ^A0-FF).getRawDecomposition() failed");
    }

    
    if( 0x100!=fn2.composePair(0x41, 0x304) ||
        fn2.composePair(0xc7, 0x301)>=0 
    ) {
        errln("FilteredNormalizer2(NFC, ^A0-FF).composePair() failed");
    }
}


int32_t
BasicNormalizerTest::countFoldFCDExceptions(uint32_t foldingOptions) {
    UnicodeString s, fold, d;
    UChar32 c;
    int32_t count;
    uint8_t cc, trailCC, foldCC, foldTrailCC;
    UNormalizationCheckResult qcResult;
    int8_t category;
    UBool isNFD;
    UErrorCode errorCode;

    logln("Test if case folding may un-FCD a string (folding options %04lx)", foldingOptions);

    count=0;
    for(c=0; c<=0x10ffff; ++c) {
        errorCode = U_ZERO_ERROR;
        category=u_charType(c);
        if(category==U_UNASSIGNED) {
            continue; 
        }
        if(c==0xac00) {
            c=0xd7a3; 
            continue;
        }
        
        if(c==0x3400) {
            c=0x4db5;
            continue;
        }
        if(c==0x4e00) {
            c=0x9fa5;
            continue;
        }
        if(c==0x20000) {
            c=0x2a6d6;
            continue;
        }

        s.setTo(c);

        
        Normalizer::decompose(s, FALSE, 0, d, errorCode);
        isNFD= s==d;
        cc=u_getCombiningClass(d.char32At(0));
        trailCC=u_getCombiningClass(d.char32At(d.length()-1));

        
        s.foldCase(foldingOptions);
        Normalizer::decompose(s, FALSE, 0, d, errorCode);
        foldCC=u_getCombiningClass(d.char32At(0));
        foldTrailCC=u_getCombiningClass(d.char32At(d.length()-1));

        qcResult=Normalizer::quickCheck(s, UNORM_FCD, errorCode);

        if (U_FAILURE(errorCode)) {
            ++count;
            dataerrln("U+%04lx: Failed with error %s", u_errorName(errorCode));
        }

        
        
        
        
        if( qcResult!=UNORM_YES ||
            s.isEmpty() ||
            (cc!=foldCC && foldCC!=0) || (trailCC!=foldTrailCC && foldTrailCC!=0)
        ) {
            ++count;
            dataerrln("U+%04lx: case-folding may un-FCD a string (folding options %04lx)", c, foldingOptions);
            dataerrln("  cc %02x trailCC %02x    foldCC(U+%04lx) %02x foldTrailCC(U+%04lx) %02x   quickCheck(folded)=%d", cc, trailCC, d.char32At(0), foldCC, d.char32At(d.length()-1), foldTrailCC, qcResult);
            continue;
        }

        
        
        
        if(isNFD && UNORM_YES!=Normalizer::quickCheck(s, UNORM_NFD, errorCode)) {
            ++count;
            errln("U+%04lx: case-folding un-NFDs this character (folding options %04lx)", c, foldingOptions);
        }
    }

    logln("There are %ld code points for which case-folding may un-FCD a string (folding options %04lx)", count, foldingOptions);
    return count;
}

void
BasicNormalizerTest::FindFoldFCDExceptions() {
    int32_t count;

    count=countFoldFCDExceptions(0);
    count+=countFoldFCDExceptions(U_FOLD_CASE_EXCLUDE_SPECIAL_I);
    if(count>0) {
        





        dataerrln("error: There are %ld code points for which case-folding may un-FCD a string for all folding options.\n"
              "See comment in BasicNormalizerTest::FindFoldFCDExceptions()!", count);
    }
}

static void
initExpectedSkippables(UnicodeSet skipSets[UNORM_MODE_COUNT], UErrorCode &errorCode) {
    skipSets[UNORM_NFD].applyPattern(
        UNICODE_STRING_SIMPLE("[[:NFD_QC=Yes:]&[:ccc=0:]]"), errorCode);
    skipSets[UNORM_NFC].applyPattern(
        UNICODE_STRING_SIMPLE("[[:NFC_QC=Yes:]&[:ccc=0:]-[:HST=LV:]]"), errorCode);
    skipSets[UNORM_NFKD].applyPattern(
        UNICODE_STRING_SIMPLE("[[:NFKD_QC=Yes:]&[:ccc=0:]]"), errorCode);
    skipSets[UNORM_NFKC].applyPattern(
        UNICODE_STRING_SIMPLE("[[:NFKC_QC=Yes:]&[:ccc=0:]-[:HST=LV:]]"), errorCode);

    
    
    
    UnicodeSet combineBack("[:NFC_QC=Maybe:]", errorCode);
    int32_t numCombineBack=combineBack.size();
    int32_t *combineBackCharsAndCc=new int32_t[numCombineBack*2];
    UnicodeSetIterator iter(combineBack);
    for(int32_t i=0; i<numCombineBack; ++i) {
        iter.next();
        UChar32 c=iter.getCodepoint();
        combineBackCharsAndCc[2*i]=c;
        combineBackCharsAndCc[2*i+1]=u_getCombiningClass(c);
    }

    
    
    UnicodeSet notInteresting("[[:C:][:Unified_Ideograph:][:HST=LVT:]]", errorCode);
    LocalPointer<UnicodeSet> unsure(&((UnicodeSet *)(skipSets[UNORM_NFC].clone()))->removeAll(notInteresting));
    

    
    
    const Normalizer2 *norm2=Normalizer2::getNFCInstance(errorCode);
    UnicodeString s;
    iter.reset(*unsure);
    while(iter.next()) {
        UChar32 c=iter.getCodepoint();
        s.setTo(c);
        int32_t cLength=s.length();
        int32_t tccc=u_getIntPropertyValue(c, UCHAR_TRAIL_CANONICAL_COMBINING_CLASS);
        for(int32_t i=0; i<numCombineBack; ++i) {
            
            
            int32_t cc2=combineBackCharsAndCc[2*i+1];
            if(tccc==0 || cc2!=0) {
                UChar32 c2=combineBackCharsAndCc[2*i];
                s.append(c2);
                if(!norm2->isNormalized(s, errorCode)) {
                    
                    skipSets[UNORM_NFC].remove(c);
                    skipSets[UNORM_NFKC].remove(c);
                    break;
                }
                s.truncate(cLength);
            }
        }
    }
    delete [] combineBackCharsAndCc;
}

void
BasicNormalizerTest::TestSkippable() {
    UnicodeSet diff, skipSets[UNORM_MODE_COUNT], expectSets[UNORM_MODE_COUNT];
    UnicodeString s, pattern;

    
    IcuTestErrorCode errorCode(*this, "TestSkippable");
    skipSets[UNORM_NFD].applyPattern(UNICODE_STRING_SIMPLE("[:NFD_Inert:]"), errorCode);
    skipSets[UNORM_NFKD].applyPattern(UNICODE_STRING_SIMPLE("[:NFKD_Inert:]"), errorCode);
    skipSets[UNORM_NFC].applyPattern(UNICODE_STRING_SIMPLE("[:NFC_Inert:]"), errorCode);
    skipSets[UNORM_NFKC].applyPattern(UNICODE_STRING_SIMPLE("[:NFKC_Inert:]"), errorCode);
    if(errorCode.logDataIfFailureAndReset("UnicodeSet(NF..._Inert) failed")) {
        return;
    }

    
    initExpectedSkippables(expectSets, errorCode);
    errorCode.assertSuccess();

    for(int32_t i=UNORM_NONE; i<UNORM_MODE_COUNT; ++i) {
        if(skipSets[i]!=expectSets[i]) {
            errln("error: TestSkippable skipSets[%d]!=expectedSets[%d]\n", i, i);
            
            
            
            
            

            s=UNICODE_STRING_SIMPLE("skip-expect=");
            (diff=skipSets[i]).removeAll(expectSets[i]).toPattern(pattern, TRUE);
            s.append(pattern);

            pattern.remove();
            s.append(UNICODE_STRING_SIMPLE("\n\nexpect-skip="));
            (diff=expectSets[i]).removeAll(skipSets[i]).toPattern(pattern, TRUE);
            s.append(pattern);
            s.append(UNICODE_STRING_SIMPLE("\n\n"));

            errln(s);
        }
    }
}

struct StringPair { const char *input, *expected; };

void
BasicNormalizerTest::TestCustomComp() {
    static const StringPair pairs[]={
        { "\\uD801\\uE000\\uDFFE", "" },
        { "\\uD800\\uD801\\uE000\\uDFFE\\uDFFF", "\\uD7FF\\uFFFF" },
        { "\\uD800\\uD801\\uDFFE\\uDFFF", "\\uD7FF\\U000107FE\\uFFFF" },
        { "\\uE001\\U000110B9\\u0345\\u0308\\u0327", "\\uE002\\U000110B9\\u0327\\u0345" },
        { "\\uE010\\U000F0011\\uE012", "\\uE011\\uE012" },
        { "\\uE010\\U000F0011\\U000F0011\\uE012", "\\uE011\\U000F0010" },
        { "\\uE111\\u1161\\uE112\\u1162", "\\uAE4C\\u1102\\u0062\\u1162" },
        { "\\uFFF3\\uFFF7\\U00010036\\U00010077", "\\U00010037\\U00010037\\uFFF6\\U00010037" }
    };
    IcuTestErrorCode errorCode(*this, "BasicNormalizerTest/TestCustomComp");
    const Normalizer2 *customNorm2=
        Normalizer2::getInstance(loadTestData(errorCode), "testnorm",
                                 UNORM2_COMPOSE, errorCode);
    if(errorCode.logDataIfFailureAndReset("unable to load testdata/testnorm.nrm")) {
        return;
    }
    for(int32_t i=0; i<UPRV_LENGTHOF(pairs); ++i) {
        const StringPair &pair=pairs[i];
        UnicodeString input=UnicodeString(pair.input, -1, US_INV).unescape();
        UnicodeString expected=UnicodeString(pair.expected, -1, US_INV).unescape();
        UnicodeString result=customNorm2->normalize(input, errorCode);
        if(result!=expected) {
            errln("custom compose Normalizer2 did not normalize input %d as expected", i);
        }
    }
}

void
BasicNormalizerTest::TestCustomFCC() {
    static const StringPair pairs[]={
        { "\\uD801\\uE000\\uDFFE", "" },
        { "\\uD800\\uD801\\uE000\\uDFFE\\uDFFF", "\\uD7FF\\uFFFF" },
        { "\\uD800\\uD801\\uDFFE\\uDFFF", "\\uD7FF\\U000107FE\\uFFFF" },
        
        
        { "\\uE001\\U000110B9\\u0345\\u0308\\u0327", "\\uE001\\U000110B9\\u0327\\u0308\\u0345" },
        { "\\uE010\\U000F0011\\uE012", "\\uE011\\uE012" },
        { "\\uE010\\U000F0011\\U000F0011\\uE012", "\\uE011\\U000F0010" },
        { "\\uE111\\u1161\\uE112\\u1162", "\\uAE4C\\u1102\\u0062\\u1162" },
        { "\\uFFF3\\uFFF7\\U00010036\\U00010077", "\\U00010037\\U00010037\\uFFF6\\U00010037" }
    };
    IcuTestErrorCode errorCode(*this, "BasicNormalizerTest/TestCustomFCC");
    const Normalizer2 *customNorm2=
        Normalizer2::getInstance(loadTestData(errorCode), "testnorm",
                                 UNORM2_COMPOSE_CONTIGUOUS, errorCode);
    if(errorCode.logDataIfFailureAndReset("unable to load testdata/testnorm.nrm")) {
        return;
    }
    for(int32_t i=0; i<UPRV_LENGTHOF(pairs); ++i) {
        const StringPair &pair=pairs[i];
        UnicodeString input=UnicodeString(pair.input, -1, US_INV).unescape();
        UnicodeString expected=UnicodeString(pair.expected, -1, US_INV).unescape();
        UnicodeString result=customNorm2->normalize(input, errorCode);
        if(result!=expected) {
            errln("custom FCC Normalizer2 did not normalize input %d as expected", i);
        }
    }
}


void
BasicNormalizerTest::TestFilteredNormalizer2Coverage() {
    UErrorCode errorCode = U_ZERO_ERROR;
    const Normalizer2 *nfcNorm2=Normalizer2::getNFCInstance(errorCode);
    if (U_FAILURE(errorCode)) {
        dataerrln("Normalizer2::getNFCInstance() call failed - %s", u_errorName(status));
        return;
    }
    UnicodeSet filter(UNICODE_STRING_SIMPLE("[^\\u00a0-\\u00ff\\u0310-\\u031f]"), errorCode);
    FilteredNormalizer2 fn2(*nfcNorm2, filter);

    UChar32 char32 = 0x0054;

    if (fn2.isInert(char32)) {
        errln("FilteredNormalizer2.isInert() failed.");
    }

    if (fn2.hasBoundaryAfter(char32)) {
        errln("FilteredNormalizer2.hasBoundaryAfter() failed.");
    }

    UChar32 c;
    for(c=0; c<=0x3ff; ++c) {
        uint8_t expectedCC= filter.contains(c) ? nfcNorm2->getCombiningClass(c) : 0;
        uint8_t cc=fn2.getCombiningClass(c);
        if(cc!=expectedCC) {
            errln(
                UnicodeString("FilteredNormalizer2(NFC, ^A0-FF,310-31F).getCombiningClass(U+")+
                hex(c)+
                ")==filtered NFC.getCC()");
        }
    }

    UnicodeString newString1 = UNICODE_STRING_SIMPLE("[^\\u0100-\\u01ff]");
    UnicodeString newString2 = UNICODE_STRING_SIMPLE("[^\\u0200-\\u02ff]");
    fn2.append(newString1, newString2, errorCode);
    if (U_FAILURE(errorCode)) {
        errln("FilteredNormalizer2.append() failed.");
    }
}

#endif 
