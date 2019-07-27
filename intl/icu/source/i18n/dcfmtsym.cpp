


















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
#include "unicode/utf16.h"
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

DecimalFormatSymbols::DecimalFormatSymbols()
        : UObject(),
          locale(Locale::getRoot()),
          currPattern(NULL) {
    *validLocale = *actualLocale = 0;
    initialize();
}

DecimalFormatSymbols*
DecimalFormatSymbols::createWithLastResortData(UErrorCode& status) {
    if (U_FAILURE(status)) { return NULL; }
    DecimalFormatSymbols* sym = new DecimalFormatSymbols();
    if (sym == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return sym;
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
        "superscriptingExponent", 
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
    LocalUResourceBundlePointer resource(ures_open(NULL, locStr, &status));
    LocalUResourceBundlePointer numberElementsRes(
        ures_getByKeyWithFallback(resource.getAlias(), gNumberElements, NULL, &status));

    if (U_FAILURE(status)) {
        if ( useLastResortData ) {
            status = U_USING_DEFAULT_WARNING;
            initialize();
        }
        return;
    }

    
    initialize();

    
    
    
    

    LocalPointer<NumberingSystem> ns(NumberingSystem::createInstance(loc, status));
    if (U_SUCCESS(status) && ns->getRadix() == 10 && !ns->isAlgorithmic()) {
        nsName = ns->getName();
        UnicodeString digitString(ns->getDescription());
        int32_t digitIndex = 0;
        UChar32 digit = digitString.char32At(0);
        fSymbols[kZeroDigitSymbol].setTo(digit);
        for (int32_t i = kOneDigitSymbol; i <= kNineDigitSymbol; ++i) {
            digitIndex += U16_LENGTH(digit);
            digit = digitString.char32At(digitIndex);
            fSymbols[i].setTo(digit);
        }
    } else {
        nsName = gLatn;
    }

    UBool isLatn = !uprv_strcmp(nsName,gLatn);

    UErrorCode nlStatus = U_ZERO_ERROR;
    LocalUResourceBundlePointer nonLatnSymbols;
    if ( !isLatn ) {
        nonLatnSymbols.adoptInstead(
            ures_getByKeyWithFallback(numberElementsRes.getAlias(), nsName, NULL, &nlStatus));
        ures_getByKeyWithFallback(nonLatnSymbols.getAlias(), gSymbols, nonLatnSymbols.getAlias(), &nlStatus);
    }

    LocalUResourceBundlePointer latnSymbols(
        ures_getByKeyWithFallback(numberElementsRes.getAlias(), gLatn, NULL, &status));
    ures_getByKeyWithFallback(latnSymbols.getAlias(), gSymbols, latnSymbols.getAlias(), &status);

    UBool kMonetaryDecimalSet = FALSE;
    UBool kMonetaryGroupingSet = FALSE;
    for(int32_t i = 0; i<kFormatSymbolCount; i++) {
        if ( gNumberElementKeys[i] != NULL ) {
            UErrorCode localStatus = U_ZERO_ERROR;
            if ( !isLatn ) {
                sym = ures_getStringByKeyWithFallback(nonLatnSymbols.getAlias(),
                                                      gNumberElementKeys[i], &len, &localStatus);
                
                
                if ( U_FAILURE(localStatus) ) {
                    localStatus = U_ZERO_ERROR;
                    sym = ures_getStringByKeyWithFallback(latnSymbols.getAlias(),
                                                          gNumberElementKeys[i], &len, &localStatus);
                }
            } else {
                    sym = ures_getStringByKeyWithFallback(latnSymbols.getAlias(),
                                                          gNumberElementKeys[i], &len, &localStatus);
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

    
    

    if ( !kMonetaryDecimalSet ) {
        setSymbol(kMonetarySeparatorSymbol,fSymbols[kDecimalSeparatorSymbol]);
    }
    if ( !kMonetaryGroupingSet ) {
        setSymbol(kMonetaryGroupingSeparatorSymbol,fSymbols[kGroupingSeparatorSymbol]);
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
    locBased.setLocaleIDs(ures_getLocaleByType(numberElementsRes.getAlias(),
                                               ULOC_VALID_LOCALE, &status),
                          ures_getLocaleByType(numberElementsRes.getAlias(),
                                               ULOC_ACTUAL_LOCALE, &status));

    
    UChar ucc[4]={0}; 
    int32_t uccLen = 4;
    const char* locName = loc.getName();
    UErrorCode localStatus = U_ZERO_ERROR;
    uccLen = ucurr_forLocale(locName, ucc, uccLen, &localStatus);

    if(U_SUCCESS(localStatus) && uccLen > 0) {
        char cc[4]={0};
        u_UCharsToChars(ucc, cc, uccLen);
        
        LocalUResourceBundlePointer currencyResource(ures_open(U_ICUDATA_CURR, locStr, &localStatus));
        LocalUResourceBundlePointer currency(
            ures_getByKeyWithFallback(currencyResource.getAlias(), "Currencies", NULL, &localStatus));
        ures_getByKeyWithFallback(currency.getAlias(), cc, currency.getAlias(), &localStatus);
        if(U_SUCCESS(localStatus) && ures_getSize(currency.getAlias())>2) { 
            ures_getByIndex(currency.getAlias(), 2, currency.getAlias(), &localStatus);
            int32_t currPatternLen = 0;
            currPattern =
                ures_getStringByIndex(currency.getAlias(), (int32_t)0, &currPatternLen, &localStatus);
            UnicodeString decimalSep =
                ures_getUnicodeStringByIndex(currency.getAlias(), (int32_t)1, &localStatus);
            UnicodeString groupingSep =
                ures_getUnicodeStringByIndex(currency.getAlias(), (int32_t)2, &localStatus);
            if(U_SUCCESS(localStatus)){
                fSymbols[kMonetaryGroupingSeparatorSymbol] = groupingSep;
                fSymbols[kMonetarySeparatorSymbol] = decimalSep;
                
                status = localStatus;
            }
        }
        
        
    }
        

    
    localStatus = U_ZERO_ERROR;
    LocalUResourceBundlePointer currencyResource(ures_open(U_ICUDATA_CURR, locStr, &localStatus));
    LocalUResourceBundlePointer currencySpcRes(
        ures_getByKeyWithFallback(currencyResource.getAlias(),
                                  gCurrencySpacingTag, NULL, &localStatus));

    if (localStatus == U_USING_FALLBACK_WARNING || U_SUCCESS(localStatus)) {
        const char* keywords[UNUM_CURRENCY_SPACING_COUNT] = {
            gCurrencyMatchTag, gCurrencySudMatchTag, gCurrencyInsertBtnTag
        };
        localStatus = U_ZERO_ERROR;
        LocalUResourceBundlePointer dataRes(
            ures_getByKeyWithFallback(currencySpcRes.getAlias(),
                                      gBeforeCurrencyTag, NULL, &localStatus));
        if (localStatus == U_USING_FALLBACK_WARNING || U_SUCCESS(localStatus)) {
            localStatus = U_ZERO_ERROR;
            for (int32_t i = 0; i < UNUM_CURRENCY_SPACING_COUNT; i++) {
                currencySpcBeforeSym[i] =
                    ures_getUnicodeStringByKey(dataRes.getAlias(), keywords[i], &localStatus);
            }
        }
        dataRes.adoptInstead(
            ures_getByKeyWithFallback(currencySpcRes.getAlias(),
                                      gAfterCurrencyTag, NULL, &localStatus));
        if (localStatus == U_USING_FALLBACK_WARNING || U_SUCCESS(localStatus)) {
            localStatus = U_ZERO_ERROR;
            for (int32_t i = 0; i < UNUM_CURRENCY_SPACING_COUNT; i++) {
                currencySpcAfterSym[i] =
                    ures_getUnicodeStringByKey(dataRes.getAlias(), keywords[i], &localStatus);
            }
        }
    }
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
    fSymbols[kExponentMultiplicationSymbol] = (UChar)0xd7; 
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


