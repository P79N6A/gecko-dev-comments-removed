




#include <string.h>
#include "unicode/utypes.h"
#include "unicode/uscript.h"
#include "unicode/uchar.h"
#include "cintltst.h"
#include "cucdapi.h"
#include "cmemory.h"

static void scriptsToString(const UScriptCode scripts[], int32_t length, char s[]) {
    int32_t i;
    if(length == 0) {
        strcpy(s, "(no scripts)");
        return;
    }
    s[0] = 0;
    for(i = 0; i < length; ++i) {
        if(i > 0) {
            strcat(s, " ");
        }
        strcat(s, uscript_getShortName(scripts[i]));
    }
}

static void assertEqualScripts(const char *msg,
                               const UScriptCode scripts1[], int32_t length1,
                               const UScriptCode scripts2[], int32_t length2,
                               UErrorCode errorCode) {
    char s1[80];
    char s2[80];
    if(U_FAILURE(errorCode)) {
        log_err("Failed: %s - %s\n", msg, u_errorName(errorCode));
        return;
    }
    scriptsToString(scripts1, length1, s1);
    scriptsToString(scripts2, length2, s2);
    if(0!=strcmp(s1, s2)) {
        log_data_err("Failed: %s: expected %s but got %s\n", msg, s1, s2);
    }
}

void TestUScriptCodeAPI(){
    int i =0;
    int numErrors =0;
    {
        const char* testNames[]={
        
        "en", "en_US", "sr", "ta" , "te_IN",
        "hi", "he", "ar",
        
        "Hani", "Hang","Hebr","Hira",
        "Knda","Kana","Khmr","Lao",
        "Latn", 
        "Mlym", "Mong",
    
        
        "CYRILLIC","DESERET","DEVANAGARI","ETHIOPIC","GEORGIAN", 
        "GOTHIC",  "GREEK",  "GUJARATI", "COMMON", "INHERITED", 
        
        "malayalam", "mongolian", "myanmar", "ogham", "old-italic",
        "oriya",     "runic",     "sinhala", "syriac","tamil",     
        "telugu",    "thaana",    "thai",    "tibetan", 
        
        "tagb", "arabic",
        
        "asfdasd", "5464", "12235",
        
        "zyyy", "YI",
        NULL  
        };
        UScriptCode expected[] ={
            
            USCRIPT_LATIN, USCRIPT_LATIN, USCRIPT_CYRILLIC, USCRIPT_TAMIL, USCRIPT_TELUGU, 
            USCRIPT_DEVANAGARI, USCRIPT_HEBREW, USCRIPT_ARABIC,
            
            USCRIPT_HAN, USCRIPT_HANGUL, USCRIPT_HEBREW, USCRIPT_HIRAGANA,
            USCRIPT_KANNADA, USCRIPT_KATAKANA, USCRIPT_KHMER, USCRIPT_LAO,
            USCRIPT_LATIN, 
            USCRIPT_MALAYALAM, USCRIPT_MONGOLIAN,
            
            USCRIPT_CYRILLIC, USCRIPT_DESERET, USCRIPT_DEVANAGARI, USCRIPT_ETHIOPIC, USCRIPT_GEORGIAN,
            USCRIPT_GOTHIC, USCRIPT_GREEK, USCRIPT_GUJARATI, USCRIPT_COMMON, USCRIPT_INHERITED,
                
            USCRIPT_MALAYALAM, USCRIPT_MONGOLIAN, USCRIPT_MYANMAR, USCRIPT_OGHAM, USCRIPT_OLD_ITALIC,
            USCRIPT_ORIYA, USCRIPT_RUNIC, USCRIPT_SINHALA, USCRIPT_SYRIAC, USCRIPT_TAMIL,
            USCRIPT_TELUGU, USCRIPT_THAANA, USCRIPT_THAI, USCRIPT_TIBETAN,
            
            USCRIPT_TAGBANWA, USCRIPT_ARABIC,
            
            USCRIPT_INVALID_CODE, USCRIPT_INVALID_CODE, USCRIPT_INVALID_CODE,
            USCRIPT_COMMON, USCRIPT_YI,
        };

        UErrorCode err = U_ZERO_ERROR;

        const int32_t capacity = 10;

        for( ; testNames[i]!=NULL; i++){
            UScriptCode script[10]={USCRIPT_INVALID_CODE};
            uscript_getCode(testNames[i],script,capacity, &err);
            if( script[0] != expected[i]){
                   log_data_err("Error getting script code Got: %i  Expected: %i for name %s (Error code does not propagate if data is not present. Are you missing data?)\n",
                       script[0],expected[i],testNames[i]);
                   numErrors++;
            }
        }
        if(numErrors >0 ){
            log_data_err("Errors uchar_getScriptCode() : %i \n",numErrors);
        }
    }

    {
        UErrorCode err = U_ZERO_ERROR;
        int32_t capacity=0;
        int32_t j;
        UScriptCode jaCode[]={USCRIPT_KATAKANA, USCRIPT_HIRAGANA, USCRIPT_HAN };
        UScriptCode script[10]={USCRIPT_INVALID_CODE};
        int32_t num = uscript_getCode("ja",script,capacity, &err);
        
        if(err==U_BUFFER_OVERFLOW_ERROR){
            err = U_ZERO_ERROR;
            capacity = 10;
            num = uscript_getCode("ja",script,capacity, &err);
            if(num!=(sizeof(jaCode)/sizeof(UScriptCode))){
                log_err("Errors uscript_getScriptCode() for Japanese locale: num=%d, expected %d \n",
                        num, (sizeof(jaCode)/sizeof(UScriptCode)));
            }
            for(j=0;j<sizeof(jaCode)/sizeof(UScriptCode);j++) {
                if(script[j]!=jaCode[j]) {
                    log_err("Japanese locale: code #%d was %d (%s) but expected %d (%s)\n", j,
                            script[j], uscript_getName(script[j]),
                            jaCode[j], uscript_getName(jaCode[j]));
                    
                }
            }
        }else{
            log_data_err("Errors in uscript_getScriptCode() expected error : %s got: %s \n", 
                "U_BUFFER_OVERFLOW_ERROR",
                 u_errorName(err));
        }

    }
    {
        static const UScriptCode LATIN[1] = { USCRIPT_LATIN };
        static const UScriptCode CYRILLIC[1] = { USCRIPT_CYRILLIC };
        static const UScriptCode DEVANAGARI[1] = { USCRIPT_DEVANAGARI };
        static const UScriptCode HAN[1] = { USCRIPT_HAN };
        static const UScriptCode JAPANESE[3] = { USCRIPT_KATAKANA, USCRIPT_HIRAGANA, USCRIPT_HAN };
        static const UScriptCode KOREAN[2] = { USCRIPT_HANGUL, USCRIPT_HAN };
        static const UScriptCode HAN_BOPO[2] = { USCRIPT_HAN, USCRIPT_BOPOMOFO };
        UScriptCode scripts[5];
        UErrorCode err;
        int32_t num;

        
        err = U_ZERO_ERROR;
        num = uscript_getCode("tg", scripts, UPRV_LENGTHOF(scripts), &err);
        assertEqualScripts("tg script: Cyrl", CYRILLIC, 1, scripts, num, err);  
        err = U_ZERO_ERROR;
        num = uscript_getCode("xsr", scripts, UPRV_LENGTHOF(scripts), &err);
        assertEqualScripts("xsr script: Deva", DEVANAGARI, 1, scripts, num, err);  

        
        err = U_ZERO_ERROR;
        num = uscript_getCode("ja", scripts, UPRV_LENGTHOF(scripts), &err);
        assertEqualScripts("ja scripts: Kana Hira Hani",
                           JAPANESE, UPRV_LENGTHOF(JAPANESE), scripts, num, err);
        err = U_ZERO_ERROR;
        num = uscript_getCode("ko", scripts, UPRV_LENGTHOF(scripts), &err);
        assertEqualScripts("ko scripts: Hang Hani",
                           KOREAN, UPRV_LENGTHOF(KOREAN), scripts, num, err);
        err = U_ZERO_ERROR;
        num = uscript_getCode("zh", scripts, UPRV_LENGTHOF(scripts), &err);
        assertEqualScripts("zh script: Hani", HAN, 1, scripts, num, err);
        err = U_ZERO_ERROR;
        num = uscript_getCode("zh-Hant", scripts, UPRV_LENGTHOF(scripts), &err);
        assertEqualScripts("zh-Hant scripts: Hani Bopo", HAN_BOPO, 2, scripts, num, err);
        err = U_ZERO_ERROR;
        num = uscript_getCode("zh-TW", scripts, UPRV_LENGTHOF(scripts), &err);
        assertEqualScripts("zh-TW scripts: Hani Bopo", HAN_BOPO, 2, scripts, num, err);

        
        err = U_ZERO_ERROR;
        num = uscript_getCode("ro-RO", scripts, UPRV_LENGTHOF(scripts), &err);
        assertEqualScripts("ro-RO script: Latn", LATIN, 1, scripts, num, err);
    }

    {
        UScriptCode testAbbr[]={
            
            USCRIPT_CYRILLIC, USCRIPT_DESERET, USCRIPT_DEVANAGARI, USCRIPT_ETHIOPIC, USCRIPT_GEORGIAN,
            USCRIPT_GOTHIC, USCRIPT_GREEK, USCRIPT_GUJARATI,
        };

        const char* expectedNames[]={
              
            
            "Cyrillic","Deseret","Devanagari","Ethiopic","Georgian", 
            "Gothic",  "Greek",  "Gujarati", 
             NULL
        };
        i=0;
        while(i<sizeof(testAbbr)/sizeof(UScriptCode)){
            const char* name = uscript_getName(testAbbr[i]);
             if(name == NULL) {
               log_data_err("Couldn't get script name\n");
               return;
             }
            numErrors=0;
            if(strcmp(expectedNames[i],name)!=0){
                log_err("Error getting abbreviations Got: %s Expected: %s\n",name,expectedNames[i]);
                numErrors++;
            }
            if(numErrors > 0){
                if(numErrors >0 ){
                    log_err("Errors uchar_getScriptAbbr() : %i \n",numErrors);
                }
            }
            i++;
        }

    }

    {
        UScriptCode testAbbr[]={
            
            USCRIPT_HAN, USCRIPT_HANGUL, USCRIPT_HEBREW, USCRIPT_HIRAGANA,
            USCRIPT_KANNADA, USCRIPT_KATAKANA, USCRIPT_KHMER, USCRIPT_LAO,
            USCRIPT_LATIN, 
            USCRIPT_MALAYALAM, USCRIPT_MONGOLIAN,
        };

        const char* expectedAbbr[]={
              
            "Hani", "Hang","Hebr","Hira",
            "Knda","Kana","Khmr","Laoo",
            "Latn",
            "Mlym", "Mong",
             NULL
        };
        i=0;
        while(i<sizeof(testAbbr)/sizeof(UScriptCode)){
            const char* name = uscript_getShortName(testAbbr[i]);
            numErrors=0;
            if(strcmp(expectedAbbr[i],name)!=0){
                log_err("Error getting abbreviations Got: %s Expected: %s\n",name,expectedAbbr[i]);
                numErrors++;
            }
            if(numErrors > 0){
                if(numErrors >0 ){
                    log_err("Errors uchar_getScriptAbbr() : %i \n",numErrors);
                }
            }
            i++;
        }

    }
    
    {
        uint32_t codepoints[] = {
                0x0000FF9D, 
                0x0000FFBE, 
                0x0000FFC7, 
                0x0000FFCF, 
                0x0000FFD7, 
                0x0000FFDC, 
                0x00010300, 
                0x00010330, 
                0x0001034A, 
                0x00010400, 
                0x00010428, 
                0x0001D167, 
                0x0001D17B, 
                0x0001D185, 
                0x0001D1AA, 
                0x00020000, 
                0x00000D02, 
                0x00000D00, 
                0x00000000, 
                0x0001D169, 
                0x0001D182, 
                0x0001D18B, 
                0x0001D1AD, 
        };

        UScriptCode expected[] = {
                USCRIPT_KATAKANA ,
                USCRIPT_HANGUL ,
                USCRIPT_HANGUL ,
                USCRIPT_HANGUL ,
                USCRIPT_HANGUL ,
                USCRIPT_HANGUL ,
                USCRIPT_OLD_ITALIC, 
                USCRIPT_GOTHIC ,
                USCRIPT_GOTHIC ,
                USCRIPT_DESERET ,
                USCRIPT_DESERET ,
                USCRIPT_INHERITED,
                USCRIPT_INHERITED,
                USCRIPT_INHERITED,
                USCRIPT_INHERITED,
                USCRIPT_HAN ,
                USCRIPT_MALAYALAM,
                USCRIPT_UNKNOWN,
                USCRIPT_COMMON,
                USCRIPT_INHERITED ,
                USCRIPT_INHERITED ,
                USCRIPT_INHERITED ,
                USCRIPT_INHERITED ,
        };
        UScriptCode code = USCRIPT_INVALID_CODE;
        UErrorCode status = U_ZERO_ERROR;
        UBool passed = TRUE;

        for(i=0; i<UPRV_LENGTHOF(codepoints); ++i){
            code = uscript_getScript(codepoints[i],&status);
            if(U_SUCCESS(status)){
                if( code != expected[i] ||
                    code != (UScriptCode)u_getIntPropertyValue(codepoints[i], UCHAR_SCRIPT)
                ) {
                    log_err("uscript_getScript for codepoint \\U%08X failed\n",codepoints[i]);
                    passed = FALSE;
                }
            }else{
                log_err("uscript_getScript for codepoint \\U%08X failed. Error: %s\n", 
                         codepoints[i],u_errorName(status));
                break;
            }
        }
        
        if(passed==FALSE){
           log_err("uscript_getScript failed.\n");
        }      
    }
    {
        UScriptCode code= USCRIPT_INVALID_CODE;
        UErrorCode  status = U_ZERO_ERROR;
        code = uscript_getScript(0x001D169,&status);
        if(code != USCRIPT_INHERITED){
            log_err("\\U001D169 is not contained in USCRIPT_INHERITED");
        }
    }
    {
        UScriptCode code= USCRIPT_INVALID_CODE;
        UErrorCode  status = U_ZERO_ERROR;
        int32_t err = 0;

        for(i = 0; i<=0x10ffff; i++){
            code =  uscript_getScript(i,&status);
            if(code == USCRIPT_INVALID_CODE){
                err++;
                log_err("uscript_getScript for codepoint \\U%08X failed.\n", i);
            }
        }
        if(err>0){
            log_err("uscript_getScript failed for %d codepoints\n", err);
        }
    }
    {
        for(i=0; (UScriptCode)i< USCRIPT_CODE_LIMIT; i++){
            const char* name = uscript_getName((UScriptCode)i);
            if(name==NULL || strcmp(name,"")==0){
                log_err("uscript_getName failed for code %i: name is NULL or \"\"\n",i);
            }
        }
    }

    {
        







        static const char* expectedLong[] = {
            "Balinese", "Batak", "Blis", "Brahmi", "Cham", "Cirt", "Cyrs",
            "Egyd", "Egyh", "Egyptian_Hieroglyphs",
            "Geok", "Hans", "Hant", "Pahawh_Hmong", "Hung", "Inds",
            "Javanese", "Kayah_Li", "Latf", "Latg",
            "Lepcha", "Linear_A", "Mandaic", "Maya", "Meroitic_Hieroglyphs",
            "Nko", "Old_Turkic", "Old_Permic", "Phags_Pa", "Phoenician", 
            "Miao", "Roro", "Sara", "Syre", "Syrj", "Syrn", "Teng", "Vai", "Visp", "Cuneiform", 
            "Zxxx", "Unknown",
            "Carian", "Jpan", "Tai_Tham", "Lycian", "Lydian", "Ol_Chiki", "Rejang", "Saurashtra", "Sgnw", "Sundanese",
            "Moon", "Meetei_Mayek",
            
            "Imperial_Aramaic", "Avestan", "Chakma", "Kore",
            "Kaithi", "Manichaean", "Inscriptional_Pahlavi", "Psalter_Pahlavi", "Phlv",
            "Inscriptional_Parthian", "Samaritan", "Tai_Viet",
            "Zmth", "Zsym",
            
            "Bamum", "Lisu", "Nkgb", "Old_South_Arabian",
            
            "Bassa_Vah", "Duployan", "Elbasan", "Grantha", "Kpel",
            "Loma", "Mende_Kikakui", "Meroitic_Cursive",
            "Old_North_Arabian", "Nabataean", "Palmyrene", "Khudawadi", "Warang_Citi",
            
            "Afak", "Jurc", "Mro", "Nshu", "Sharada", "Sora_Sompeng", "Takri", "Tang", "Wole",
            
            "Hluw", "Khojki", "Tirhuta",
            
            "Caucasian_Albanian", "Mahajani",
            
            "Ahom", "Hatr", "Modi", "Mult", "Pau_Cin_Hau", "Siddham"
        };
        static const char* expectedShort[] = {
            "Bali", "Batk", "Blis", "Brah", "Cham", "Cirt", "Cyrs", "Egyd", "Egyh", "Egyp",
            "Geok", "Hans", "Hant", "Hmng", "Hung", "Inds", "Java", "Kali", "Latf", "Latg",
            "Lepc", "Lina", "Mand", "Maya", "Mero", "Nkoo", "Orkh", "Perm", "Phag", "Phnx",
            "Plrd", "Roro", "Sara", "Syre", "Syrj", "Syrn", "Teng", "Vaii", "Visp", "Xsux",
            "Zxxx", "Zzzz",
            "Cari", "Jpan", "Lana", "Lyci", "Lydi", "Olck", "Rjng", "Saur", "Sgnw", "Sund",
            "Moon", "Mtei",
            
            "Armi", "Avst", "Cakm", "Kore",
            "Kthi", "Mani", "Phli", "Phlp", "Phlv", "Prti", "Samr", "Tavt",
            "Zmth", "Zsym",
            
            "Bamu", "Lisu", "Nkgb", "Sarb",
            
            "Bass", "Dupl", "Elba", "Gran", "Kpel", "Loma", "Mend", "Merc",
            "Narb", "Nbat", "Palm", "Sind", "Wara",
            
            "Afak", "Jurc", "Mroo", "Nshu", "Shrd", "Sora", "Takr", "Tang", "Wole",
            
            "Hluw", "Khoj", "Tirh",
            
            "Aghb", "Mahj",
            
            "Ahom", "Hatr", "Modi", "Mult", "Pauc", "Sidd"
        };
        int32_t j = 0;
        if(UPRV_LENGTHOF(expectedLong)!=(USCRIPT_CODE_LIMIT-USCRIPT_BALINESE)) {
            log_err("need to add new script codes in cucdapi.c!\n");
            return;
        }
        for(i=USCRIPT_BALINESE; (UScriptCode)i<USCRIPT_CODE_LIMIT; i++, j++){
            const char* name = uscript_getName((UScriptCode)i);
            if(name==NULL || strcmp(name,expectedLong[j])!=0){
                log_err("uscript_getName failed for code %i: %s!=%s\n", i, name, expectedLong[j]);
            }
            name = uscript_getShortName((UScriptCode)i);
            if(name==NULL || strcmp(name,expectedShort[j])!=0){
                log_err("uscript_getShortName failed for code %i: %s!=%s\n", i, name, expectedShort[j]);
            }
        }
        for(i=0; i<UPRV_LENGTHOF(expectedLong); i++){
            UScriptCode fillIn[5] = {USCRIPT_INVALID_CODE};
            UErrorCode status = U_ZERO_ERROR;
            int32_t len = 0;
            len = uscript_getCode(expectedShort[i], fillIn, UPRV_LENGTHOF(fillIn), &status);
            if(U_FAILURE(status)){
                log_err("uscript_getCode failed for script name %s. Error: %s\n",expectedShort[i], u_errorName(status));
            }
            if(len>1){
                log_err("uscript_getCode did not return expected number of codes for script %s. EXPECTED: 1 GOT: %i\n", expectedShort[i], len);
            }
            if(fillIn[0]!= (UScriptCode)(USCRIPT_BALINESE+i)){
                log_err("uscript_getCode did not return expected code for script %s. EXPECTED: %i GOT: %i\n", expectedShort[i], (USCRIPT_BALINESE+i), fillIn[0] );
            }
        }
    }

    {
        
        UErrorCode errorCode=U_ZERO_ERROR;
        if(!(
                USCRIPT_COMMON==uscript_getScript(0x0640, &errorCode) &&
                USCRIPT_INHERITED==uscript_getScript(0x0650, &errorCode) &&
                USCRIPT_ARABIC==uscript_getScript(0xfdf2, &errorCode)) ||
            U_FAILURE(errorCode)
        ) {
            log_err("uscript_getScript(character with Script_Extensions) failed\n");
        }
    }
}

void TestHasScript() {
    if(!(
        !uscript_hasScript(0x063f, USCRIPT_COMMON) &&
        uscript_hasScript(0x063f, USCRIPT_ARABIC) &&  
        !uscript_hasScript(0x063f, USCRIPT_SYRIAC) &&
        !uscript_hasScript(0x063f, USCRIPT_THAANA))
    ) {
        log_err("uscript_hasScript(U+063F, ...) is wrong\n");
    }
    if(!(
        !uscript_hasScript(0x0640, USCRIPT_COMMON) &&  
        uscript_hasScript(0x0640, USCRIPT_ARABIC) &&
        uscript_hasScript(0x0640, USCRIPT_SYRIAC) &&
        !uscript_hasScript(0x0640, USCRIPT_THAANA))
    ) {
        log_err("uscript_hasScript(U+0640, ...) is wrong\n");
    }
    if(!(
        !uscript_hasScript(0x0650, USCRIPT_INHERITED) &&  
        uscript_hasScript(0x0650, USCRIPT_ARABIC) &&
        uscript_hasScript(0x0650, USCRIPT_SYRIAC) &&
        !uscript_hasScript(0x0650, USCRIPT_THAANA))
    ) {
        log_err("uscript_hasScript(U+0650, ...) is wrong\n");
    }
    if(!(
        !uscript_hasScript(0x0660, USCRIPT_COMMON) &&  
        uscript_hasScript(0x0660, USCRIPT_ARABIC) &&
        !uscript_hasScript(0x0660, USCRIPT_SYRIAC) &&
        uscript_hasScript(0x0660, USCRIPT_THAANA))
    ) {
        log_err("uscript_hasScript(U+0660, ...) is wrong\n");
    }
    if(!(
        !uscript_hasScript(0xfdf2, USCRIPT_COMMON) &&
        uscript_hasScript(0xfdf2, USCRIPT_ARABIC) &&  
        !uscript_hasScript(0xfdf2, USCRIPT_SYRIAC) &&
        uscript_hasScript(0xfdf2, USCRIPT_THAANA))
    ) {
        log_err("uscript_hasScript(U+FDF2, ...) is wrong\n");
    }
    if(uscript_hasScript(0x0640, 0xaffe)) {
        
        log_err("uscript_hasScript(U+0640, bogus 0xaffe) is wrong\n");
    }
}

static UBool scriptsContain(int32_t scripts[], int32_t length, int32_t script) {
    UBool contain=FALSE;
    int32_t prev=-1, i;
    for(i=0; i<length; ++i) {
        int32_t s=scripts[i];
        if(s<=prev) {
            log_err("uscript_getScriptExtensions() not in sorted order: %d %d\n", (int)prev, (int)s);
        }
        if(s==script) { contain=TRUE; }
    }
    return contain;
}

void TestGetScriptExtensions() {
    UScriptCode scripts[20];
    int32_t length;
    UErrorCode errorCode;

    
    errorCode=U_PARSE_ERROR;
    length=uscript_getScriptExtensions(0x0640, scripts, UPRV_LENGTHOF(scripts), &errorCode);
    if(errorCode!=U_PARSE_ERROR) {
        log_err("uscript_getScriptExtensions(U+0640, U_PARSE_ERROR) did not preserve the UErrorCode - %s\n",
              u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0x0640, NULL, UPRV_LENGTHOF(scripts), &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("uscript_getScriptExtensions(U+0640, NULL) did not set U_ILLEGAL_ARGUMENT_ERROR - %s\n",
              u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0x0640, scripts, -1, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("uscript_getScriptExtensions(U+0640, capacity<0) did not set U_ILLEGAL_ARGUMENT_ERROR - %s\n",
              u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0x0640, scripts, 0, &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || length<3) {
        log_err("uscript_getScriptExtensions(U+0640, capacity=0: pure preflighting)=%d < 3 - %s\n",
              (int)length, u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0x0640, scripts, 1, &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || length<3) {
        log_err("uscript_getScriptExtensions(U+0640, capacity=1: preflighting)=%d < 3 - %s\n",
              (int)length, u_errorName(errorCode));
    }
    
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0x063f, scripts, 0, &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || length!=1) {
        log_err("uscript_getScriptExtensions(U+063F, capacity=0)=%d != 1 - %s\n",
              (int)length, u_errorName(errorCode));
    }

    
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(-1, scripts, UPRV_LENGTHOF(scripts), &errorCode);
    if(U_FAILURE(errorCode) || length!=1 || scripts[0]!=USCRIPT_UNKNOWN) {
        log_err("uscript_getScriptExtensions(-1)=%d does not return {UNKNOWN} - %s\n",
              (int)length, u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0x110000, scripts, UPRV_LENGTHOF(scripts), &errorCode);
    if(U_FAILURE(errorCode) || length!=1 || scripts[0]!=USCRIPT_UNKNOWN) {
        log_err("uscript_getScriptExtensions(0x110000)=%d does not return {UNKNOWN} - %s\n",
              (int)length, u_errorName(errorCode));
    }

    
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0x063f, scripts, 1, &errorCode);
    if(U_FAILURE(errorCode) || length!=1 || scripts[0]!=USCRIPT_ARABIC) {
        log_err("uscript_getScriptExtensions(U+063F, capacity=1)=%d does not return {ARABIC} - %s\n",
              (int)length, u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0x0640, scripts, UPRV_LENGTHOF(scripts), &errorCode);
    if(U_FAILURE(errorCode) || length<3 ||
            !scriptsContain(scripts, length, USCRIPT_ARABIC) ||
            !scriptsContain(scripts, length, USCRIPT_SYRIAC) ||
            !scriptsContain(scripts, length, USCRIPT_MANDAIC)) {
        log_err("uscript_getScriptExtensions(U+0640)=%d failed - %s\n",
              (int)length, u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0xfdf2, scripts, UPRV_LENGTHOF(scripts), &errorCode);
    if(U_FAILURE(errorCode) || length!=2 || scripts[0]!=USCRIPT_ARABIC || scripts[1]!=USCRIPT_THAANA) {
        log_err("uscript_getScriptExtensions(U+FDF2)=%d failed - %s\n",
              (int)length, u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    length=uscript_getScriptExtensions(0xff65, scripts, UPRV_LENGTHOF(scripts), &errorCode);
    if(U_FAILURE(errorCode) || length!=6 || scripts[0]!=USCRIPT_BOPOMOFO || scripts[5]!=USCRIPT_YI) {
        log_err("uscript_getScriptExtensions(U+FF65)=%d failed - %s\n",
              (int)length, u_errorName(errorCode));
    }
}

void TestScriptMetadataAPI() {
    
    UErrorCode errorCode=U_ZERO_ERROR;
    UChar sample[8];

    if(uscript_getSampleString(USCRIPT_LATIN, sample, UPRV_LENGTHOF(sample), &errorCode)!=1 ||
            U_FAILURE(errorCode) ||
            uscript_getScript(sample[0], &errorCode)!=USCRIPT_LATIN ||
            sample[1]!=0) {
        log_err("uscript_getSampleString(Latn) failed - %s\n", u_errorName(errorCode));
    }
    sample[0]=0xfffe;
    if(uscript_getSampleString(USCRIPT_LATIN, sample, 0, &errorCode)!=1 ||
            errorCode!=U_BUFFER_OVERFLOW_ERROR ||
            sample[0]!=0xfffe) {
        log_err("uscript_getSampleString(Latn, capacity=0) failed - %s\n", u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    if(uscript_getSampleString(USCRIPT_INVALID_CODE, sample, UPRV_LENGTHOF(sample), &errorCode)!=0 ||
            U_FAILURE(errorCode) ||
            sample[0]!=0) {
        log_err("uscript_getSampleString(invalid) failed - %s\n", u_errorName(errorCode));
    }
    sample[0]=0xfffe;
    if(uscript_getSampleString(USCRIPT_CODE_LIMIT, sample, 0, &errorCode)!=0 ||
            errorCode!=U_STRING_NOT_TERMINATED_WARNING ||
            sample[0]!=0xfffe) {
        log_err("uscript_getSampleString(limit, capacity=0) failed - %s\n", u_errorName(errorCode));
    }

    if(uscript_getUsage(USCRIPT_LATIN)!=USCRIPT_USAGE_RECOMMENDED ||
            uscript_getUsage(USCRIPT_YI)!=USCRIPT_USAGE_ASPIRATIONAL ||
            uscript_getUsage(USCRIPT_CHEROKEE)!=USCRIPT_USAGE_LIMITED_USE ||
            uscript_getUsage(USCRIPT_COPTIC)!=USCRIPT_USAGE_EXCLUDED ||
            uscript_getUsage(USCRIPT_CIRTH)!=USCRIPT_USAGE_NOT_ENCODED ||
            uscript_getUsage(USCRIPT_INVALID_CODE)!=USCRIPT_USAGE_NOT_ENCODED ||
            uscript_getUsage(USCRIPT_CODE_LIMIT)!=USCRIPT_USAGE_NOT_ENCODED) {
        log_err("uscript_getUsage() failed\n");
    }

    if(uscript_isRightToLeft(USCRIPT_LATIN) ||
            uscript_isRightToLeft(USCRIPT_CIRTH) ||
            !uscript_isRightToLeft(USCRIPT_ARABIC) ||
            !uscript_isRightToLeft(USCRIPT_HEBREW)) {
        log_err("uscript_isRightToLeft() failed\n");
    }

    if(uscript_breaksBetweenLetters(USCRIPT_LATIN) ||
            uscript_breaksBetweenLetters(USCRIPT_CIRTH) ||
            !uscript_breaksBetweenLetters(USCRIPT_HAN) ||
            !uscript_breaksBetweenLetters(USCRIPT_THAI)) {
        log_err("uscript_breaksBetweenLetters() failed\n");
    }

    if(uscript_isCased(USCRIPT_CIRTH) ||
            uscript_isCased(USCRIPT_HAN) ||
            !uscript_isCased(USCRIPT_LATIN) ||
            !uscript_isCased(USCRIPT_GREEK)) {
        log_err("uscript_isCased() failed\n");
    }
}

void TestBinaryValues() {
    



    static const char *const falseValues[]={ "N", "No", "F", "False" };
    static const char *const trueValues[]={ "Y", "Yes", "T", "True" };
    int32_t i;
    for(i=0; i<UPRV_LENGTHOF(falseValues); ++i) {
        if(FALSE!=u_getPropertyValueEnum(UCHAR_ALPHABETIC, falseValues[i])) {
            log_data_err("u_getPropertyValueEnum(UCHAR_ALPHABETIC, \"%s\")!=FALSE (Are you missing data?)\n", falseValues[i]);
        }
    }
    for(i=0; i<UPRV_LENGTHOF(trueValues); ++i) {
        if(TRUE!=u_getPropertyValueEnum(UCHAR_ALPHABETIC, trueValues[i])) {
            log_data_err("u_getPropertyValueEnum(UCHAR_ALPHABETIC, \"%s\")!=TRUE (Are you missing data?)\n", trueValues[i]);
        }
    }
}
