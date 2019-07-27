





#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/numfmt.h"
#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/locid.h"
#include "unicode/uclean.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void capi();
void cppapi();

static void
showCurrencyFormatting(UBool useICU26API);

int main(int argc, char **argv) {
    printf("%s output is in UTF-8\n", argv[0]);

    printf("C++ API\n");
    cppapi();

    printf("C API\n");
    capi();

    showCurrencyFormatting(FALSE);
    showCurrencyFormatting(TRUE);

    u_cleanup();    

    printf("Exiting successfully\n");
    return 0;
}




void cppapi() {
    Locale us("en", "US");
    UErrorCode status = U_ZERO_ERROR;
    
    
    NumberFormat *fmt = NumberFormat::createInstance(us, status);
    check(status, "NumberFormat::createInstance");

    
    
    UnicodeString str("9876543210.123");
    Formattable result;
    fmt->parse(str, result, status);
    check(status, "NumberFormat::parse");

    printf("NumberFormat::parse(\""); 
    uprintf(str);
    printf("\") => ");
    uprintf(formattableToString(result));
    printf("\n");

    
    
    str.remove(); 
    fmt->format(result, str, status);
    check(status, "NumberFormat::format");

    printf("NumberFormat::format("); 
    uprintf(formattableToString(result));
    printf(") => \"");
    uprintf(str);
    printf("\"\n");

    delete fmt; 
    
}


















static void
setNumberFormatCurrency_2_4(NumberFormat &nf, const char *currency, UErrorCode &errorCode) {
    
    if(U_FAILURE(errorCode)) {
        return;
    }
    if(currency==NULL || strlen(currency)!=3) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    
    
    
    DecimalFormat *dnf=dynamic_cast<DecimalFormat *>(&nf);
    if(dnf==NULL) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    
    
    
    
    
    static const struct {
        
        const char *currency;

        
        
        int32_t fractionDigits;

        







        double roundingIncrement;

        
        UChar symbol[16];
    } currencyMap[]={
        { "USD", 2, 0.0, { 0x24, 0 } },
        { "GBP", 2, 0.0, { 0xa3, 0 } },
        { "EUR", 2, 0.0, { 0x20ac, 0 } },
        { "JPY", 0, 0.0, { 0xa5, 0 } }
    };

    int32_t i;

    for(i=0; i<UPRV_LENGTHOF(currencyMap); ++i) {
        if(strcmp(currency, currencyMap[i].currency)==0) {
            break;
        }
    }
    if(i==UPRV_LENGTHOF(currencyMap)) {
        
        errorCode=U_UNSUPPORTED_ERROR;
        return;
    }

    

    nf.setMinimumFractionDigits(currencyMap[i].fractionDigits);
    nf.setMaximumFractionDigits(currencyMap[i].fractionDigits);

    dnf->setRoundingIncrement(currencyMap[i].roundingIncrement);

    DecimalFormatSymbols symbols(*dnf->getDecimalFormatSymbols());
    symbols.setSymbol(DecimalFormatSymbols::kCurrencySymbol, currencyMap[i].symbol);
    dnf->setDecimalFormatSymbols(symbols); 
}













static void
setNumberFormatCurrency_2_6(NumberFormat &nf, const char *currency, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    if(currency==NULL || strlen(currency)!=3) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    
    UChar uCurrency[4];
    u_charsToUChars(currency, uCurrency, 4);

    
    
#if (U_ICU_VERSION_MAJOR_NUM < 3)
    nf.setCurrency(uCurrency);
#else
    nf.setCurrency(uCurrency, errorCode);
#endif
}

static const char *const
sampleLocaleIDs[]={
    
    
    "en_US", "en_GB", "de_DE", "ja_JP", "fr_FR", "hi_IN"
};

static const char *const
sampleCurrencies[]={
    "USD", "GBP", "EUR", "JPY"
};

static void
showCurrencyFormatting(UBool useICU26API) {
    NumberFormat *nf;
    int32_t i, j;

    UnicodeString output;

    UErrorCode errorCode;

    
    

    for(i=0; i<UPRV_LENGTHOF(sampleLocaleIDs); ++i) {
        printf("show currency formatting (method for %s) in the locale \"%s\"\n",
                useICU26API ? "ICU 2.6" : "before ICU 2.6",
                sampleLocaleIDs[i]);

        
        errorCode=U_ZERO_ERROR;
        nf=NumberFormat::createCurrencyInstance(sampleLocaleIDs[i], errorCode);
        if(U_FAILURE(errorCode)) {
            printf("NumberFormat::createCurrencyInstance(%s) failed - %s\n",
                    sampleLocaleIDs[i], u_errorName(errorCode));
            continue;
        }

        for(j=0; j<UPRV_LENGTHOF(sampleCurrencies); ++j) {
            printf("  - format currency \"%s\": ", sampleCurrencies[j]);

            
            if(useICU26API) {
                setNumberFormatCurrency_2_6(*nf, sampleCurrencies[j], errorCode);
            } else {
                setNumberFormatCurrency_2_4(*nf, sampleCurrencies[j], errorCode);
            }
            if(U_FAILURE(errorCode)) {
                printf("setNumberFormatCurrency(%s) failed - %s\n",
                        sampleCurrencies[j], u_errorName(errorCode));
                continue;
            }

            
            output.remove();
            nf->format(12345678.93, output);
            output+=(UChar)0x0a; 
            uprintf(output);
        }
    }
}
