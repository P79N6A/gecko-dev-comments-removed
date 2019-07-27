

















#include <string.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/uloc.h"
#include "unicode/ubrk.h"
#include "unicode/ucasemap.h"
#include "cmemory.h"
#include "cintltst.h"
#include "ustr_imp.h"



static void
TestCaseLower(void) {
    static const UChar

    beforeLower[]= { 0x61, 0x42, 0x49,  0x3a3, 0xdf, 0x3a3, 0x2f, 0xd93f, 0xdfff },
    lowerRoot[]=   { 0x61, 0x62, 0x69,  0x3c3, 0xdf, 0x3c2, 0x2f, 0xd93f, 0xdfff },
    lowerTurkish[]={ 0x61, 0x62, 0x131, 0x3c3, 0xdf, 0x3c2, 0x2f, 0xd93f, 0xdfff };

    UChar buffer[32];
    int32_t length;
    UErrorCode errorCode;

    
    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToLower(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        beforeLower, sizeof(beforeLower)/U_SIZEOF_UCHAR,
                        "",
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(lowerRoot)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(lowerRoot, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        log_err("error in u_strToLower(root locale)=%ld error=%s string matches: %s\t\nlowerRoot=%s\t\nbuffer=%s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(lowerRoot, buffer, length*U_SIZEOF_UCHAR)==0 &&
buffer[length]==0 ? "yes" : "no",
            aescstrdup(lowerRoot,-1),
            aescstrdup(buffer,-1));
    }

    
    uprv_memcpy(buffer, beforeLower, sizeof(beforeLower));
    buffer[sizeof(beforeLower)/U_SIZEOF_UCHAR]=0;
    errorCode=U_ZERO_ERROR;
    length=u_strToLower(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        buffer, -1, 
                        "tr",
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(lowerTurkish)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(lowerTurkish, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        log_err("error in u_strToLower(turkish locale)=%ld error=%s string matches: %s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(lowerTurkish, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no");
    }

    
    buffer[0]=buffer[2]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToLower(buffer, 2, 
                        beforeLower, sizeof(beforeLower)/U_SIZEOF_UCHAR,
                        "",
                        &errorCode);
    if( errorCode!=U_BUFFER_OVERFLOW_ERROR ||
        length!=(sizeof(lowerRoot)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(lowerRoot, buffer, 2*U_SIZEOF_UCHAR)!=0 ||
        buffer[2]!=0xabcd
    ) {
        log_err("error in u_strToLower(root locale preflighting)=%ld error=%s string matches: %s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(lowerRoot, buffer, 2*U_SIZEOF_UCHAR)==0 && buffer[2]==0xabcd ? "yes" : "no");
    }

    
    errorCode=U_ZERO_ERROR;
    length=u_strToLower(NULL, sizeof(buffer)/U_SIZEOF_UCHAR,
                        beforeLower, sizeof(beforeLower)/U_SIZEOF_UCHAR,
                        "",
                        &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("error in u_strToLower(root locale dest=NULL)=%ld error=%s\n",
            length,
            u_errorName(errorCode));
    }

    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToLower(buffer, -1,
                        beforeLower, sizeof(beforeLower)/U_SIZEOF_UCHAR,
                        "",
                        &errorCode);
    if( errorCode!=U_ILLEGAL_ARGUMENT_ERROR ||
        buffer[0]!=0xabcd
    ) {
        log_err("error in u_strToLower(root locale destCapacity=-1)=%ld error=%s buffer[0]==0x%lx\n",
            length,
            u_errorName(errorCode),
            buffer[0]);
    }
}

static void
TestCaseUpper(void) {
    static const UChar

    beforeUpper[]= { 0x61, 0x42, 0x69,  0x3c2, 0xdf,       0x3c3, 0x2f, 0xfb03,           0xd93f, 0xdfff },
    upperRoot[]=   { 0x41, 0x42, 0x49,  0x3a3, 0x53, 0x53, 0x3a3, 0x2f, 0x46, 0x46, 0x49, 0xd93f, 0xdfff },
    upperTurkish[]={ 0x41, 0x42, 0x130, 0x3a3, 0x53, 0x53, 0x3a3, 0x2f, 0x46, 0x46, 0x49, 0xd93f, 0xdfff };

    UChar buffer[32];
    int32_t length;
    UErrorCode errorCode;

    
    uprv_memcpy(buffer, beforeUpper, sizeof(beforeUpper));
    errorCode=U_ZERO_ERROR;
    length=u_strToUpper(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        buffer, sizeof(beforeUpper)/U_SIZEOF_UCHAR,
                        "",
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(upperRoot)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(upperRoot, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        log_err("error in u_strToUpper(root locale)=%ld error=%s string matches: %s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(upperRoot, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no");
    }

    
    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToUpper(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        beforeUpper, sizeof(beforeUpper)/U_SIZEOF_UCHAR,
                        "tr",
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(upperTurkish)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(upperTurkish, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        log_err("error in u_strToUpper(turkish locale)=%ld error=%s string matches: %s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(upperTurkish, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no");
    }

    
    errorCode=U_ZERO_ERROR;
    length=u_strToUpper(NULL, 0,
                        beforeUpper, sizeof(beforeUpper)/U_SIZEOF_UCHAR,
                        "tr",
                        &errorCode);
    if( errorCode!=U_BUFFER_OVERFLOW_ERROR ||
        length!=(sizeof(upperTurkish)/U_SIZEOF_UCHAR)
    ) {
        log_err("error in u_strToUpper(turkish locale pure preflighting)=%ld error=%s\n",
            length,
            u_errorName(errorCode));
    }

    
    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToUpper(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        NULL, sizeof(beforeUpper)/U_SIZEOF_UCHAR,
                        "tr",
                        &errorCode);
    if( errorCode!=U_ILLEGAL_ARGUMENT_ERROR ||
        buffer[0]!=0xabcd
    ) {
        log_err("error in u_strToUpper(turkish locale src=NULL)=%ld error=%s buffer[0]==0x%lx\n",
            length,
            u_errorName(errorCode),
            buffer[0]);
    }

    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToUpper(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        beforeUpper, -2,
                        "tr",
                        &errorCode);
    if( errorCode!=U_ILLEGAL_ARGUMENT_ERROR ||
        buffer[0]!=0xabcd
    ) {
        log_err("error in u_strToUpper(turkish locale srcLength=-2)=%ld error=%s buffer[0]==0x%lx\n",
            length,
            u_errorName(errorCode),
            buffer[0]);
    }
}

#if !UCONFIG_NO_BREAK_ITERATION

static void
TestCaseTitle(void) {
    static const UChar

    beforeTitle[]= { 0x61, 0x42, 0x20, 0x69,  0x3c2, 0x20, 0xdf,       0x3c3, 0x2f, 0xfb03,           0xd93f, 0xdfff },
    titleWord[]=   { 0x41, 0x62, 0x20, 0x49,  0x3c2, 0x20, 0x53, 0x73, 0x3c3, 0x2f, 0x46, 0x66, 0x69, 0xd93f, 0xdfff },
    titleChar[]=   { 0x41, 0x42, 0x20, 0x49,  0x3a3, 0x20, 0x53, 0x73, 0x3a3, 0x2f, 0x46, 0x66, 0x69, 0xd93f, 0xdfff };

    UChar buffer[32];
    UBreakIterator *titleIterChars;
    int32_t length;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    titleIterChars=ubrk_open(UBRK_CHARACTER, "", beforeTitle, sizeof(beforeTitle)/U_SIZEOF_UCHAR, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err_status(errorCode, "error: ubrk_open(UBRK_CHARACTER)->%s\n", u_errorName(errorCode));
        return;
    }

    
    uprv_memcpy(buffer, beforeTitle, sizeof(beforeTitle));
    errorCode=U_ZERO_ERROR;
    length=u_strToTitle(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        buffer, sizeof(beforeTitle)/U_SIZEOF_UCHAR,
                        NULL, "",
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(titleWord)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(titleWord, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        log_err("error in u_strToTitle(standard iterator)=%ld error=%s string matches: %s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(titleWord, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no");
    }

    
    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToTitle(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        beforeTitle, sizeof(beforeTitle)/U_SIZEOF_UCHAR,
                        titleIterChars, "",
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(titleChar)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(titleChar, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        log_err("error in u_strToTitle(UBRK_CHARACTERS)=%ld error=%s string matches: %s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(titleChar, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no");
    }

    
    errorCode=U_ZERO_ERROR;
    length=u_strToTitle(NULL, 0,
                        beforeTitle, sizeof(beforeTitle)/U_SIZEOF_UCHAR,
                        titleIterChars, "",
                        &errorCode);
    if( errorCode!=U_BUFFER_OVERFLOW_ERROR ||
        length!=(sizeof(titleChar)/U_SIZEOF_UCHAR)
    ) {
        log_err("error in u_strToTitle(UBRK_CHARACTERS pure preflighting)=%ld error=%s\n",
            length,
            u_errorName(errorCode));
    }

    
    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToTitle(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        NULL, sizeof(beforeTitle)/U_SIZEOF_UCHAR,
                        titleIterChars, "",
                        &errorCode);
    if( errorCode!=U_ILLEGAL_ARGUMENT_ERROR ||
        buffer[0]!=0xabcd
    ) {
        log_err("error in u_strToTitle(UBRK_CHARACTERS src=NULL)=%ld error=%s buffer[0]==0x%lx\n",
            length,
            u_errorName(errorCode),
            buffer[0]);
    }

    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToTitle(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        beforeTitle, -2,
                        titleIterChars, "",
                        &errorCode);
    if( errorCode!=U_ILLEGAL_ARGUMENT_ERROR ||
        buffer[0]!=0xabcd
    ) {
        log_err("error in u_strToTitle(UBRK_CHARACTERS srcLength=-2)=%ld error=%s buffer[0]==0x%lx\n",
            length,
            u_errorName(errorCode),
            buffer[0]);
    }

    ubrk_close(titleIterChars);
}

static void
TestCaseDutchTitle(void) {
    static const UChar

    beforeTitle[]= { 0x69, 0x6A, 0x73, 0x73,  0x45, 0x6c, 0x20, 0x69, 0x67, 0x6c, 0x4f, 0x6f , 0x20 , 0x49, 0x4A, 0x53, 0x53, 0x45, 0x4C },
    titleRoot[]=   { 0x49, 0x6A, 0x73, 0x73,  0x65, 0x6c, 0x20, 0x49, 0x67, 0x6c, 0x6f, 0x6f , 0x20 , 0x49, 0x6A, 0x73, 0x73, 0x65, 0x6C },
    titleDutch[]=  { 0x49, 0x4A, 0x73, 0x73,  0x65, 0x6c, 0x20, 0x49, 0x67, 0x6c, 0x6f, 0x6f , 0x20 , 0x49, 0x4A, 0x73, 0x73, 0x65, 0x6C };

    UChar buffer[32];
    UBreakIterator *titleIterWord;
    int32_t length;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    titleIterWord=ubrk_open(UBRK_WORD, "", beforeTitle, sizeof(beforeTitle)/U_SIZEOF_UCHAR, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err_status(errorCode, "error: ubrk_open(UBRK_WORD)->%s\n", u_errorName(errorCode));
        return;
    }

    
    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToTitle(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        beforeTitle, sizeof(beforeTitle)/U_SIZEOF_UCHAR,
                        titleIterWord, "",
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(titleRoot)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(titleRoot, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        char charsOut[21];
        u_UCharsToChars(buffer,charsOut,sizeof(charsOut));
        log_err("error in u_strToTitle(UBRK_CHARACTERS)=%ld error=%s root locale string matches: %s\noutput buffer is {%s}\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(titleRoot, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no", charsOut);
    }
    
    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strToTitle(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        beforeTitle, sizeof(beforeTitle)/U_SIZEOF_UCHAR,
                        titleIterWord, "nl",
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(titleDutch)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(titleDutch, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        char charsOut[21];
        u_UCharsToChars(buffer,charsOut,sizeof(charsOut));
        log_err("error in u_strToTitle(UBRK_CHARACTERS)=%ld error=%s dutch locale string matches: %s\noutput buffer is {%s}\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(titleDutch, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no", charsOut);
    }

    ubrk_close(titleIterWord);
}

#endif



static void
TestCaseFolding(void) {
    









    static const UChar32
    simple[]={
        
        0x61,   0x61,  0x61,
        0x49,   0x69,  0x131,
        0x130,  0x130, 0x69,
        0x131,  0x131, 0x131,
        0xdf,   0xdf,  0xdf,
        0xfb03, 0xfb03, 0xfb03,
        0x1040e,0x10436,0x10436,
        0x5ffff,0x5ffff,0x5ffff
    };

    static const UChar
    mixed[]=                { 0x61, 0x42, 0x130,       0x49,  0x131, 0x3d0, 0xdf,       0xfb03,           0xd93f, 0xdfff },
    foldedDefault[]=        { 0x61, 0x62, 0x69, 0x307, 0x69,  0x131, 0x3b2, 0x73, 0x73, 0x66, 0x66, 0x69, 0xd93f, 0xdfff },
    foldedExcludeSpecialI[]={ 0x61, 0x62, 0x69,        0x131, 0x131, 0x3b2, 0x73, 0x73, 0x66, 0x66, 0x69, 0xd93f, 0xdfff };

    UVersionInfo unicodeVersion={ 0, 0, 17, 89 }, unicode_3_1={ 3, 1, 0, 0 };

    const UChar32 *p;
    int32_t i;

    UChar buffer[32];
    int32_t length;
    UErrorCode errorCode;
    UBool isUnicode_3_1;

    
    u_getUnicodeVersion(unicodeVersion);
    isUnicode_3_1= uprv_memcmp(unicodeVersion, unicode_3_1, 4)>=0;

    
    p=simple;
    for(i=0; i<sizeof(simple)/12; p+=3, ++i) {
        if(u_foldCase(p[0], U_FOLD_CASE_DEFAULT)!=p[1]) {
            log_err("error: u_foldCase(0x%04lx, default)=0x%04lx instead of 0x%04lx\n",
                    p[0], u_foldCase(p[0], U_FOLD_CASE_DEFAULT), p[1]);
            return;
        }

        if(isUnicode_3_1 && u_foldCase(p[0], U_FOLD_CASE_EXCLUDE_SPECIAL_I)!=p[2]) {
            log_err("error: u_foldCase(0x%04lx, exclude special i)=0x%04lx instead of 0x%04lx\n",
                    p[0], u_foldCase(p[0], U_FOLD_CASE_EXCLUDE_SPECIAL_I), p[2]);
            return;
        }
    }

    
    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        mixed, sizeof(mixed)/U_SIZEOF_UCHAR,
                        U_FOLD_CASE_DEFAULT,
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(foldedDefault)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(foldedDefault, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        log_err("error in u_strFoldCase(default)=%ld error=%s string matches: %s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(foldedDefault, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no");
    }

    
    if(isUnicode_3_1) {
        buffer[0]=0xabcd;
        errorCode=U_ZERO_ERROR;
        length=u_strFoldCase(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                            mixed, sizeof(mixed)/U_SIZEOF_UCHAR,
                            U_FOLD_CASE_EXCLUDE_SPECIAL_I,
                            &errorCode);
        if( U_FAILURE(errorCode) ||
            length!=(sizeof(foldedExcludeSpecialI)/U_SIZEOF_UCHAR) ||
            uprv_memcmp(foldedExcludeSpecialI, buffer, length*U_SIZEOF_UCHAR)!=0 ||
            buffer[length]!=0
        ) {
            log_err("error in u_strFoldCase(exclude special i)=%ld error=%s string matches: %s\n",
                length,
                u_errorName(errorCode),
                uprv_memcmp(foldedExcludeSpecialI, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no");
        }
    }

    
    uprv_memcpy(buffer, mixed, sizeof(mixed));
    buffer[sizeof(mixed)/U_SIZEOF_UCHAR]=0;
    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        buffer, -1, 
                        U_FOLD_CASE_DEFAULT,
                        &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=(sizeof(foldedDefault)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(foldedDefault, buffer, length*U_SIZEOF_UCHAR)!=0 ||
        buffer[length]!=0
    ) {
        log_err("error in u_strFoldCase(default same buffer)=%ld error=%s string matches: %s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(foldedDefault, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no");
    }

    
    if(isUnicode_3_1) {
        uprv_memcpy(buffer, mixed, sizeof(mixed));
        errorCode=U_ZERO_ERROR;
        length=u_strFoldCase(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                            buffer, sizeof(mixed)/U_SIZEOF_UCHAR,
                            U_FOLD_CASE_EXCLUDE_SPECIAL_I,
                            &errorCode);
        if( U_FAILURE(errorCode) ||
            length!=(sizeof(foldedExcludeSpecialI)/U_SIZEOF_UCHAR) ||
            uprv_memcmp(foldedExcludeSpecialI, buffer, length*U_SIZEOF_UCHAR)!=0 ||
            buffer[length]!=0
        ) {
            log_err("error in u_strFoldCase(exclude special i same buffer)=%ld error=%s string matches: %s\n",
                length,
                u_errorName(errorCode),
                uprv_memcmp(foldedExcludeSpecialI, buffer, length*U_SIZEOF_UCHAR)==0 && buffer[length]==0 ? "yes" : "no");
        }
    }

    
    buffer[0]=buffer[2]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(buffer, 2, 
                        mixed, sizeof(mixed)/U_SIZEOF_UCHAR,
                        U_FOLD_CASE_DEFAULT,
                        &errorCode);
    if( errorCode!=U_BUFFER_OVERFLOW_ERROR ||
        length!=(sizeof(foldedDefault)/U_SIZEOF_UCHAR) ||
        uprv_memcmp(foldedDefault, buffer, 2*U_SIZEOF_UCHAR)!=0 ||
        buffer[2]!=0xabcd
    ) {
        log_err("error in u_strFoldCase(default preflighting)=%ld error=%s string matches: %s\n",
            length,
            u_errorName(errorCode),
            uprv_memcmp(foldedDefault, buffer, 2*U_SIZEOF_UCHAR)==0 && buffer[2]==0xabcd ? "yes" : "no");
    }

    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(NULL, 0,
                        mixed, sizeof(mixed)/U_SIZEOF_UCHAR,
                        U_FOLD_CASE_DEFAULT,
                        &errorCode);
    if( errorCode!=U_BUFFER_OVERFLOW_ERROR ||
        length!=(sizeof(foldedDefault)/U_SIZEOF_UCHAR)
    ) {
        log_err("error in u_strFoldCase(default pure preflighting)=%ld error=%s\n",
            length,
            u_errorName(errorCode));
    }

    
    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(NULL, sizeof(buffer)/U_SIZEOF_UCHAR,
                        mixed, sizeof(mixed)/U_SIZEOF_UCHAR,
                        U_FOLD_CASE_DEFAULT,
                        &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("error in u_strFoldCase(default dest=NULL)=%ld error=%s\n",
            length,
            u_errorName(errorCode));
    }

    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(buffer, -1,
                        mixed, sizeof(mixed)/U_SIZEOF_UCHAR,
                        U_FOLD_CASE_DEFAULT,
                        &errorCode);
    if( errorCode!=U_ILLEGAL_ARGUMENT_ERROR ||
        buffer[0]!=0xabcd
    ) {
        log_err("error in u_strFoldCase(default destCapacity=-1)=%ld error=%s buffer[0]==0x%lx\n",
            length,
            u_errorName(errorCode),
            buffer[0]);
    }

    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        NULL, sizeof(mixed)/U_SIZEOF_UCHAR,
                        U_FOLD_CASE_EXCLUDE_SPECIAL_I,
                        &errorCode);
    if( errorCode!=U_ILLEGAL_ARGUMENT_ERROR ||
        buffer[0]!=0xabcd
    ) {
        log_err("error in u_strFoldCase(exclude special i src=NULL)=%ld error=%s buffer[0]==0x%lx\n",
            length,
            u_errorName(errorCode),
            buffer[0]);
    }

    buffer[0]=0xabcd;
    errorCode=U_ZERO_ERROR;
    length=u_strFoldCase(buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                        mixed, -2,
                        U_FOLD_CASE_EXCLUDE_SPECIAL_I,
                        &errorCode);
    if( errorCode!=U_ILLEGAL_ARGUMENT_ERROR ||
        buffer[0]!=0xabcd
    ) {
        log_err("error in u_strFoldCase(exclude special i srcLength=-2)=%ld error=%s buffer[0]==0x%lx\n",
            length,
            u_errorName(errorCode),
            buffer[0]);
    }
}

static void
TestCaseCompare(void) {
    static const UChar

    mixed[]=               { 0x61, 0x42, 0x131, 0x3a3, 0xdf,       0xfb03,           0xd93f, 0xdfff, 0 },
    otherDefault[]=        { 0x41, 0x62, 0x131, 0x3c3, 0x73, 0x53, 0x46, 0x66, 0x49, 0xd93f, 0xdfff, 0 },
    otherExcludeSpecialI[]={ 0x41, 0x62, 0x131, 0x3c3, 0x53, 0x73, 0x66, 0x46, 0x69, 0xd93f, 0xdfff, 0 },
    different[]=           { 0x41, 0x62, 0x131, 0x3c3, 0x73, 0x53, 0x46, 0x66, 0x49, 0xd93f, 0xdffd, 0 };

    UVersionInfo unicodeVersion={ 0, 0, 17, 89 }, unicode_3_1={ 3, 1, 0, 0 };

    int32_t result, lenMixed, lenOtherDefault, lenOtherExcludeSpecialI, lenDifferent;
    UErrorCode errorCode;
    UBool isUnicode_3_1;

    errorCode=U_ZERO_ERROR;

    lenMixed=u_strlen(mixed);
    lenOtherDefault=u_strlen(otherDefault);
    (void)lenOtherDefault;    
    lenOtherExcludeSpecialI=u_strlen(otherExcludeSpecialI);
    lenDifferent=u_strlen(different);

    
    u_getUnicodeVersion(unicodeVersion);
    isUnicode_3_1= uprv_memcmp(unicodeVersion, unicode_3_1, 4)>=0;
    (void)isUnicode_3_1;    

    
    result=u_strcasecmp(mixed, otherDefault, U_FOLD_CASE_DEFAULT);
    if(result!=0) {
        log_err("error: u_strcasecmp(mixed, other, default)=%ld instead of 0\n", result);
    }
    result=u_strCaseCompare(mixed, -1, otherDefault, -1, U_FOLD_CASE_DEFAULT, &errorCode);
    if(result!=0) {
        log_err("error: u_strCaseCompare(mixed, other, default)=%ld instead of 0\n", result);
    }

    
    result=u_strcasecmp(mixed, otherExcludeSpecialI, U_FOLD_CASE_EXCLUDE_SPECIAL_I);
    if(result!=0) {
        log_err("error: u_strcasecmp(mixed, other, exclude special i)=%ld instead of 0\n", result);
    }
    result=u_strCaseCompare(mixed, lenMixed, otherExcludeSpecialI, lenOtherExcludeSpecialI, U_FOLD_CASE_EXCLUDE_SPECIAL_I, &errorCode);
    if(result!=0) {
        log_err("error: u_strCaseCompare(mixed, other, exclude special i)=%ld instead of 0\n", result);
    }

    
    result=u_strcasecmp(mixed, different, U_FOLD_CASE_DEFAULT);
    if(result<=0) {
        log_err("error: u_strcasecmp(mixed, different, default)=%ld instead of positive\n", result);
    }
    result=u_strCaseCompare(mixed, -1, different, lenDifferent, U_FOLD_CASE_DEFAULT, &errorCode);
    if(result<=0) {
        log_err("error: u_strCaseCompare(mixed, different, default)=%ld instead of positive\n", result);
    }

    
    result=u_strncasecmp(mixed, different, 4, U_FOLD_CASE_DEFAULT);
    if(result!=0) {
        log_err("error: u_strncasecmp(mixed, different, 4, default)=%ld instead of 0\n", result);
    }
    result=u_strCaseCompare(mixed, 4, different, 4, U_FOLD_CASE_DEFAULT, &errorCode);
    if(result!=0) {
        log_err("error: u_strCaseCompare(mixed, 4, different, 4, default)=%ld instead of 0\n", result);
    }

    
    result=u_strncasecmp(mixed, different, 5, U_FOLD_CASE_DEFAULT);
    if(result<=0) {
        log_err("error: u_strncasecmp(mixed, different, 5, default)=%ld instead of positive\n", result);
    }
    result=u_strCaseCompare(mixed, 5, different, 5, U_FOLD_CASE_DEFAULT, &errorCode);
    if(result<=0) {
        log_err("error: u_strCaseCompare(mixed, 5, different, 5, default)=%ld instead of positive\n", result);
    }

    
    result=u_memcasecmp(mixed, different, 4, U_FOLD_CASE_DEFAULT);
    if(result!=0) {
        log_err("error: u_memcasecmp(mixed, different, 4, default)=%ld instead of 0\n", result);
    }

    
    result=u_memcasecmp(mixed, different, 5, U_FOLD_CASE_DEFAULT);
    if(result<=0) {
        log_err("error: u_memcasecmp(mixed, different, 5, default)=%ld instead of positive\n", result);
    }
}








static void
TestUCaseMap(void) {
    static const char
        aBc[] ={ 0x61, 0x42, 0x63, 0 },
        abc[] ={ 0x61, 0x62, 0x63, 0 },
        ABCg[]={ 0x41, 0x42, 0x43, 0x67, 0 },
        defg[]={ 0x64, 0x65, 0x66, 0x67, 0 };
    char utf8Out[8];

    UCaseMap *csm;
    const char *locale;
    uint32_t options;
    int32_t length;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    csm=ucasemap_open("tur", 0xa5, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ucasemap_open(\"tur\") failed - %s\n", u_errorName(errorCode));
        return;
    }
    locale=ucasemap_getLocale(csm);
    if(0!=strcmp(locale, "tr")) {
        log_err("ucasemap_getLocale(ucasemap_open(\"tur\"))==%s!=\"tr\"\n", locale);
    }
    
    ucasemap_setLocale(csm, "I-kLInGOn-the-quick-brown-fox-jumps-over-the-lazy-dog", &errorCode);
    locale=ucasemap_getLocale(csm);
    if(0!=strcmp(locale, "i-klingon")) {
        log_err("ucasemap_getLocale(ucasemap_setLocale(\"I-kLInGOn-the-quick-br...\"))==%s!=\"i-klingon\"\n", locale);
    }

    errorCode=U_ZERO_ERROR;
    options=ucasemap_getOptions(csm);
    if(options!=0xa5) {
        log_err("ucasemap_getOptions(ucasemap_open(0xa5))==0x%lx!=0xa5\n", (long)options);
    }
    ucasemap_setOptions(csm, 0x333333, &errorCode);
    options=ucasemap_getOptions(csm);
    if(options!=0x333333) {
        log_err("ucasemap_getOptions(ucasemap_setOptions(0x333333))==0x%lx!=0x333333\n", (long)options);
    }

    

    
    errorCode=U_ZERO_ERROR;
    length=ucasemap_utf8ToLower(csm, utf8Out, (int32_t)sizeof(utf8Out), aBc, -1, &errorCode);
    if(U_FAILURE(errorCode) || length!=3 || 0!=strcmp(abc, utf8Out)) {
        log_err("ucasemap_utf8ToLower(aBc\\0) failed\n");
    }

    
    errorCode=U_PARSE_ERROR;
    strcpy(utf8Out, defg);
    length=ucasemap_utf8ToLower(csm, utf8Out, (int32_t)sizeof(utf8Out), aBc, -1, &errorCode);
    if(errorCode!=U_PARSE_ERROR || 0!=strcmp(defg, utf8Out)) {
        log_err("ucasemap_utf8ToLower(failure) failed\n");
    }

    
    errorCode=U_ZERO_ERROR;
    strcpy(utf8Out, aBc);
    length=ucasemap_utf8ToUpper(csm, utf8Out, 2, utf8Out+1, 2, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || 0!=strcmp(aBc, utf8Out)) {
        log_err("ucasemap_utf8ToUpper(overlap 1) failed\n");
    }

    
    errorCode=U_ZERO_ERROR;
    strcpy(utf8Out, aBc);
    length=ucasemap_utf8ToUpper(csm, utf8Out+1, 2, utf8Out, 2, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || 0!=strcmp(aBc, utf8Out)) {
        log_err("ucasemap_utf8ToUpper(overlap 2) failed\n");
    }

    
    errorCode=U_ZERO_ERROR;
    strcpy(utf8Out, defg);
    length=ucasemap_utf8ToLower(csm, NULL, (int32_t)sizeof(utf8Out), aBc, -1, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || 0!=strcmp(defg, utf8Out)) {
        log_err("ucasemap_utf8ToLower(dest=NULL) failed\n");
    }

    
    errorCode=U_ZERO_ERROR;
    strcpy(utf8Out, defg);
    length=ucasemap_utf8ToLower(csm, utf8Out, -2, aBc, -1, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || 0!=strcmp(defg, utf8Out)) {
        log_err("ucasemap_utf8ToLower(destCapacity<0) failed\n");
    }

    
    errorCode=U_ZERO_ERROR;
    strcpy(utf8Out, defg);
    length=ucasemap_utf8ToLower(csm, utf8Out, (int32_t)sizeof(utf8Out), NULL, -1, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || 0!=strcmp(defg, utf8Out)) {
        log_err("ucasemap_utf8ToLower(src=NULL) failed\n");
    }

    
    errorCode=U_ZERO_ERROR;
    strcpy(utf8Out, defg);
    length=ucasemap_utf8ToLower(csm, utf8Out, (int32_t)sizeof(utf8Out), aBc, -2, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || 0!=strcmp(defg, utf8Out)) {
        log_err("ucasemap_utf8ToLower(srcLength<-1) failed\n");
    }

    
    errorCode=U_ZERO_ERROR;
    strcpy(utf8Out, defg);
    length=ucasemap_utf8ToUpper(csm, utf8Out, 2, aBc, 3, &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || length!=3 || 0!=strcmp(defg+2, utf8Out+2)) {
        log_err("ucasemap_utf8ToUpper(overflow) failed\n");
    }

    
    errorCode=U_ZERO_ERROR;
    strcpy(utf8Out, defg);
    length=ucasemap_utf8ToUpper(csm, utf8Out, 3, aBc, 3, &errorCode);
    if(errorCode!=U_STRING_NOT_TERMINATED_WARNING || length!=3 || 0!=strcmp(ABCg, utf8Out)) {
        log_err("ucasemap_utf8ToUpper(overflow) failed\n");
    }

    
    errorCode=U_ZERO_ERROR;
    utf8Out[0]=0;
    length=ucasemap_utf8FoldCase(csm, utf8Out, (int32_t)sizeof(utf8Out), aBc, 3, &errorCode);
    if(U_FAILURE(errorCode) || length!=3 || 0!=strcmp(abc, utf8Out)) {
        log_err("ucasemap_utf8FoldCase(aBc) failed\n");
    }

    ucasemap_close(csm);
}

#if !UCONFIG_NO_BREAK_ITERATION


static void
TestUCaseMapToTitle(void) {
    
    




    static const UChar

    beforeTitle[]=      { 0x61, 0x20, 0x2bb, 0x43, 0x61, 0x54, 0x2e, 0x20, 0x41, 0x20, 0x2bb, 0x64, 0x4f, 0x67, 0x21, 0x20, 0x2bb, 0x65, 0x54, 0x63, 0x2e },
    titleWord[]=        { 0x41, 0x20, 0x2bb, 0x43, 0x61, 0x74, 0x2e, 0x20, 0x41, 0x20, 0x2bb, 0x44, 0x6f, 0x67, 0x21, 0x20, 0x2bb, 0x45, 0x74, 0x63, 0x2e },
    titleWordNoAdjust[]={ 0x41, 0x20, 0x2bb, 0x63, 0x61, 0x74, 0x2e, 0x20, 0x41, 0x20, 0x2bb, 0x64, 0x6f, 0x67, 0x21, 0x20, 0x2bb, 0x65, 0x74, 0x63, 0x2e },
    titleSentNoLower[]= { 0x41, 0x20, 0x2bb, 0x43, 0x61, 0x54, 0x2e, 0x20, 0x41, 0x20, 0x2bb, 0x64, 0x4f, 0x67, 0x21, 0x20, 0x2bb, 0x45, 0x54, 0x63, 0x2e };

    UChar buffer[32];
    UCaseMap *csm;
    UBreakIterator *sentenceIter;
    const UBreakIterator *iter;
    int32_t length;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    csm=ucasemap_open("", 0, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ucasemap_open(\"\") failed - %s\n", u_errorName(errorCode));
        return;
    }

    iter=ucasemap_getBreakIterator(csm);
    if(iter!=NULL) {
        log_err("ucasemap_getBreakIterator() returns %p!=NULL before setting any iterator or titlecasing\n", iter);
    }

    
    length=ucasemap_toTitle(csm, buffer, UPRV_LENGTHOF(buffer), beforeTitle, UPRV_LENGTHOF(beforeTitle), &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=UPRV_LENGTHOF(titleWord) ||
        0!=u_memcmp(buffer, titleWord, length) ||
        buffer[length]!=0
    ) {
        log_err_status(errorCode, "ucasemap_toTitle(default iterator)=%ld failed - %s\n", (long)length, u_errorName(errorCode));
    }
    if (U_SUCCESS(errorCode)) {
        iter=ucasemap_getBreakIterator(csm);
        if(iter==NULL) {
            log_err("ucasemap_getBreakIterator() returns NULL after titlecasing\n");
        }
    }

    
    ucasemap_setOptions(csm, U_TITLECASE_NO_BREAK_ADJUSTMENT, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err_status(errorCode, "error: ucasemap_setOptions(U_TITLECASE_NO_BREAK_ADJUSTMENT) failed - %s\n", u_errorName(errorCode));
        return;
    }

    length=ucasemap_toTitle(csm, buffer, UPRV_LENGTHOF(buffer), beforeTitle, UPRV_LENGTHOF(beforeTitle), &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=UPRV_LENGTHOF(titleWordNoAdjust) ||
        0!=u_memcmp(buffer, titleWordNoAdjust, length) ||
        buffer[length]!=0
    ) {
        log_err("ucasemap_toTitle(default iterator, no break adjustment)=%ld failed - %s\n", (long)length, u_errorName(errorCode));
    }

    
    errorCode=U_ZERO_ERROR;
    sentenceIter=ubrk_open(UBRK_SENTENCE, "", NULL, 0, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: ubrk_open(UBRK_SENTENCE) failed - %s\n", u_errorName(errorCode));
        ucasemap_close(csm);
        return;
    }
    ucasemap_setBreakIterator(csm, sentenceIter, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: ucasemap_setBreakIterator(sentence iterator) failed - %s\n", u_errorName(errorCode));
        ubrk_close(sentenceIter);
        ucasemap_close(csm);
        return;
    }
    iter=ucasemap_getBreakIterator(csm);
    if(iter!=sentenceIter) {
        log_err("ucasemap_getBreakIterator() returns %p!=%p after setting the iterator\n", iter, sentenceIter);
    }

    ucasemap_setOptions(csm, U_TITLECASE_NO_LOWERCASE, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: ucasemap_setOptions(U_TITLECASE_NO_LOWERCASE) failed - %s\n", u_errorName(errorCode));
        return;
    }

    
    length=ucasemap_toTitle(csm, NULL, 0, beforeTitle, UPRV_LENGTHOF(beforeTitle), &errorCode);
    if( errorCode!=U_BUFFER_OVERFLOW_ERROR ||
        length!=UPRV_LENGTHOF(titleSentNoLower)
    ) {
        log_err("ucasemap_toTitle(preflight sentence break iterator, no lowercasing)=%ld failed - %s\n", (long)length, u_errorName(errorCode));
    }

    errorCode=U_ZERO_ERROR;
    buffer[0]=0;
    length=ucasemap_toTitle(csm, buffer, UPRV_LENGTHOF(buffer), beforeTitle, UPRV_LENGTHOF(beforeTitle), &errorCode);
    if( U_FAILURE(errorCode) ||
        length!=UPRV_LENGTHOF(titleSentNoLower) ||
        0!=u_memcmp(buffer, titleSentNoLower, length) ||
        buffer[length]!=0
    ) {
        log_err("ucasemap_toTitle(sentence break iterator, no lowercasing)=%ld failed - %s\n", (long)length, u_errorName(errorCode));
    }

    
    {
        char utf8BeforeTitle[64], utf8TitleSentNoLower[64], utf8[64];
        int32_t utf8BeforeTitleLength, utf8TitleSentNoLowerLength;

        errorCode=U_ZERO_ERROR;
        u_strToUTF8(utf8BeforeTitle, (int32_t)sizeof(utf8BeforeTitle), &utf8BeforeTitleLength, beforeTitle, UPRV_LENGTHOF(beforeTitle), &errorCode);
        u_strToUTF8(utf8TitleSentNoLower, (int32_t)sizeof(utf8TitleSentNoLower), &utf8TitleSentNoLowerLength, titleSentNoLower, UPRV_LENGTHOF(titleSentNoLower), &errorCode);

        length=ucasemap_utf8ToTitle(csm, utf8, (int32_t)sizeof(utf8), utf8BeforeTitle, utf8BeforeTitleLength, &errorCode);
        if( U_FAILURE(errorCode) ||
            length!=utf8TitleSentNoLowerLength ||
            0!=uprv_memcmp(utf8, utf8TitleSentNoLower, length) ||
            utf8[length]!=0
        ) {
            log_err("ucasemap_utf8ToTitle(sentence break iterator, no lowercasing)=%ld failed - %s\n", (long)length, u_errorName(errorCode));
        }
    }

    ucasemap_close(csm);
}

#endif


static void
TestUCaseInsensitivePrefixMatch(void) {
    struct {
        const char     *s1;
        const char     *s2;
        int32_t         r1;
        int32_t         r2;
    } testCases[] = {
        {"ABC", "ab", 2, 2},
        {"ABCD", "abcx", 3, 3},
        {"ABC", "xyz", 0, 0},
        
        {"A\\u00dfBC", "Ass", 2, 3},
        {"Fust", "Fu\\u00dfball", 2, 2},
        {"\\u00dfsA", "s\\u00dfB", 2, 2},
        {"\\u00dfs", "s\\u00df", 2, 2},
        
        {"XYZ\\u0130i\\u0307xxx", "xyzi\\u0307\\u0130yyy", 6, 6},
        {0, 0, 0, 0}
    };
    int32_t i;

    for (i = 0; testCases[i].s1 != 0; i++) {
        UErrorCode sts = U_ZERO_ERROR;
        UChar u1[64], u2[64];
        int32_t matchLen1, matchLen2;

        u_unescape(testCases[i].s1, u1, 64);
        u_unescape(testCases[i].s2, u2, 64);

        u_caseInsensitivePrefixMatch(u1, -1, u2, -1, 0, &matchLen1, &matchLen2, &sts);
        if (U_FAILURE(sts)) {
            log_err("error: %s, s1=%s, s2=%s", u_errorName(sts), testCases[i].s1, testCases[i].s2);
        } else if (matchLen1 != testCases[i].r1 || matchLen2 != testCases[i].r2) {
            log_err("s1=%s, s2=%2 / match len1=%d, len2=%d / expected len1=%d, len2=%d",
                testCases[i].s1, testCases[i].s2,
                matchLen1, matchLen2,
                testCases[i].r1, testCases[i].r2);
        }
    }
}

void addCaseTest(TestNode** root);

void addCaseTest(TestNode** root) {
    
    addTest(root, &TestCaseLower, "tsutil/cstrcase/TestCaseLower");
    addTest(root, &TestCaseUpper, "tsutil/cstrcase/TestCaseUpper");
#if !UCONFIG_NO_BREAK_ITERATION && !UCONFIG_NO_FILE_IO
    addTest(root, &TestCaseTitle, "tsutil/cstrcase/TestCaseTitle");
    addTest(root, &TestCaseDutchTitle, "tsutil/cstrcase/TestCaseDutchTitle");
#endif
    addTest(root, &TestCaseFolding, "tsutil/cstrcase/TestCaseFolding");
    addTest(root, &TestCaseCompare, "tsutil/cstrcase/TestCaseCompare");
    addTest(root, &TestUCaseMap, "tsutil/cstrcase/TestUCaseMap");
#if !UCONFIG_NO_BREAK_ITERATION && !UCONFIG_NO_FILE_IO
    addTest(root, &TestUCaseMapToTitle, "tsutil/cstrcase/TestUCaseMapToTitle");
#endif
    addTest(root, &TestUCaseInsensitivePrefixMatch, "tsutil/cstrcase/TestUCaseInsensitivePrefixMatch");
}
