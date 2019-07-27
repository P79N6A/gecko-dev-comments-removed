


















#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/locid.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/unistr.h"




static UConverter *cnv=NULL;

static void
printUString(const char *announce, const UChar *s, int32_t length) {
    static char out[200];
    UChar32 c;
    int32_t i;
    UErrorCode errorCode=U_ZERO_ERROR;

    




    ucnv_fromUChars(cnv, out, sizeof(out), s, length, &errorCode);
    if(U_FAILURE(errorCode) || errorCode==U_STRING_NOT_TERMINATED_WARNING) {
        printf("%sproblem converting string from Unicode: %s\n", announce, u_errorName(errorCode));
        return;
    }

    printf("%s%s {", announce, out);

    
    if(length>=0) {
        
        for(i=0; i<length; ) {
            U16_NEXT(s, i, length, c);
            printf(" %04x", c);
        }
    } else {
        
        for(i=0; ; ) {
            U16_NEXT(s, i, length, c);
            if(c==0) {
                break;
            }
            printf(" %04x", c);
        }
    }
    printf(" }\n");
}

static void
printUnicodeString(const char *announce, const UnicodeString &s) {
    static char out[200];
    int32_t i, length;

    

    
    
    
    
    
    
    out[s.extract(0, 99, out)]=0;
    printf("%s%s {", announce, out);

    
    length=s.length();
    for(i=0; i<length; ++i) {
        printf(" %04x", s.charAt(i));
    }
    printf(" }\n");
}



static void
demo_utf_h_macros() {
    static UChar input[]={ 0x0061, 0xd800, 0xdc00, 0xdbff, 0xdfff, 0x0062 };
    UChar32 c;
    int32_t i;
    UBool isError;

    printf("\n* demo_utf_h_macros() -------------- ***\n\n");

    printUString("iterate forward through: ", input, UPRV_LENGTHOF(input));
    for(i=0; i<UPRV_LENGTHOF(input); ) {
        





        printf("Codepoint at offset %d: U+", i);
        U16_NEXT(input, i, UPRV_LENGTHOF(input), c);
        printf("%04x\n", c); 
    }

    puts("");

    isError=FALSE;
    i=1; 
    U16_APPEND(input, i, UPRV_LENGTHOF(input), 0x0062, isError);

    printUString("iterate backward through: ", input, UPRV_LENGTHOF(input));
    for(i=UPRV_LENGTHOF(input); i>0; ) {
        U16_PREV(input, 0, i, c);
        






        printf("Codepoint at offset %d: U+%04x\n", i, c);
    }
}



static void demo_C_Unicode_strings() {
    printf("\n* demo_C_Unicode_strings() --------- ***\n\n");

    static const UChar text[]={ 0x41, 0x42, 0x43, 0 };          
    static const UChar appendText[]={ 0x61, 0x62, 0x63, 0 };    
    static const UChar cmpText[]={ 0x61, 0x53, 0x73, 0x43, 0 }; 
    UChar buffer[32];
    int32_t compare;
    int32_t length=u_strlen(text); 

    
    buffer[0]=0;                    
    u_strncat(buffer, text, 1);     
    u_strcat(buffer, appendText);   
    length=u_strlen(buffer);        
    printUString("should be \"Aabc\": ", buffer, -1);

    
    compare=u_strcmp(buffer, text);
    if(compare<=0) {
        printf("String comparison error, expected \"Aabc\" > \"ABC\"\n");
    }

    
    u_strcpy(buffer, text);
    buffer[1]=0xdf; 
    printUString("should be \"A<sharp s>C\": ", buffer, -1);

    
    compare=u_strcasecmp(buffer, cmpText, U_FOLD_CASE_DEFAULT);
    if(compare!=0) {
        printf("String case insensitive comparison error, expected \"AbC\" to be equal to \"ABC\"\n");
    }
}



static void demoCaseMapInC() {
    






    static const UChar input[]={
        0x61, 0x42, 0x3a3,
        0x69, 0x49, 0x131, 0x130, 0x20,
        0xdf, 0x20, 0xfb03,
        0x3c2, 0x3c3, 0x3a3, 0
    };
    UChar buffer[32];

    UErrorCode errorCode;
    UChar32 c;
    int32_t i, j, length;
    UBool isError;

    printf("\n* demoCaseMapInC() ----------------- ***\n\n");

    







    printUString("input string: ", input, -1);

    
    isError=FALSE;
    for(i=j=0; j<UPRV_LENGTHOF(buffer) && !isError; ) {
        U16_NEXT(input, i, INT32_MAX, c); 
        if(c==0) {
            break; 
        }
        c=u_toupper(c);
        U16_APPEND(buffer, j, UPRV_LENGTHOF(buffer), c, isError);
    }
    printUString("simple-uppercased: ", buffer, j);
    
    isError=FALSE;
    for(i=j=0; j<UPRV_LENGTHOF(buffer) && !isError; ) {
        U16_NEXT(input, i, INT32_MAX, c); 
        if(c==0) {
            break; 
        }
        c=u_tolower(c);
        U16_APPEND(buffer, j, UPRV_LENGTHOF(buffer), c, isError);
    }
    printUString("simple-lowercased: ", buffer, j);
    
    isError=FALSE;
    for(i=j=0; j<UPRV_LENGTHOF(buffer) && !isError; ) {
        U16_NEXT(input, i, INT32_MAX, c); 
        if(c==0) {
            break; 
        }
        c=u_totitle(c);
        U16_APPEND(buffer, j, UPRV_LENGTHOF(buffer), c, isError);
    }
    printUString("simple-titlecased: ", buffer, j);
    
    isError=FALSE;
    for(i=j=0; j<UPRV_LENGTHOF(buffer) && !isError; ) {
        U16_NEXT(input, i, INT32_MAX, c); 
        if(c==0) {
            break; 
        }
        c=u_foldCase(c, U_FOLD_CASE_DEFAULT);
        U16_APPEND(buffer, j, UPRV_LENGTHOF(buffer), c, isError);
    }
    printUString("simple-case-folded/default: ", buffer, j);
    
    isError=FALSE;
    for(i=j=0; j<UPRV_LENGTHOF(buffer) && !isError; ) {
        U16_NEXT(input, i, INT32_MAX, c); 
        if(c==0) {
            break; 
        }
        c=u_foldCase(c, U_FOLD_CASE_EXCLUDE_SPECIAL_I);
        U16_APPEND(buffer, j, UPRV_LENGTHOF(buffer), c, isError);
    }
    printUString("simple-case-folded/Turkic: ", buffer, j);

    












    printUString("\ninput string: ", input, -1);

    
    errorCode=U_ZERO_ERROR;
    length=u_strToLower(buffer, UPRV_LENGTHOF(buffer), input, -1, "en", &errorCode);
    if(U_SUCCESS(errorCode)) {
        printUString("full-lowercased/en: ", buffer, length);
    } else {
        printf("error in u_strToLower(en)=%ld error=%s\n", length, u_errorName(errorCode));
    }
    
    errorCode=U_ZERO_ERROR;
    length=u_strToLower(buffer, UPRV_LENGTHOF(buffer), input, -1, "tr", &errorCode);
    if(U_SUCCESS(errorCode)) {
        printUString("full-lowercased/tr: ", buffer, length);
    } else {
        printf("error in u_strToLower(tr)=%ld error=%s\n", length, u_errorName(errorCode));
    }
    
    errorCode=U_ZERO_ERROR;
    length=u_strToUpper(buffer, UPRV_LENGTHOF(buffer), input, -1, "en", &errorCode);
    if(U_SUCCESS(errorCode)) {
        printUString("full-uppercased/en: ", buffer, length);
    } else {
        printf("error in u_strToUpper(en)=%ld error=%s\n", length, u_errorName(errorCode));
    }
    
    errorCode=U_ZERO_ERROR;
    length=u_strToUpper(buffer, UPRV_LENGTHOF(buffer), input, -1, "tr", &errorCode);
    if(U_SUCCESS(errorCode)) {
        printUString("full-uppercased/tr: ", buffer, length);
    } else {
        printf("error in u_strToUpper(tr)=%ld error=%s\n", length, u_errorName(errorCode));
    }
    
    errorCode=U_ZERO_ERROR;
    length=u_strToTitle(buffer, UPRV_LENGTHOF(buffer), input, -1, NULL, "en", &errorCode);
    if(U_SUCCESS(errorCode)) {
        printUString("full-titlecased/en: ", buffer, length);
    } else {
        printf("error in u_strToTitle(en)=%ld error=%s\n", length, u_errorName(errorCode));
    }
    
    errorCode=U_ZERO_ERROR;
    length=u_strToTitle(buffer, UPRV_LENGTHOF(buffer), input, -1, NULL, "tr", &errorCode);
    if(U_SUCCESS(errorCode)) {
        printUString("full-titlecased/tr: ", buffer, length);
    } else {
        printf("error in u_strToTitle(tr)=%ld error=%s\n", length, u_errorName(errorCode));
    }
    
    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(buffer, UPRV_LENGTHOF(buffer), input, -1, U_FOLD_CASE_DEFAULT, &errorCode);
    if(U_SUCCESS(errorCode)) {
        printUString("full-case-folded/default: ", buffer, length);
    } else {
        printf("error in u_strFoldCase(default)=%ld error=%s\n", length, u_errorName(errorCode));
    }
    
    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(buffer, UPRV_LENGTHOF(buffer), input, -1, U_FOLD_CASE_EXCLUDE_SPECIAL_I, &errorCode);
    if(U_SUCCESS(errorCode)) {
        printUString("full-case-folded/Turkic: ", buffer, length);
    } else {
        printf("error in u_strFoldCase(Turkic)=%ld error=%s\n", length, u_errorName(errorCode));
    }
}



static void demoCaseMapInCPlusPlus() {
    






    static const UChar input[]={
        0x61, 0x42, 0x3a3,
        0x69, 0x49, 0x131, 0x130, 0x20,
        0xdf, 0x20, 0xfb03,
        0x3c2, 0x3c3, 0x3a3, 0
    };

    printf("\n* demoCaseMapInCPlusPlus() --------- ***\n\n");

    UnicodeString s(input), t;
    const Locale &en=Locale::getEnglish();
    Locale tr("tr");

    





    printUnicodeString("input string: ", s);

    
    printUnicodeString("full-lowercased/en: ", (t=s).toLower(en));
    
    printUnicodeString("full-lowercased/tr: ", (t=s).toLower(tr));
    
    printUnicodeString("full-uppercased/en: ", (t=s).toUpper(en));
    
    printUnicodeString("full-uppercased/tr: ", (t=s).toUpper(tr));
    
    printUnicodeString("full-titlecased/en: ", (t=s).toTitle(NULL, en));
    
    printUnicodeString("full-titlecased/tr: ", (t=s).toTitle(NULL, tr));
    
    printUnicodeString("full-case-folded/default: ", (t=s).foldCase(U_FOLD_CASE_DEFAULT));
    
    printUnicodeString("full-case-folded/Turkic: ", (t=s).foldCase(U_FOLD_CASE_EXCLUDE_SPECIAL_I));
}



static const UChar readonly[]={
    0x61, 0x31, 0x20ac
};
static UChar writeable[]={
    0x62, 0x32, 0xdbc0, 0xdc01 
};
static char out[100];

static void
demoUnicodeStringStorage() {
    
    
    
    
    
    int32_t i;

    printf("\n* demoUnicodeStringStorage() ------- ***\n\n");

    
    
    
    UnicodeString one((UChar32)0x24001);
    
    UnicodeString two=one;
    printf("length of short string copy: %d\n", two.length());
    
    
    one.setTo(readonly, UPRV_LENGTHOF(readonly));

    
    
    one+=UnicodeString(writeable, UPRV_LENGTHOF(writeable));
    one+=one;
    one+=one;
    printf("length of longer string: %d\n", one.length());
    
    
    two=one;
    printf("length of longer string copy: %d\n", two.length());

    
    
    UnicodeString three(FALSE, readonly, UPRV_LENGTHOF(readonly));
    printUnicodeString("readonly-alias string: ", three);
    
    
    three.setCharAt(1, 0x39);
    printUnicodeString("readonly-aliasing string after modification: ", three);
    
    for(i=0; i<three.length(); ++i) {
        printf("readonly buffer[%d] after modifying its string: 0x%lx\n",
               i, readonly[i]);
    }
    
    one.setTo(FALSE, writeable, UPRV_LENGTHOF(writeable));
    
    
    two.fastCopyFrom(one);
    printUnicodeString("fastCopyFrom(readonly alias of \"writeable\" array): ", two);
    printf("verify that a fastCopyFrom(readonly alias) uses the same buffer pointer: %d (should be 1)\n",
        one.getBuffer()==two.getBuffer());
    
    two=one;
    printf("verify that a regular copy of a readonly alias uses a different buffer pointer: %d (should be 0)\n",
        one.getBuffer()==two.getBuffer());

    
    UnicodeString four(writeable, UPRV_LENGTHOF(writeable), UPRV_LENGTHOF(writeable));
    printUnicodeString("writeable-alias string: ", four);
    
    four.setCharAt(1, 0x39);
    for(i=0; i<four.length(); ++i) {
        printf("writeable-alias backing buffer[%d]=0x%lx "
               "after modification\n", i, writeable[i]);
    }
    
    
    two=four;
    two.setCharAt(1, 0x21);
    for(i=0; i<two.length(); ++i) {
        printf("writeable-alias backing buffer[%d]=0x%lx after "
               "modification of string copy\n", i, writeable[i]);
    }
    
    one.setTo(writeable, UPRV_LENGTHOF(writeable), UPRV_LENGTHOF(writeable));
    
    
    one.append((UChar)0x40);
    
    one.truncate(one.length()-1);
    
    one.setCharAt(1, 0x25);
    printf("string after growing too much and then shrinking[1]=0x%lx\n"
           "                          backing store for this[1]=0x%lx\n",
           one.charAt(1), writeable[1]);
    
    
    
    if(one.length()<UPRV_LENGTHOF(writeable)) {
        i=one.length();
    } else {
        i=UPRV_LENGTHOF(writeable);
    }
    one.extract(0, i, writeable);
    for(i=0; i<UPRV_LENGTHOF(writeable); ++i) {
        printf("writeable-alias backing buffer[%d]=0x%lx after re-extract\n",
               i, writeable[i]);
    }
}



static void
demoUnicodeStringInit() {
    
    
    

    printf("\n* demoUnicodeStringInit() ---------- ***\n\n");

    
    UnicodeString invariantOnly=UNICODE_STRING("such characters are safe 123 %-.", 32);

    





    
    U_STRING_DECL(invString, "such characters are safe 123 %-.", 32);
    
    U_STRING_INIT(invString, "such characters are safe 123 %-.", 32);

    
    printf("C and C++ Unicode strings are equal: %d\n", invariantOnly==UnicodeString(TRUE, invString, 32));

    



    static const char *cs1="such characters are safe 123 %-.";
    static UChar us1[40];
    static char cs2[40];
    u_charsToUChars(cs1, us1, 33); 
    u_UCharsToChars(us1, cs2, 33);
    printf("char * -> UChar * -> char * with only "
           "invariant characters: \"%s\"\n",
           cs2);

    
    
    
    
    UnicodeString german=UNICODE_STRING(
        "Sch\\u00f6nes Auto: \\u20ac 11240.\\fPrivates Zeichen: \\U00102345\\n", 64).
        unescape();
    printUnicodeString("german UnicodeString from unescaping:\n    ", german);

    



    UChar buffer[200];
    int32_t length;
    length=u_unescape(
        "Sch\\u00f6nes Auto: \\u20ac 11240.\\fPrivates Zeichen: \\U00102345\\n",
        buffer, UPRV_LENGTHOF(buffer));
    printf("german C Unicode string from char * unescaping: (length %d)\n    ", length);
    printUnicodeString("", UnicodeString(buffer));
}

extern int
main(int argc, const char *argv[]) {
    UErrorCode errorCode=U_ZERO_ERROR;

    

    
    
    
    
    
    cnv=ucnv_open(NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error %s opening the default converter\n", u_errorName(errorCode));
        return errorCode;
    }

    ucnv_setFromUCallBack(cnv, UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_C, NULL, NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error %s setting the escape callback in the default converter\n", u_errorName(errorCode));
        ucnv_close(cnv);
        return errorCode;
    }

    demo_utf_h_macros();
    demo_C_Unicode_strings();
    demoCaseMapInC();
    demoCaseMapInCPlusPlus();
    demoUnicodeStringStorage();
    demoUnicodeStringInit();

    ucnv_close(cnv);
    return 0;
}
