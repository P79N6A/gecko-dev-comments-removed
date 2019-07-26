


















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/dcfmtsym.h"
#include "unicode/ures.h"
#include "unicode/decimfmt.h"
#include "unicode/ucurr.h"
#include "unicode/choicfmt.h"
#include "unicode/unistr.h"
#include "unicode/numsys.h"
#include "unicode/unum.h"
#include "ucurrimp.h"
#include "cstring.h"
#include "locbased.h"
#include "uresimp.h"
#include "ureslocs.h"





U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DecimalFormatSymbols)

static const char gNumberElements[] = "NumberElements";
static const char gCurrencySpacingTag[] = "currencySpacing";
static const char gBeforeCurrencyTag[] = "beforeCurrency";
static const char gAfterCurrencyTag[] = "afterCurrency";
static const char gCurrencyMatchTag[] = "currencyMatch";
static const char gCurrencySudMatchTag[] = "surroundingMatch";
static const char gCurrencyInsertBtnTag[] = "insertBetween";


static const UChar INTL_CURRENCY_SYMBOL_STR[] = {0xa4, 0xa4, 0};




DecimalFormatSymbols::DecimalFormatSymbols(UErrorCode& status)
    : UObject(),
    locale()
{
    initialize(locale, status, TRUE);
}




DecimalFormatSymbols::DecimalFormatSymbols(const Locale& loc, UErrorCode& status)
    : UObject(),
    locale(loc)
{
    initialize(locale, status);
}



DecimalFormatSymbols::~DecimalFormatSymbols()
{
}




DecimalFormatSymbols::DecimalFormatSymbols(const DecimalFormatSymbols &source)
    : UObject(source)
{
    *this = source;
}




DecimalFormatSymbols&
DecimalFormatSymbols::operator=(const DecimalFormatSymbols& rhs)
{
    if (this != &rhs) {
        for(int32_t i = 0; i < (int32_t)kFormatSymbolCount; ++i) {
            
            fSymbols[(ENumberFormatSymbol)i].fastCopyFrom(rhs.fSymbols[(ENumberFormatSymbol)i]);
        }
        for(int32_t i = 0; i < (int32_t)UNUM_CURRENCY_SPACING_COUNT; ++i) {
            currencySpcBeforeSym[i].fastCopyFrom(rhs.currencySpcBeforeSym[i]);
            currencySpcAfterSym[i].fastCopyFrom(rhs.currencySpcAfterSym[i]);
        }
        locale = rhs.locale;
        uprv_strcpy(validLocale, rhs.validLocale);
        uprv_strcpy(actualLocale, rhs.actualLocale);
    }
    return *this;
}



UBool
DecimalFormatSymbols::operator==(const DecimalFormatSymbols& that) const
{
    if (this == &that) {
        return TRUE;
    }
    for(int32_t i = 0; i < (int32_t)kFormatSymbolCount; ++i) {
        if(fSymbols[(ENumberFormatSymbol)i] != that.fSymbols[(ENumberFormatSymbol)i]) {
            return FALSE;
        }
    }
    for(int32_t i = 0; i < (int32_t)UNUM_CURRENCY_SPACING_COUNT; ++i) {
        if(currencySpcBeforeSym[i] != that.currencySpcBeforeSym[i]) {
            return FALSE;
        }
        if(currencySpcAfterSym[i] != that.currencySpcAfterSym[i]) {
            return FALSE;
        }
    }
    return locale == that.locale &&
        uprv_strcmp(validLocale, that.validLocale) == 0 &&
        uprv_strcmp(actualLocale, that.actualLocale) == 0;
}



void
DecimalFormatSymbols::initialize(const Locale& loc, UErrorCode& status, UBool useLastResortData)
{
    static const char *gNumberElementKeys[kFormatSymbolCount] = {
        "decimal",
        "group",
        "list",
        "percentSign",
        NULL, 
        NULL, 
        "minusSign",
        "plusSign",
        NULL, 
        NULL, 
        "currencyDecimal",
        "exponential",
        "perMille",
        NULL, 
        "infinity",
        "nan",
        NULL, 
        "currencyGroup",
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
    };

    static const char *gLatn =  "latn";
    static const char *gSymbols = "symbols";
    const char *nsName;
    const UChar *sym = NULL;
    int32_t len = 0;

    *validLocale = *actualLocale = 0;
    currPattern = NULL;
    if (U_FAILURE(status))
        return;

    const char* locStr = loc.getName();
    UResourceBundle *resource = ures_open((char *)0, locStr, &status);
    UResourceBundle *numberElementsRes = ures_getByKeyWithFallback(resource, gNumberElements, NULL, &status);

    if (U_FAILURE(status)) {
        if ( useLastResortData ) {
            status = U_USING_DEFAULT_WARNING;
            initialize();
        }
        return;
    } else {

        
        initialize();

        
        
        
        

        NumberingSystem* ns = NumberingSystem::createInstance(loc,status);
        if (U_SUCCESS(status) && ns->getRadix() == 10 && !ns->isAlgorithmic()) {
            nsName = ns->getName();
            UnicodeString digitString(ns->getDescription());
            setSymbol(kZeroDigitSymbol,  digitString.tempSubString(0, 1), FALSE);
            setSymbol(kOneDigitSymbol,   digitString.tempSubString(1, 1), FALSE);
            setSymbol(kTwoDigitSymbol,   digitString.tempSubString(2, 1), FALSE);
            setSymbol(kThreeDigitSymbol, digitString.tempSubString(3, 1), FALSE);
            setSymbol(kFourDigitSymbol,  digitString.tempSubString(4, 1), FALSE);
            setSymbol(kFiveDigitSymbol,  digitString.tempSubString(5, 1), FALSE);
            setSymbol(kSixDigitSymbol,   digitString.tempSubString(6, 1), FALSE);
            setSymbol(kSevenDigitSymbol, digitString.tempSubString(7, 1), FALSE);
            setSymbol(kEightDigitSymbol, digitString.tempSubString(8, 1), FALSE);
            setSymbol(kNineDigitSymbol,  digitString.tempSubString(9, 1), FALSE);
        } else {
            nsName = gLatn;
        }
       
        UBool isLatn = !uprv_strcmp(nsName,gLatn);

        UErrorCode nlStatus = U_ZERO_ERROR;
        UResourceBundle *nonLatnSymbols = NULL;
        if ( !isLatn ) {
            nonLatnSymbols = ures_getByKeyWithFallback(numberElementsRes, nsName, NULL, &nlStatus);
            nonLatnSymbols = ures_getByKeyWithFallback(nonLatnSymbols, gSymbols, nonLatnSymbols, &nlStatus);
        }

        UResourceBundle *latnSymbols = ures_getByKeyWithFallback(numberElementsRes, gLatn, NULL, &status);
        latnSymbols = ures_getByKeyWithFallback(latnSymbols, gSymbols, latnSymbols, &status);

        UBool kMonetaryDecimalSet = FALSE;
        UBool kMonetaryGroupingSet = FALSE;
        for(int32_t i = 0; i<kFormatSymbolCount; i++) {
            if ( gNumberElementKeys[i] != NULL ) {
                UErrorCode localStatus = U_ZERO_ERROR;
                if ( !isLatn ) {
                    sym = ures_getStringByKeyWithFallback(nonLatnSymbols,gNumberElementKeys[i],&len,&localStatus);
                    
                    
                    if ( U_FAILURE(localStatus) ) {
                        localStatus = U_ZERO_ERROR;
                        sym = ures_getStringByKeyWithFallback(latnSymbols,gNumberElementKeys[i],&len,&localStatus);
                    }
                } else {
                        sym = ures_getStringByKeyWithFallback(latnSymbols,gNumberElementKeys[i],&len,&localStatus);
                }

                if ( U_SUCCESS(localStatus) ) {
                    setSymbol((ENumberFormatSymbol)i, UnicodeString(TRUE, sym, len));
                    if ( i == kMonetarySeparatorSymbol ) {
                        kMonetaryDecimalSet = TRUE;
                    } else if ( i == kMonetaryGroupingSeparatorSymbol ) {
                        kMonetaryGroupingSet = TRUE;
                    }
                }
            }
        }

        ures_close(latnSymbols);
        if ( !isLatn ) {
            ures_close(nonLatnSymbols);
        }

        
        

        if ( !kMonetaryDecimalSet ) {
            setSymbol(kMonetarySeparatorSymbol,fSymbols[kDecimalSeparatorSymbol]);
        }
        if ( !kMonetaryGroupingSet ) {
            setSymbol(kMonetaryGroupingSeparatorSymbol,fSymbols[kGroupingSeparatorSymbol]);
        }

        if (ns) {
            delete ns;
        }

        
        
        
        UErrorCode internalStatus = U_ZERO_ERROR; 
        UChar curriso[4];
        UnicodeString tempStr;
        ucurr_forLocale(locStr, curriso, 4, &internalStatus);

        
        uprv_getStaticCurrencyName(curriso, locStr, tempStr, internalStatus);
        if (U_SUCCESS(internalStatus)) {
            fSymbols[kIntlCurrencySymbol].setTo(curriso, -1);
            fSymbols[kCurrencySymbol] = tempStr;
        }
        

        U_LOCALE_BASED(locBased, *this);
        locBased.setLocaleIDs(ures_getLocaleByType(numberElementsRes,
                              ULOC_VALID_LOCALE, &status),
                              ures_getLocaleByType(numberElementsRes,
                              ULOC_ACTUAL_LOCALE, &status));
        
        
        UChar ucc[4]={0}; 
        int32_t uccLen = 4;
        const char* locName = loc.getName();
        UErrorCode localStatus = U_ZERO_ERROR;
        uccLen = ucurr_forLocale(locName, ucc, uccLen, &localStatus);

        if(U_SUCCESS(localStatus) && uccLen > 0) {
            char cc[4]={0};
            u_UCharsToChars(ucc, cc, uccLen);
            
            UResourceBundle *currencyResource = ures_open(U_ICUDATA_CURR, locStr, &localStatus);
            UResourceBundle *currency = ures_getByKeyWithFallback(currencyResource, "Currencies", NULL, &localStatus);
            currency = ures_getByKeyWithFallback(currency, cc, currency, &localStatus);
            if(U_SUCCESS(localStatus) && ures_getSize(currency)>2) { 
                currency = ures_getByIndex(currency, 2, currency, &localStatus);
                int32_t currPatternLen = 0;
                currPattern = ures_getStringByIndex(currency, (int32_t)0, &currPatternLen, &localStatus);
                UnicodeString decimalSep = ures_getUnicodeStringByIndex(currency, (int32_t)1, &localStatus);
                UnicodeString groupingSep = ures_getUnicodeStringByIndex(currency, (int32_t)2, &localStatus);
                if(U_SUCCESS(localStatus)){
                    fSymbols[kMonetaryGroupingSeparatorSymbol] = groupingSep;
                    fSymbols[kMonetarySeparatorSymbol] = decimalSep;
                    
                    status = localStatus;
                }
            }
            ures_close(currency);
            ures_close(currencyResource);
            
            
        }
            

        
        localStatus = U_ZERO_ERROR;
        UResourceBundle *currencyResource = ures_open(U_ICUDATA_CURR, locStr, &localStatus);
        UResourceBundle *currencySpcRes = ures_getByKeyWithFallback(currencyResource,
                                           gCurrencySpacingTag, NULL, &localStatus);

        if (localStatus == U_USING_FALLBACK_WARNING || U_SUCCESS(localStatus)) {
            const char* keywords[UNUM_CURRENCY_SPACING_COUNT] = {
                gCurrencyMatchTag, gCurrencySudMatchTag, gCurrencyInsertBtnTag
            };
            localStatus = U_ZERO_ERROR;
            UResourceBundle *dataRes = ures_getByKeyWithFallback(currencySpcRes,
                                       gBeforeCurrencyTag, NULL, &localStatus);
            if (localStatus == U_USING_FALLBACK_WARNING || U_SUCCESS(localStatus)) {
                localStatus = U_ZERO_ERROR;
                for (int32_t i = 0; i < UNUM_CURRENCY_SPACING_COUNT; i++) {
                  currencySpcBeforeSym[i] = ures_getUnicodeStringByKey(dataRes, keywords[i], &localStatus);
                }
                ures_close(dataRes);
            }
            dataRes = ures_getByKeyWithFallback(currencySpcRes,
                                      gAfterCurrencyTag, NULL, &localStatus);
            if (localStatus == U_USING_FALLBACK_WARNING || U_SUCCESS(localStatus)) {
                localStatus = U_ZERO_ERROR;
                for (int32_t i = 0; i < UNUM_CURRENCY_SPACING_COUNT; i++) {
                  currencySpcAfterSym[i] = ures_getUnicodeStringByKey(dataRes, keywords[i], &localStatus);
                }
                ures_close(dataRes);
            }
            ures_close(currencySpcRes);
            ures_close(currencyResource);
        }
    }
    ures_close(resource);
    ures_close(numberElementsRes);

}

void
DecimalFormatSymbols::initialize() {
    




    fSymbols[kDecimalSeparatorSymbol] = (UChar)0x2e;    
    fSymbols[kGroupingSeparatorSymbol].remove();        
    fSymbols[kPatternSeparatorSymbol] = (UChar)0x3b;    
    fSymbols[kPercentSymbol] = (UChar)0x25;             
    fSymbols[kZeroDigitSymbol] = (UChar)0x30;           
    fSymbols[kOneDigitSymbol] = (UChar)0x31;            
    fSymbols[kTwoDigitSymbol] = (UChar)0x32;            
    fSymbols[kThreeDigitSymbol] = (UChar)0x33;          
    fSymbols[kFourDigitSymbol] = (UChar)0x34;           
    fSymbols[kFiveDigitSymbol] = (UChar)0x35;           
    fSymbols[kSixDigitSymbol] = (UChar)0x36;            
    fSymbols[kSevenDigitSymbol] = (UChar)0x37;          
    fSymbols[kEightDigitSymbol] = (UChar)0x38;          
    fSymbols[kNineDigitSymbol] = (UChar)0x39;           
    fSymbols[kDigitSymbol] = (UChar)0x23;               
    fSymbols[kPlusSignSymbol] = (UChar)0x002b;          
    fSymbols[kMinusSignSymbol] = (UChar)0x2d;           
    fSymbols[kCurrencySymbol] = (UChar)0xa4;            
    fSymbols[kIntlCurrencySymbol].setTo(TRUE, INTL_CURRENCY_SYMBOL_STR, 2);
    fSymbols[kMonetarySeparatorSymbol] = (UChar)0x2e;   
    fSymbols[kExponentialSymbol] = (UChar)0x45;         
    fSymbols[kPerMillSymbol] = (UChar)0x2030;           
    fSymbols[kPadEscapeSymbol] = (UChar)0x2a;           
    fSymbols[kInfinitySymbol] = (UChar)0x221e;          
    fSymbols[kNaNSymbol] = (UChar)0xfffd;               
    fSymbols[kSignificantDigitSymbol] = (UChar)0x0040;  
    fSymbols[kMonetaryGroupingSeparatorSymbol].remove(); 
}

Locale
DecimalFormatSymbols::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocale(type, status);
}

const UnicodeString&
DecimalFormatSymbols::getPatternForCurrencySpacing(UCurrencySpacing type,
                                                 UBool beforeCurrency,
                                                 UErrorCode& status) const {
    if (U_FAILURE(status)) {
      return fNoSymbol;  
    }
    if (beforeCurrency) {
      return currencySpcBeforeSym[(int32_t)type];
    } else {
      return currencySpcAfterSym[(int32_t)type];
    }
}

void
DecimalFormatSymbols::setPatternForCurrencySpacing(UCurrencySpacing type,
                                                   UBool beforeCurrency,
                                             const UnicodeString& pattern) {
  if (beforeCurrency) {
    currencySpcBeforeSym[(int32_t)type] = pattern;
  } else {
    currencySpcAfterSym[(int32_t)type] =  pattern;
  }
}
U_NAMESPACE_END

#endif 


