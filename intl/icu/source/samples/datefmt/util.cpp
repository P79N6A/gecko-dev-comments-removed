





#include "unicode/unistr.h"
#include <stdio.h>
#include <stdlib.h>


void check(UErrorCode& status, const char* msg) {
    if (U_FAILURE(status)) {
        printf("ERROR: %s (%s)\n", u_errorName(status), msg);
        exit(1);
    }
    
}
                                                      

static UnicodeString& appendHex(uint32_t number, 
                         int8_t digits, 
                         UnicodeString& target) {
    static const UnicodeString DIGIT_STRING("0123456789ABCDEF");
    while (digits > 0) {
        target += DIGIT_STRING[(number >> ((--digits) * 4)) & 0xF];
    }
    return target;
}


UnicodeString escape(const UnicodeString &source) {
    int32_t i;
    UnicodeString target;
    target += "\"";
    for (i=0; i<source.length(); ++i) {
        UChar ch = source[i];
        if (ch < 0x09 || (ch > 0x0A && ch < 0x20) || ch > 0x7E) {
            target += "\\u";
            appendHex(ch, 4, target);
        } else {
            target += ch;
        }
    }
    target += "\"";
    return target;
}


void uprintf(const UnicodeString &str) {
    char *buf = 0;
    int32_t len = str.length();
    
    

    int32_t bufLen = len + 16;
	int32_t actualLen;
    buf = new char[bufLen + 1];
    actualLen = str.extract(0, len, buf); 
    buf[actualLen] = 0;
    printf("%s", buf);
    delete[] buf;
}
