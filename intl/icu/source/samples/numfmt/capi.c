





#include "unicode/unum.h"
#include "unicode/ustring.h"
#include <stdio.h>
#include <stdlib.h>

static void uprintf(const UChar* str) {
    char buf[256];
    u_austrcpy(buf, str);
    printf("%s", buf);
}

void capi() {
    UNumberFormat *fmt;
    UErrorCode status = U_ZERO_ERROR;
    
    UChar str[] = { 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33,
                    0x32, 0x31, 0x30, 0x2E, 0x31, 0x32, 0x33, 0 };
    UChar buf[256];
    int32_t needed;
    double a;
    
    
    fmt = unum_open(
          UNUM_DECIMAL,      
          0,                 
          0,                 
          "en_US",           
          0,                 
          &status);
    if (U_FAILURE(status)) {
        printf("FAIL: unum_open\n");
        exit(1);
    }

    




    a = unum_parseDouble(fmt, str, u_strlen(str), NULL, &status);
    if (U_FAILURE(status)) {
        printf("FAIL: unum_parseDouble\n");
        exit(1);
    }

    
    printf("unum_parseDouble(\"");
    uprintf(str);
    printf("\") => %g\n", a);

    






    needed = unum_formatDouble(fmt, a, buf, 256, NULL, &status);
    if (U_FAILURE(status)) {
        printf("FAIL: format_parseDouble\n");
        exit(1);
    }

    
    printf("unum_formatDouble(%g) => \"", a);
    uprintf(buf);
    printf("\"\n");

    
    unum_close(fmt);
}
